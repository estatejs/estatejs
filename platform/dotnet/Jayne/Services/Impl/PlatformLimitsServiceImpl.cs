using System;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Models;

namespace Estate.Jayne.Services.Impl
{
    public class PlatformLimitsServiceImpl : IPlatformLimitsService
    {
        private readonly IWorkerKeyCacheService _workerKeyCacheService;
        private readonly PlatformLimitsConfig _config;

        private uint? _maybeMaxAccounts;
        private uint? _maybeWorkersPerUser;
        private uint? _maybeMaxWorkers;
        
        private bool _hasInit;
        private DateTime? _lastUpdated;
        private readonly SemaphoreSlim _updateSemaphore = new(1);

        public PlatformLimitsServiceImpl(PlatformLimitsConfig config, IWorkerKeyCacheService workerKeyCacheService)
        {
            Requires.NotDefault(nameof(config), config);
            Requires.NotDefault(nameof(workerKeyCacheService), workerKeyCacheService);
            _workerKeyCacheService = workerKeyCacheService;
            _config = config;
        }

        private bool CanUpdate => !_lastUpdated.HasValue || DateTime.UtcNow - _lastUpdated > _config.UpdateFrequency;

        private async Task<bool> MaybeUpdateLimitsAsync(CancellationToken cancellationToken, bool init)
        {
            if (!init && !_hasInit)
                throw new InvalidOperationException("attempted to access before init");

            if (!CanUpdate)
                return false;
            
            await _updateSemaphore.WaitAsync(0, cancellationToken);
            try
            {
                if (CanUpdate)
                {
                    var logMessage = new StringBuilder();

                    var maxAccountsPrev = _maybeMaxAccounts;
                    _maybeMaxAccounts = await _workerKeyCacheService.GetMaxAccountsAsync(_config.DefaultMaxAccounts);
                    if (_maybeMaxAccounts != maxAccountsPrev)
                        logMessage.Append($"MaxAccounts = {_maybeMaxAccounts} ({(_maybeMaxAccounts != _config.DefaultMaxAccounts ? "overridden" : "default")})\n");

                    var workersPerUserPrev = _maybeWorkersPerUser;
                    _maybeWorkersPerUser = await _workerKeyCacheService.GetWorkersPerUserAsync(_config.DefaultWorkersPerUser);
                    if (_maybeWorkersPerUser != workersPerUserPrev)
                        logMessage.AppendFormat("WorkersPerUser = {0} ({1})\n", _maybeWorkersPerUser,
                            _maybeWorkersPerUser != _config.DefaultWorkersPerUser ? "overridden" : "default");

                    var maxWorkersPrev = _maybeMaxWorkers;
                    _maybeMaxWorkers = await _workerKeyCacheService.GetMaxWorkersAsync(_config.DefaultMaxWorkers);
                    if (_maybeMaxWorkers != maxWorkersPrev)
                        logMessage.AppendFormat("MaxWorkers = {0} ({1})\n", _maybeMaxWorkers,
                            _maybeMaxWorkers != _config.DefaultMaxWorkers ? "overridden" : "default");

                    if (logMessage.Length > 0)
                        Log.Info("Platform limits have been updated: \n" + logMessage.ToString().TrimEnd());
                
                    _lastUpdated = DateTime.UtcNow;
                    return true;
                }
            }
            finally
            {
                _updateSemaphore.Release();
            }

            return false;
        }

        public async Task<PlatformLimits> GetLimitsAsync(CancellationToken cancellationToken)
        {
            await MaybeUpdateLimitsAsync(cancellationToken, false);
            return new PlatformLimits(_maybeMaxAccounts ?? 0, _maybeWorkersPerUser ?? 0, _maybeMaxWorkers ?? 0);
        }
        
        public async Task InitializeAsync()
        {
            if (_hasInit)
                throw new InvalidOperationException("attempted to re-init");

            if (!await MaybeUpdateLimitsAsync(CancellationToken.None, true))
                throw new Exception("Unable to update limits during initialization");
            
            _hasInit = true;
        }
    }
}
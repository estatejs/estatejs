using System;
using System.Threading.Tasks;
using StackExchange.Redis;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Errors;
using Estate.Jayne.Util;

namespace Estate.Jayne.Services.Impl
{
    public class WorkerKeyCacheServiceImpl : IWorkerKeyCacheService
    {
        private readonly ConnectionMultiplexer _redis;
        private readonly TimeSpan _accountCreationTokenTTL;
        private readonly TimeSpan _emailVerifiedTTL;

        public WorkerKeyCacheServiceImpl(WorkerKeyCacheServiceConfig config)
        {
            Requires.NotDefault(nameof(config), config);
            config.Validate();

            var configOptions = RedisUtil.ParseConfigurationOptions(config.RedisEndpoint);
            _redis = ConnectionMultiplexer.Connect(configOptions);
            _accountCreationTokenTTL = config.AccountCreationTokenTTL;
            _emailVerifiedTTL = config.EmailVerifiedTTL;
        }

        //NOTE: This MUST match what's in cpp/lib/internal/include/estate/internal/worker_authentication.h
        private string GetUserKeyKey(string userKey) => "uk:" + userKey;
        private string GetAdminKeyKey(string adminKey) => "ak:" + adminKey;
        private string GetAccountCreationTokenKey(string accountCreationToken) => "act:" + accountCreationToken;
        private string GetEmailVerifiedKey(string accountCreationToken) => "ev:" + accountCreationToken;
        private string GetLimitKey(string name) => "l:" + name;

        public async Task<ulong?> TryGetWorkerIdByUserKeyAsync(string userKey)
        {
            Requires.NotNullOrWhitespace(nameof(userKey), userKey);
            var db = _redis.GetDatabase();
            var result = await db.StringGetAsync(GetUserKeyKey(userKey));
            return result.HasValue && result.IsInteger ? (ulong?) result : null;
        }

        public async Task<string> TryGetWorkerOwnerUserIdByAdminKeyAsync(string adminKey)
        {
            Requires.NotNullOrWhitespace(nameof(adminKey), adminKey);
            var db = _redis.GetDatabase();
            var result = await db.StringGetAsync(GetAdminKeyKey(adminKey));
            if (result.HasValue)
                return result.ToString();
            return null;
        }

        public async Task<string> TryGetWorkerOwnerUserIdByAccountCreationTokenOnceAsync(string accountCreationToken)
        {
            Requires.NotNullOrWhitespace(nameof(accountCreationToken), accountCreationToken);
            var db = _redis.GetDatabase();
            var result = await db.StringGetSetAsync(GetAccountCreationTokenKey(accountCreationToken), RedisValue.EmptyString);
            if (result.HasValue)
            {
                var str = result.ToString();
                return string.IsNullOrWhiteSpace(str) ? null : str;
            }
            return null;
        }

        public async Task<bool> TrySetUserKeyAsync(string userKey, ulong workerId)
        {
            Requires.NotNullOrWhitespace(nameof(userKey), userKey);

            var db = _redis.GetDatabase();
            return await db.StringSetAsync(GetUserKeyKey(userKey), workerId);
        }

        public async Task<bool> TrySetAdminKeyAsync(string adminKey, string workerOwnerUserId)
        {
            Requires.NotNullOrWhitespace(nameof(adminKey), adminKey);
            Requires.NotNullOrWhitespace(nameof(workerOwnerUserId), workerOwnerUserId);

            var db = _redis.GetDatabase();
            return await db.StringSetAsync(GetAdminKeyKey(adminKey), workerOwnerUserId);
        }
        
        public async Task<bool> TrySetAccountCreationTokenAsync(string accountCreationToken, string workerOwnerUserId)
        {
            Requires.NotNullOrWhitespace(nameof(accountCreationToken), accountCreationToken);
            Requires.NotNullOrWhitespace(nameof(workerOwnerUserId), workerOwnerUserId);

            var db = _redis.GetDatabase();
            return await db.StringSetAsync(GetAccountCreationTokenKey(accountCreationToken), workerOwnerUserId, _accountCreationTokenTTL);
        }

        public async Task<bool> TrySetEmailVerifiedAsync(string accountCreationToken)
        {
            Requires.NotNullOrWhitespace(nameof(accountCreationToken), accountCreationToken);
            var db = _redis.GetDatabase();
            return await db.StringSetAsync(GetEmailVerifiedKey(accountCreationToken), "verified", _emailVerifiedTTL);
        }

        public async Task<bool> TryGetEmailVerifiedOnceAsync(string accountCreationToken)
        {
            Requires.NotNullOrWhitespace(nameof(accountCreationToken), accountCreationToken);
            var db = _redis.GetDatabase();
            
            var result = await db.StringGetSetAsync(GetEmailVerifiedKey(accountCreationToken), RedisValue.EmptyString);
            return result.HasValue && !result.IsNullOrEmpty;
        }
        
        private async Task<uint> GetLimitAsync(string name, uint defaultValue)
        {
            var db= _redis.GetDatabase();
            var result = await db.StringGetAsync(GetLimitKey(name));
            if (result.HasValue)
            {
                if (!result.TryParse(out long val))
                    throw new Exception($"Unable to parse limit {name} value: " + result.ToString());
                if(val is < 0 or > uint.MaxValue)
                    throw new Exception($"Invalid limit {name} value: " + result.ToString());
                return (uint) val;
            }

            return defaultValue;
        }
        
        public async Task<uint> GetMaxAccountsAsync(uint defaultMaxAccounts)
        {
            return await GetLimitAsync("MaxAccounts", defaultMaxAccounts);
        }

        public async Task<uint> GetWorkersPerUserAsync(uint defaultWorkersPerUser)
        {
            return await GetLimitAsync("WorkersPerUser", defaultWorkersPerUser);
        }

        public async Task<uint> GetMaxWorkersAsync(uint defaultMaxWorkers)
        {
            return await GetLimitAsync("MaxWorkers", defaultMaxWorkers);
        }

        public async Task SetEmailVerifiedAsync(string accountCreationToken)
        {
            if (!await TrySetEmailVerifiedAsync(accountCreationToken))
            {
                Log.Error("Failed to set email verified in the key cache");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToSetEmailVerifiedInKeyCache);
            }
        }
        
        public async Task<uint> SetAccountCreationTokenAsync(string accountCreationToken, string workerOwnerUserId)
        {
            if (!await TrySetAccountCreationTokenAsync(accountCreationToken, workerOwnerUserId))
            {
                Log.Error("Failed to set account creation token in the key cache");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToSetAccountCreationTokenInKeyCache);
            }

            return (uint) _accountCreationTokenTTL.TotalSeconds;
        }

        public async Task SetUserKeyAsync(string userKey, ulong workerId)
        {
            if (!await TrySetUserKeyAsync(userKey, workerId))
            {
                Log.Error("Failed to set user key in key cache");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToSetUserKeyInKeyCache);
            }
        }

        public async Task SetAdminKeyAsync(string adminKey, string workerOwnerUserId)
        {
            if (!await TrySetAdminKeyAsync(adminKey, workerOwnerUserId))
            {
                Log.Error("Failed to set admin key in key cache");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToSetAdminKeyInKeyCache);
            }
        }

        public async Task DeleteUserKeyAsync(string userKey)
        {
            Requires.NotNullOrWhitespace(nameof(userKey), userKey);
            var db = _redis.GetDatabase();
            await db.KeyDeleteAsync(GetUserKeyKey(userKey));
        }

        public async Task DeleteAdminKeyAsync(string adminKey)
        {
            Requires.NotNullOrWhitespace(nameof(adminKey), adminKey);
            var db = _redis.GetDatabase();
            await db.KeyDeleteAsync(GetAdminKeyKey(adminKey));
        }
    }
}
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Microsoft.EntityFrameworkCore;
using Estate.Jayne.AccountsEntities;
using Estate.Jayne.ApiModels;
using Estate.Jayne.ApiModels.Request;
using Estate.Jayne.ApiModels.Response;
using Estate.Jayne.Common;
using Estate.Jayne.Common.Exceptions;
using Estate.Jayne.Config;
using Estate.Jayne.Errors;
using Estate.Jayne.Protocol;
using Estate.Jayne.Services;
using Estate.Jayne.Util;

// ReSharper disable MethodHasAsyncOverloadWithCancellation

namespace Estate.Jayne.Systems.Impl
{
    internal class WorkerAdminSystemImpl : IWorkerAdminSystem
    {
        private readonly AccountsContext _accountsContext;
        private readonly IParserFactoryService _parserFactoryService;
        private readonly IHttpContextAccessor _httpContextAccessor;
        private readonly ISerenityService _serenityService;
        private readonly IWorkerKeyCacheService _workerKeyCacheService;
        private readonly IProtocolSerializer _protocolSerializer;
        private readonly IPlatformLimitsService _platformLimitsService;
        private readonly HashSet<string> _builtInUserIds = new();


        public WorkerAdminSystemImpl(AccountsContext accountsContext,
            IParserFactoryService parserFactoryService,
            IHttpContextAccessor httpContextAccessor,
            ISerenityService serenityService,
            IWorkerKeyCacheService workerKeyCacheService,
            IProtocolSerializer protocolSerializer,
            IPlatformLimitsService platformLimitsService,
            BuiltInAccountSecrets builtInAccountSecrets)
        {
            Requires.NotDefault(nameof(parserFactoryService), parserFactoryService);
            Requires.NotDefault(nameof(accountsContext), accountsContext);
            Requires.NotDefault(nameof(httpContextAccessor), httpContextAccessor);
            Requires.NotDefault(nameof(serenityService), serenityService);
            Requires.NotDefault(nameof(workerKeyCacheService), workerKeyCacheService);
            Requires.NotDefault(nameof(protocolSerializer), protocolSerializer);
            Requires.NotDefault(nameof(platformLimitsService), platformLimitsService);
            Requires.NotDefault(nameof(builtInAccountSecrets), builtInAccountSecrets);

            _accountsContext = accountsContext;
            _parserFactoryService = parserFactoryService;
            _httpContextAccessor = httpContextAccessor;
            _serenityService = serenityService;
            _workerKeyCacheService = workerKeyCacheService;
            _protocolSerializer = protocolSerializer;
            _platformLimitsService = platformLimitsService;

            foreach (var builtInAccount in builtInAccountSecrets.Accounts)
                _builtInUserIds.Add(builtInAccount.UserId);
        }

        public async Task<DeployWorkerResponse> DeployWorkerAsync(CancellationToken cancellationToken, DeployWorkerRequest request)
        {
            Requires.NotDefault(nameof(request), request);
            
            if (!string.IsNullOrWhiteSpace(request.workerIdStr))
                return await UpdateWorkerAsync(cancellationToken, request);

            return await CreateWorkerAsync(cancellationToken, request);
        }

        public async Task<GetWorkerConnectInfoResponse> GetWorkerConnectionInfoAsync(CancellationToken cancellationToken,
            string logContext, string workerName)
        {
            Requires.NotNullOrWhitespace(nameof(logContext), logContext);
            Requires.NotNullOrWhitespace(nameof(workerName), workerName);

            var userId = _httpContextAccessor.GetUserId();

            var query =
                from w in _accountsContext.Worker
                join a in _accountsContext.Account on w.AccountRowId equals a.RowId
                join wi in _accountsContext.WorkerIndex on w.Id equals wi.WorkerId
                join wtd in _accountsContext.WorkerTypeDefinition on w.Id equals wtd.WorkerId
                where a.UserId == userId && !a.Deleted.HasValue && w.Name == workerName
                select new {wi.Index, w.UserKey, wtd.CompressedTypeDefinitions};

            var item = await query.SingleOrDefaultAsync(cancellationToken);

            if (item == null)
            {
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.WorkerNotFoundByName);
            }

            Log.Info($"GetWorkerConnectInfo: Ok (worker:{workerName})");
            
            return new GetWorkerConnectInfoResponse
            {
                logContext = logContext,
                userKey = item.UserKey,
                workerIndexStr = Convert.ToBase64String(item.Index),
                compressedWorkerTypeDefinitionsStr = Convert.ToBase64String(item.CompressedTypeDefinitions)
            };
        }

        public async Task<ListWorkersResponse> ListWorkersAsync(CancellationToken cancellationToken, string logContext)
        {
            Requires.NotNullOrWhitespace(nameof(logContext), logContext);
            var userId = _httpContextAccessor.GetUserId();

            var query = from w in _accountsContext.Worker
                join a in _accountsContext.Account on w.AccountRowId equals a.RowId
                where a.UserId == userId && !a.Deleted.HasValue
                select w.Name;

            var names = await query.ToArrayAsync(cancellationToken);

            Log.Info($"ListWorkers: Ok");
            
            return new ListWorkersResponse
            {
                logContext = logContext,
                ownedWorkers = names
            };
        }

        public async Task DeleteWorkerAsync(CancellationToken cancellationToken, DeleteWorkerRequest request)
        {
            var userId = _httpContextAccessor.GetUserId();

            var query = from w in _accountsContext.Worker
                join a in _accountsContext.Account on w.AccountRowId equals a.RowId
                where a.UserId == userId && !a.Deleted.HasValue && w.Name == request.workerName
                select w;

            var worker = await query.SingleOrDefaultAsync(cancellationToken);
            if (worker == null)
            {
                Log.Error("Worker was not found while trying to delete it");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.WorkerNotFoundById);
            }

            //delete it from Serenity first because the version may be wrong.
            await _serenityService.DeleteWorkerAsync(cancellationToken, request.logContext, worker.Id, worker.Version);

            //remove it from the database
            _accountsContext.Worker.Remove(worker);
            await _accountsContext.SaveChangesAsync(cancellationToken);

            //remove the user key so it can't be used anymore
            await _workerKeyCacheService.DeleteUserKeyAsync(worker.UserKey);
            
            Log.Info($"DeleteWorker: Ok (name: {worker.Name}, id: {worker.Id})");
        }
        
        private async Task<DeployWorkerResponse> UpdateWorkerAsync(CancellationToken cancellationToken,
            DeployWorkerRequest request)
        {
            if (!ulong.TryParse(request.currentWorkerVersionStr, out var previousWorkerVersion))
            {
                Log.Error("Missing current worker version in worker update request");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.MissingCurrentWorkerVersion);
            }

            if (!ulong.TryParse(request.workerIdStr, out var workerId))
            {
                Log.Error("Missing worker id in worker update request");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.MissingWorkerId);
            }

            var compressedWorkerTypeDefinitions = request.GetCompressedWorkerTypeDefinitionsOrDefault();
            if (compressedWorkerTypeDefinitions == default)
            {
                Log.Error("Missing type definitions");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.MissingTypeDefinitions);
            }
            
            var workerIndexCreator = _parserFactoryService.GetParser(request.language);
            
            var parsedClassMappings = workerIndexCreator.ParseClassMappings(request.workerClassMappings);

            var userId = _httpContextAccessor.GetUserId();

            var workerName = request.workerName;

            await using var tx = await _accountsContext.Database.BeginTransactionAsync(cancellationToken);

            var query = from w in _accountsContext.Worker
                join a in _accountsContext.Account on w.AccountRowId equals a.RowId
                join wi in _accountsContext.WorkerIndex on w.Id equals wi.WorkerId
                join wtd in _accountsContext.WorkerTypeDefinition on w.Id equals wtd.WorkerId
                where a.UserId == userId &&
                      !a.Deleted.HasValue &&
                      w.Id == workerId &&
                      w.Version == previousWorkerVersion
                select new {Worker = w, WorkerIndex = wi, WorkerTypeDefinition = wtd};

            var workerInfo = await query.SingleOrDefaultAsync(cancellationToken);
            if (workerInfo == default)
            {
                Log.Error(
                    $"Unable to find worker in database for user id {userId}, worker id {workerId}, and version {previousWorkerVersion}");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.WorkerNotFoundPullLatest);
            }

            if (workerInfo.Worker.Name != workerName)
                workerInfo.Worker.Name = workerName;

            var workerVersion = previousWorkerVersion + 1;
            workerInfo.Worker.Version = workerVersion;

            bool worked = false;
            WorkerClassMapping[] classMappings;
            try
            {
                //create the worker index
                var result = workerIndexCreator.ParseWorkerCode(workerId, workerVersion, workerName, request.workerFiles, 
                    parsedClassMappings, request.lastClassId);

                //Execute the pre-compilation
                var code = new List<string>();
                foreach(var directive in result.PreCompilerDirectives)
                    code.Add(directive.PreCompile().code);

                classMappings = workerIndexCreator.CreateClassMappings(result.WorkerIndex);
                var workerIndexBytes = _protocolSerializer.SerializeWorkerIndex(result.WorkerIndex);
                
                // set the worker index bytes
                workerInfo.WorkerIndex.Index = workerIndexBytes;
                
                // set the worker type definition compressed bytes
                workerInfo.WorkerTypeDefinition.CompressedTypeDefinitions = compressedWorkerTypeDefinitions;

                //save the index and type definition changes to the database
                await _accountsContext.SaveChangesAsync(false, cancellationToken);

                //call SetupWorker on Serenity
                await _serenityService.SetupWorkerAsync(cancellationToken, request.logContext, workerId, workerVersion,
                    previousWorkerVersion, workerIndexBytes, code.ToArray());

                //commit the txn in the accounts database
                await tx.CommitAsync(cancellationToken);
                worked = true;
            }
            catch (EstateException)
            {
                throw;
            }
            catch (DbUpdateException ex) when (AccountsContext.WhenDuplicateWorkerName(ex))
            {
                Log.Warning("Duplicate worker name: " + workerName);
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.DuplicateWorkerName);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Unknown error");
                throw JayneErrors.Platform(PlatformErrorCode.UnknownWorkerUpdateFailure);
            }
            finally
            {
                if (!worked)
                    await tx.RollbackAsync(CancellationToken.None);
            }

            Log.Info($"UpdateWorker: Ok (worker:{workerName} v{previousWorkerVersion} -> v{workerVersion}) ");
            
            //return the response
            return new DeployWorkerResponse
            {
                logContext = request.logContext,
                workerIdStr = workerId.ToString(),
                workerVersionStr = workerVersion.ToString(),
                workerClassMappings = classMappings
            };
        }

        private async Task<DeployWorkerResponse> CreateWorkerAsync(CancellationToken cancellationToken,
            DeployWorkerRequest request)
        {
            var userId = _httpContextAccessor.GetUserId();
            bool builtIn = _builtInUserIds.Contains(userId);
            
            if(!builtIn)
            {
                var limits = await _platformLimitsService.GetLimitsAsync(cancellationToken);

                //Enforce WorkersPerUser limit first because if they delete one of their workers there's a good chance they'll skip the max worker limit.
                var userWorkers = await (from w in _accountsContext.Worker
                    join a in _accountsContext.Account on w.AccountRowId equals a.RowId
                    where a.UserId == userId &&
                          !a.Deleted.HasValue
                    select w.Id).CountAsync(cancellationToken);
                if (userWorkers >= limits.WorkersPerUser)
                {
                    Log.Warning(
                        $"Unable to create worker because the user already has {userWorkers} workers which has reached the maximum allowable workers per user({limits.WorkersPerUser}).");
                    throw JayneErrors.Platform(PlatformErrorCode.UserWorkerLimitReached);
                }

                //Enforce MaxWorkers limit
                var totalWorkers = await (from w in _accountsContext.Worker
                    select w.Id).CountAsync(cancellationToken);
                if (totalWorkers >= limits.MaxWorkers)
                {
                    Log.Warning(
                        $"Unable to create worker because the total number of workers ({totalWorkers}) has reached the maximum allowable workers ({limits.MaxWorkers}).");
                    throw JayneErrors.Platform(PlatformErrorCode.MaxWorkerLimitReached);
                }
            }
            else
            {
                Log.Info("Skipping worker limit enforcement for built-in account.");
            }
            
            var compressedWorkerTypeDefinitions = request.GetCompressedWorkerTypeDefinitionsOrDefault();
            if (compressedWorkerTypeDefinitions == default)
            {
                Log.Error("Missing type definitions");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.MissingTypeDefinitions);
            }

            var workerIndexCreator = _parserFactoryService.GetParser(request.language);
            
            var parsedClassMappings = workerIndexCreator.ParseClassMappings(request.workerClassMappings);

            var query = from a in _accountsContext.Account
                where !a.Deleted.HasValue && a.UserId == userId
                select a.RowId;

            var accountRowId = await query.SingleOrDefaultAsync(cancellationToken);
            if (accountRowId == default)
            {
                Log.Error($"Unable to find account in database for user id {userId}");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UserNotFound);
            }

            var userKey = KeyUtil.GenerateUserKey();

            //create it in the database
            string workerName = request.workerName;
            ulong workerId;
            ulong workerVersion = 1;

            await using var tx = await _accountsContext.Database.BeginTransactionAsync(cancellationToken);

            try
            {
                var worker = new WorkerEntity
                {
                    Name = workerName,
                    Version = workerVersion,
                    UserKey = userKey,
                    AccountRowId = accountRowId
                };
                _accountsContext.Worker.Add(worker);
                await _accountsContext.SaveChangesAsync(true, cancellationToken);
                workerId = worker.Id;
                Debug.Assert(workerId > 0);
            }
            catch (DbUpdateException ex) when (AccountsContext.WhenDuplicateWorkerName(ex))
            {
                Log.Warning("Duplicate worker name: " + request.workerName);
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.DuplicateWorkerName);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Unknown error");
                throw JayneErrors.Platform(PlatformErrorCode.UnknownWorkerCreationFailure);
            }

            bool completed = false;
            bool hasSetupWorker = false;
            bool hasSetUserKey = false;
            WorkerClassMapping[] classMappings;
            try
            {
                //create the WorkerIndex
                var result = workerIndexCreator.ParseWorkerCode(workerId, workerVersion, workerName, request.workerFiles, 
                    parsedClassMappings, request.lastClassId);

                classMappings = workerIndexCreator.CreateClassMappings(result.WorkerIndex);
                var workerIndexBytes = _protocolSerializer.SerializeWorkerIndex(result.WorkerIndex);

                //Execute the pre-compilation
                var code = new List<string>();
                foreach(var directive in result.PreCompilerDirectives)
                    code.Add(directive.PreCompile().code);
                
                // add the new worker index
                _accountsContext.WorkerIndex.Add(new WorkerIndexEntity
                {
                    Index = workerIndexBytes,
                    WorkerId = workerId
                });
                
                // add the worker type definitions
                _accountsContext.WorkerTypeDefinition.Add(new WorkerTypeDefinitionEntity
                {
                    WorkerId = workerId,
                    CompressedTypeDefinitions = compressedWorkerTypeDefinitions
                });

                await _accountsContext.SaveChangesAsync(cancellationToken);

                //call SetupWorker on Serenity
                await _serenityService.SetupWorkerAsync(cancellationToken, request.logContext, workerId, workerVersion, null,
                    workerIndexBytes, code.ToArray());

                hasSetupWorker = true;

                //set the Worker's user key in the key cache
                await _workerKeyCacheService.SetUserKeyAsync(userKey, workerId);
                hasSetUserKey = true;

                //cool
                await tx.CommitAsync(cancellationToken);
                completed = true;
            }
            catch (EstateException)
            {
                throw;
            }
            catch (Exception e)
            {
                Log.Critical(e,$"Worker rollback: Unknown error caused worker creation failure for worker {workerId}. Rolling back changes.");
                throw JayneErrors.Platform(PlatformErrorCode.UnknownWorkerCreationFailure, e);
            }
            finally
            {
                if (!completed)
                {
                    //rollback
                    try
                    {
                        await tx.RollbackAsync(CancellationToken.None);
                    }
                    catch (Exception e)
                    {
                        Log.Critical(e,$"Worker rollback failed: Unable to rollback database changes for {workerId}. It must be deleted manually.");
                    }
                    
                    if (hasSetUserKey)
                    {
                        try
                        {
                            await _workerKeyCacheService.DeleteUserKeyAsync(userKey);
                        }
                        catch (Exception e)
                        {
                            Log.Critical(e,$"Worker rollback failed: Unable to delete the user key {userKey} from Redis for worker {workerId}. It must be deleted manually.");
                        }
                    }

                    if (hasSetupWorker)
                    {
                        try
                        {
                            await _serenityService.DeleteWorkerAsync(cancellationToken, request.logContext, workerId,
                                workerVersion);
                            Log.Warning($"Worker rollback: successfully removed {workerId} from Serenity");
                        }
                        catch (Exception e)
                        {
                            Log.Critical(e,$"Worker rollback failed: Unable to delete worker from Serenity during roll back for worker {workerId}. It must be deleted manually.");
                        }
                    }
                }
            }
            
            Log.Info($"CreateWorker: Ok (worker:{workerName})");

            return new DeployWorkerResponse
            {
                logContext = request.logContext,
                workerIdStr = workerId.ToString(),
                workerVersionStr = workerVersion.ToString(),
                workerClassMappings = classMappings
            };
        }
    }
}
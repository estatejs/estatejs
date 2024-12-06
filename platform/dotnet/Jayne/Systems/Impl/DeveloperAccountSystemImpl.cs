using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Encodings.Web;
using System.Threading;
using System.Threading.Tasks;
using System.Web;
using FirebaseAdmin.Auth;
using Microsoft.AspNetCore.Http;
using Microsoft.EntityFrameworkCore;
using Estate.Jayne.AccountsEntities;
using Estate.Jayne.ApiModels.Response;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Errors;
using Estate.Jayne.Services;
using Estate.Jayne.Util;

namespace Estate.Jayne.Systems.Impl
{
    internal class DeveloperAccountSystemImpl : IDeveloperAccountSystem
    {
        private readonly IHttpContextAccessor _httpContextAccessor;
        private readonly AccountsContext _accountsContext;
        private readonly IWorkerKeyCacheService _workerKeyCacheService;
        private readonly ISerenityService _serenityService;
        private readonly IPlatformLimitsService _platformLimitsService;

        private readonly Dictionary<string, BuiltInAccount> _builtInUsersByEmail = new();

        public DeveloperAccountSystemImpl(IHttpContextAccessor httpContextAccessor,
            AccountsContext accountsContext,
            IWorkerKeyCacheService workerKeyCacheService,
            ISerenityService serenityService,
            IPlatformLimitsService platformLimitsService,
            BuiltInAccountSecrets builtInAccountSecrets)
        {
            Requires.NotDefault(nameof(accountsContext), accountsContext);
            Requires.NotDefault(nameof(httpContextAccessor), httpContextAccessor);
            Requires.NotDefault(nameof(workerKeyCacheService), workerKeyCacheService);
            Requires.NotDefault(nameof(serenityService), serenityService);
            Requires.NotDefault(nameof(platformLimitsService), platformLimitsService);
            Requires.NotDefault(nameof(builtInAccountSecrets), builtInAccountSecrets);

            _httpContextAccessor = httpContextAccessor;
            _accountsContext = accountsContext;
            _workerKeyCacheService = workerKeyCacheService;
            _serenityService = serenityService;
            _platformLimitsService = platformLimitsService;

            foreach (var builtInAccount in builtInAccountSecrets.Accounts)
                _builtInUsersByEmail[builtInAccount.Email] = builtInAccount;
        }

        public async Task PopulateUserKeyCacheAsync()
        {
            var workers = from a in _accountsContext.Account
                join w in _accountsContext.Worker on a.RowId equals w.AccountRowId
                where !a.Deleted.HasValue
                select new {a.Email, a.UserId, w.UserKey, WorkerId = w.Id};

            int errorCount = 0;
            foreach (var worker in workers)
            {
                if (!await _workerKeyCacheService.TrySetUserKeyAsync(worker.UserKey, worker.WorkerId))
                {
                    Log.Error($"Unable to set user key for worker {worker.WorkerId} for account {worker.Email}");
                    ++errorCount;
                }
            }

            if (errorCount > 0)
            {
                Log.Critical($"User key cache population finished with {errorCount} errors!");
            }
            else
            {
                Log.Info("User key cache was populated successfully");
            }
        }

        public async Task DeleteAccountAsync(CancellationToken cancellationToken, string logContext)
        {
            var userId = _httpContextAccessor.HttpContext.User.GetUserId();

            Log.AppendContext("LogContext", logContext);
            Log.AppendContext("UserId", userId);

            //Delete the user from Google Identity but if this fails it means that it shouldn't exist in AccountsDB
            try
            {
                await FirebaseAuth.DefaultInstance.DeleteUserAsync(userId, CancellationToken.None);
            }
            catch (FirebaseAuthException fex) when (fex.AuthErrorCode == AuthErrorCode.UserNotFound)
            {
                Log.Error($"Unable to delete user {userId} from Google Identity.");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UserNotFound);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Failed to delete user from Google Identity");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToDeleteAccountWithGoogleIdentity);
            }

            //Delete the user from AccountsDB
            await using var tx = await _accountsContext.Database.BeginTransactionAsync(cancellationToken);

            var query =
                from a in _accountsContext.Account
                join w in _accountsContext.Worker on a.RowId equals w.AccountRowId into wmaybe
                from wm in wmaybe.DefaultIfEmpty()
                where a.UserId == userId
                select new {Account = a, Worker = wm};

            var values = await query.ToArrayAsync(cancellationToken);

            if (values.Length == 0)
            {
                Log.Warning(
                    $"User {userId} not found in AccountsDB after successfully deleting them from Google Identity.");
                return;
            }

            var workers = values.Where(arg => arg.Worker != null)
                .Select(arg => arg.Worker)
                .ToArray();

            var account = values.First().Account;

            if (workers.Length > 0)
            {
                //delete the workers in the database
                _accountsContext.Worker.RemoveRange(workers);
            }
            
            // delete the account
            _accountsContext.Account.Remove(account);

            await _accountsContext.SaveChangesAsync(cancellationToken);

            //delete the admin key
            await _workerKeyCacheService.DeleteAdminKeyAsync(account.AdminKey);

            //delete all the user keys before trying to delete the workers from Serenity
            foreach (var worker in workers)
            {
                await _workerKeyCacheService.DeleteUserKeyAsync(worker.UserKey);
            }

            await tx.CommitAsync(CancellationToken.None);

            //tell Serenity to delete each of the actual workers.
            foreach (var worker in workers)
            {
                await _serenityService.DeleteWorkerAsync(CancellationToken.None, logContext, worker.Id, worker.Version);
            }

            Log.Info($"DeleteAccount: Ok");
        }

        /// <summary>
        /// Brute force all the admin/user keys into Redis.
        /// </summary>
        public async Task PopulateAdminKeyCacheAsync()
        {
            var accounts = from a in _accountsContext.Account
                where !a.Deleted.HasValue
                select new {a.Email, a.AdminKey, a.UserId};

            int errorCount = 0;
            foreach (var account in accounts)
            {
                if (!await _workerKeyCacheService.TrySetAdminKeyAsync(account.AdminKey, account.UserId))
                {
                    Log.Error("Unable to set admin key for user " + account.Email);
                    ++errorCount;
                }
            }

            if (errorCount > 0)
            {
                Log.Critical($"Key cache population finished with {errorCount} errors!");
            }
            else
            {
                Log.Info("Key cache was populated successfully");
            }
        }

        public async Task<LoginExistingAccountResponse> LoginExistingAccountAsync(CancellationToken cancellationToken,
            string logContext)
        {
            var userId = _httpContextAccessor.HttpContext.User.GetUserId();

            Log.AppendContext("LogContext", logContext);
            Log.AppendContext("UserId", userId);

            var query = from account in _accountsContext.Account
                where account.UserId == userId && !account.Deleted.HasValue
                select new
                {
                    account.AdminKey
                };

            var keys = await query.SingleOrDefaultAsync(cancellationToken);
            if (keys == null)
            {
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UserNotFound);
            }

            Log.Info($"LoginExistingAccount: Ok");
            
            return new LoginExistingAccountResponse
            {
                logContext = logContext,
                adminKey = keys.AdminKey
            };
        }

        public async Task<BeginCreateAccountResponse> BeginCreateAccountAsync(CancellationToken cancellationToken,
            string logContext, string email, string password)
        {
            Requires.NotNullOrWhitespace(nameof(email), email);
            Requires.NotNullOrWhitespace(nameof(password), password);

            Log.AppendContext("LogContext", logContext);
            Log.AppendContext("Email", email);

            // Create the user in Firebase
            var builtInAccount = _builtInUsersByEmail.GetValueOrDefault(email);
            
            string userId;
            bool emailVerified = false;
            if (builtInAccount == null)
            {
                userId = KeyUtil.GenerateUserId();
                
                // Enforce max accounts limit on non-built in accounts.
                var limits = await _platformLimitsService.GetLimitsAsync(cancellationToken);
                var accountsCount = (from a in _accountsContext.Account
                    where !a.Deleted.HasValue
                    select a.RowId).Count();
                if (accountsCount >= limits.MaxAccounts)
                {
                    Log.Warning($"Unable to create account because the total number of accounts ({accountsCount}) has reached the maximum allowable accounts ({limits.MaxAccounts}).");
                    throw JayneErrors.Platform(PlatformErrorCode.AccountCreationTemporarilyDisabled);
                }
            }
            else
            {
                if (builtInAccount.Password != password)
                {
                    Log.Error("Failed to create built-in account because the password was incorrect.");
                    //note: this error is bogus because we shouldn't let the hax0rs know when they're making progress. 
                    throw JayneErrors.Platform(PlatformErrorCode.FailedToCreateAccountWithGoogleIdentity);
                }
                userId = builtInAccount.UserId;
                emailVerified = true;
            }

            try
            {
                var args = new UserRecordArgs()
                {
                    Uid = userId,
                    Email = email,
                    EmailVerified = emailVerified, /* Tools will fire off the verification email. */
                    Password = password,
                    Disabled = false
                };
                await FirebaseAuth.DefaultInstance.CreateUserAsync(args, cancellationToken);
            }
            catch (FirebaseAuthException fex) when (fex.AuthErrorCode == AuthErrorCode.EmailAlreadyExists)
            {
                Log.Error("User already exists");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UserAlreadyExists);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Failed to create account with Google Identity");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToCreateAccountWithGoogleIdentity);
            }

            if (emailVerified)
            {
                Log.Info("BeginCreateAccount: OK (preVerified)");
                
                //finalize the account if it's a built-in user.
                await InternalFinalizeAccountOnceAsync(cancellationToken, null, userId, true);
                return new BeginCreateAccountResponse
                {
                    logContext = logContext,
                    emailVerified = true
                };
            }

            var accountCreationToken = KeyUtil.GenerateAccountCreationToken();
            var expiresInSeconds =
                await _workerKeyCacheService.SetAccountCreationTokenAsync(accountCreationToken, userId);
            
            Log.Info($"BeginCreateAccount: OK (act:{accountCreationToken})");
            
            return new BeginCreateAccountResponse
            {
                logContext = logContext,
                accountCreationToken = accountCreationToken,
                expiresInSeconds = expiresInSeconds,
                emailVerified = false
            };
        }

        public async Task FinalizeAccountOnceAsync(CancellationToken cancellationToken, string logContext,
            string accountCreationToken)
        {
            var userId =
                await _workerKeyCacheService.TryGetWorkerOwnerUserIdByAccountCreationTokenOnceAsync(accountCreationToken);
            if (string.IsNullOrWhiteSpace(userId))
            {
                Log.Error("Failed to finalize user account because the account creation token was unknown.");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.InvalidAccountCreationToken);
            }

            await InternalFinalizeAccountOnceAsync(cancellationToken, accountCreationToken, userId, false);
            
            Log.Info($"FinalizeAccountOnce: Ok (act:{accountCreationToken} == uid:{userId} )");
        }

        public async Task<bool> IsEmailVerifiedAsync(string accountCreationToken)
        {
            var value = await _workerKeyCacheService.TryGetEmailVerifiedOnceAsync(accountCreationToken);
            if (value)
                Log.Info($"IsEmailVerified: True (act:{accountCreationToken})");
            return value;
        }

        private async Task InternalFinalizeAccountOnceAsync(CancellationToken cancellationToken,
            string accountCreationToken, string userId, bool preVerified)
        {
            if (!preVerified)
                Requires.NotNullOrWhitespace(nameof(accountCreationToken), accountCreationToken);

            // Get the user from Firebase
            UserRecord user;
            try
            {
                user = await FirebaseAuth.DefaultInstance.GetUserAsync(userId, cancellationToken);
            }
            catch (FirebaseAuthException fex) when (fex.AuthErrorCode == AuthErrorCode.UserNotFound)
            {
                Log.Error("Failed to finalize user account because the user was not found.");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UserNotFound);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Failed to get account from Google Identity");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToGetAccountFromGoogleIdentity);
            }

            if (user.Disabled)
            {
                Log.Error("Failed to finalize user account because the user was disabled.");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.UserDisabled);
            }

            if (!user.EmailVerified)
            {
                Log.Error("Failed to finalize user account because the email was unverified.");
                throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.EmailUnverified);
            }

            // Create the account in the database with its new admin key.
            var adminKey = KeyUtil.GenerateAdminKey();

            try
            {
                var account = new AccountEntity
                {
                    UserId = userId,
                    AdminKey = adminKey,
                    Email = user.Email,
                    Created = DateTime.UtcNow
                };

                // ReSharper disable once MethodHasAsyncOverloadWithCancellation
                _accountsContext.Account.Add(account);
                await _accountsContext.SaveChangesAsync(cancellationToken);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Failed to create account in database");
                throw JayneErrors.Platform(PlatformErrorCode.FailedToCreateAccountInDatabase);
            }

            // add the admin key to the key cache so it can be retrieved later
            await _workerKeyCacheService.SetAdminKeyAsync(adminKey, userId);

            if (!preVerified)
            {
                // so when tools next asks, it knows to do the real login.
                await _workerKeyCacheService.SetEmailVerifiedAsync(accountCreationToken);
            }
        }
    }
}
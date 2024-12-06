using System.Threading.Tasks;

namespace Estate.Jayne.Services
{
    public interface IWorkerKeyCacheService
    {
        Task<ulong?> TryGetWorkerIdByUserKeyAsync(string userKey);

        Task<string> TryGetWorkerOwnerUserIdByAdminKeyAsync(string adminKey);
        Task<string> TryGetWorkerOwnerUserIdByAccountCreationTokenOnceAsync(string accountCreationToken);

        Task<bool> TrySetUserKeyAsync(string userKey, ulong workerId);

        Task<bool> TrySetAdminKeyAsync(string adminKey, string workerOwnerUserId);
        
        Task SetUserKeyAsync(string userKey, ulong workerId);

        Task SetAdminKeyAsync(string adminKey, string workerOwnerUserId);

        Task DeleteUserKeyAsync(string userKey);
        Task DeleteAdminKeyAsync(string adminKey);
        Task<uint> SetAccountCreationTokenAsync(string accountCreationToken, string workerOwnerUserId);
        Task SetEmailVerifiedAsync(string accountCreationToken);
        Task<bool> TrySetEmailVerifiedAsync(string accountCreationToken);
        Task<bool> TryGetEmailVerifiedOnceAsync(string accountCreationToken);
        Task<uint> GetMaxAccountsAsync(uint defaultMaxAccounts);
        Task<uint> GetWorkersPerUserAsync(uint defaultWorkersPerUser);
        Task<uint> GetMaxWorkersAsync(uint defaultMaxWorkers);
    }
}
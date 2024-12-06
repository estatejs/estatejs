using System.Threading;
using System.Threading.Tasks;
using Estate.Jayne.ApiModels.Response;

namespace Estate.Jayne.Systems
{
    public interface IDeveloperAccountSystem
    {
        Task<LoginExistingAccountResponse> LoginExistingAccountAsync(CancellationToken cancellationToken, string logContext);
        Task<BeginCreateAccountResponse> BeginCreateAccountAsync(CancellationToken cancellationToken, string logContext, string username, string password);
        Task FinalizeAccountOnceAsync(CancellationToken cancellationToken, string logContext, string accountCreationToken);
        Task PopulateAdminKeyCacheAsync();
        Task PopulateUserKeyCacheAsync();
        Task DeleteAccountAsync(CancellationToken cancellationToken, string logContext);
        Task<bool> IsEmailVerifiedAsync(string accountCreationToken);
    }
}
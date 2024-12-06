using System.Threading;
using System.Threading.Tasks;
using Estate.Jayne.ApiModels.Request;
using Estate.Jayne.ApiModels.Response;

namespace Estate.Jayne.Systems
{
    public interface IWorkerAdminSystem
    {
        Task<DeployWorkerResponse> DeployWorkerAsync(CancellationToken cancellationToken, DeployWorkerRequest request);
        Task<GetWorkerConnectInfoResponse> GetWorkerConnectionInfoAsync(CancellationToken cancellationToken, string logContext, string workerName);
        Task<ListWorkersResponse> ListWorkersAsync(CancellationToken cancellationToken, string logContext);
        Task DeleteWorkerAsync(CancellationToken cancellationToken, DeleteWorkerRequest request);
    }
}
using System.Threading;
using System.Threading.Tasks;

namespace Estate.Jayne.Services
{
    public interface ISerenityService
    {
        Task SetupWorkerAsync(CancellationToken cancellationToken, string logContext, ulong workerId, ulong workerVersion,
            ulong? previousWorkerVersion,
            byte[] workerIndex, string[] code);

        Task DeleteWorkerAsync(CancellationToken cancellationToken, string logContext,
            ulong workerId, ulong workerVersion);
    }
}
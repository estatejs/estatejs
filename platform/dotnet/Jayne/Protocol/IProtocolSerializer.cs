using Estate.Jayne.Models;
using Estate.Jayne.Models.Protocol;

namespace Estate.Jayne.Protocol
{
    public interface IProtocolSerializer
    {
        byte[] SerializeWorkerIndex(WorkerIndexInfo workerIndex);
        byte[] SerializeSetupWorkerRequest(string logContext, ulong workerId, ulong workerVersion, ulong? previousWorkerVersion, byte[] workerIndex, string[] code);
        byte[] SerializeDeleteWorkerRequest(string logContext, ulong workerId, ulong workerVersion);
    }
}
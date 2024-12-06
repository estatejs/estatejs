using FlatBuffers;

namespace Estate.Jayne.Protocol.Impl
{
    public class WorkerIndexProtocolDeserializerImpl : IProtocolDeserializer<WorkerIndexProto>
    {
        public WorkerIndexProto Deserialize(byte[] bytes)
        {
            var buffer = new ByteBuffer(bytes);
            return WorkerIndexProto.GetRootAsWorkerIndexProto(buffer);
        }
    }
}
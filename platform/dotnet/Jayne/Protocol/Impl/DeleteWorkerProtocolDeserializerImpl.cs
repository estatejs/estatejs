using FlatBuffers;

namespace Estate.Jayne.Protocol.Impl
{
    internal class DeleteWorkerProtocolDeserializerImpl : IProtocolDeserializer<DeleteWorkerResponseProto>
    {
        public DeleteWorkerResponseProto Deserialize(byte[] bytes)
        {
            var buffer = new ByteBuffer(bytes);
            return DeleteWorkerResponseProto.GetRootAsDeleteWorkerResponseProto(buffer);
        }
    }
}
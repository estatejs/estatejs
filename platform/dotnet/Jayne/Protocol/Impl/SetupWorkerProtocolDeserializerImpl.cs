using FlatBuffers;

namespace Estate.Jayne.Protocol.Impl
{
    internal class SetupWorkerProtocolDeserializerImpl : IProtocolDeserializer<SetupWorkerResponseProto>
    {
        public SetupWorkerResponseProto Deserialize(byte[] bytes)
        {
            var buffer = new ByteBuffer(bytes);
            return SetupWorkerResponseProto.GetRootAsSetupWorkerResponseProto(buffer);
        }
    }
}
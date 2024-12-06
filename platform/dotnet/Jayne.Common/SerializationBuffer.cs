using System.Collections.Generic;
using FlatBuffers;

namespace Estate.Jayne.Common
{
    public class SerializationBuffer
    {
        private readonly FlatBufferBuilder builder;

        public SerializationBuffer(int initialBufferSize)
        {
            builder = new FlatBufferBuilder(initialBufferSize);
        }

        public byte[] CreateSetupWorkerRequest(string logContext, byte[] workerIndex, IReadOnlyList<string> code, ulong workerId, ulong workerVersion, ulong previousWorkerVersion)
        {
            builder.Clear();

            var logContextStr = builder.CreateString(logContext);

            var codeOffsets = new StringOffset[code.Count];
            int i = 0;
            foreach (var c in code)
                codeOffsets[i++] = builder.CreateString(c);
            
            var workerIndexVector = SetupWorkerRequestProto.CreateWorkerIndexVector(builder, workerIndex);
            var workerCodeVector = SetupWorkerRequestProto.CreateWorkerCodeVector(builder, codeOffsets);

            var off = SetupWorkerRequestProto.CreateSetupWorkerRequestProto(builder, 
                logContextStr,
                workerId,
                workerVersion,
                previousWorkerVersion,
                workerIndexVector, 
                workerCodeVector);

            builder.Finish(off.Value);
            
            return builder.SizedByteArray();
        }
    }
}
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging.Abstractions;
using Newtonsoft.Json;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Protocol.Impl;
using Estate.Jayne.Services.Impl;
using Xunit;

namespace Estate.Jayne.UnitTests
{
    public class SetupWorkerTest
    {
        class SetupWorkerRequest
        {
            public string LogContext;
            public ulong WorkerId;
            public ulong WorkerVersion;
            public ulong? PreviousWorkerVersion;
            public string[] Code;
        } 
        
        [Fact]
        public async Task CanSetupWorkerAsync()
        {
            Log.Init(NullLogger.Instance);
            
            var sut = new SerenityServiceImpl(new ProtocolSerializerImpl(new ProtocolSerializerConfig{InitialBufferSize = 1024}),
                new SetupWorkerProtocolDeserializerImpl(),
                new DeleteWorkerProtocolDeserializerImpl());

            var request = JsonConvert.DeserializeObject<SetupWorkerRequest>(File.ReadAllText("/tmp/jayne/request.json"));
            var workerIndex = File.ReadAllBytes("/tmp/jayne/worker_index.bin");

            await sut.SetupWorkerAsync(CancellationToken.None, request.LogContext, request.WorkerId, request.WorkerVersion, request.PreviousWorkerVersion, workerIndex, request.Code);
        }
    }
}
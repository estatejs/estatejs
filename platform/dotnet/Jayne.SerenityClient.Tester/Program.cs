using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging.Abstractions;
using Newtonsoft.Json;
using Estate.Jayne.Common;
using Estate.Jayne.Common.Error;
using Estate.Jayne.Config;
using Estate.Jayne.Protocol.Impl;
using Estate.Jayne.Services.Impl;

namespace Estate.Jayne.SerenityClient.Tester
{
    class SetupWorkerRequest
    {
        public string LogContext;
        public ulong WorkerId;
        public ulong WorkerVersion;
        public ulong? PreviousWorkerVersion;
        public string[] Code;
    }

    class DeleteWorkerRequest
    {
        public string LogContext;
        public ulong WorkerId;
        public ulong WorkerVersion;
    }
    
    class Program
    {
        static async Task SetupWorkerTest(ulong workerId)
        {
            Log.Init(NullLogger.Instance);

            var testDir = Path.Combine(Environment.CurrentDirectory, "../../../");
            
            var sut = new SerenityServiceImpl(new ProtocolSerializerImpl(new ProtocolSerializerConfig{InitialBufferSize = 1024}),
                new SetupWorkerProtocolDeserializerImpl(),
                new DeleteWorkerProtocolDeserializerImpl());

            var request = JsonConvert.DeserializeObject<SetupWorkerRequest>(File.ReadAllText(Path.Combine(testDir, "setup-request.json")));
            var workerIndex = File.ReadAllBytes(Path.Combine(testDir, "worker_index.bin"));

            await sut.SetupWorkerAsync(CancellationToken.None, request.LogContext, workerId, request.WorkerVersion, request.PreviousWorkerVersion, workerIndex, request.Code);
        }

        static async Task DeleteWorkerTest(ulong workerId)
        {
            Log.Init(NullLogger.Instance);

            var testDir = Path.Combine(Environment.CurrentDirectory, "../../../");
            
            var sut = new SerenityServiceImpl(new ProtocolSerializerImpl(new ProtocolSerializerConfig{InitialBufferSize = 1024}),
                new SetupWorkerProtocolDeserializerImpl(),
                new DeleteWorkerProtocolDeserializerImpl());

            var request = JsonConvert.DeserializeObject<DeleteWorkerRequest>(File.ReadAllText(Path.Combine(testDir, "delete-request.json")));

            await sut.DeleteWorkerAsync(CancellationToken.None, request.LogContext, workerId, request.WorkerVersion);
        }
        
        static async Task Main(string[] args)
        {
            try
            {
                const ulong first = 55;
                ulong workerId = first;
                await SetupWorkerTest(workerId);
                await SetupWorkerTest(++workerId);
                await SetupWorkerTest(++workerId);
                await SetupWorkerTest(++workerId);
                await SetupWorkerTest(++workerId);
                workerId = first;
                await DeleteWorkerTest(workerId);
                await DeleteWorkerTest(++workerId);
                await DeleteWorkerTest(++workerId);
                await DeleteWorkerTest(++workerId);
                await DeleteWorkerTest(++workerId);
            }
            catch (EstateNativeCodeException e)
            {
                Console.Error.WriteLine("<ERROR> Serenity returned error code: " + ((CodeError)e.GetError()).error);
            }
        }
    }
}
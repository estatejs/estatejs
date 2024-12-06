using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Estate.Jayne.Common;
using Estate.Jayne.Config;
using Estate.Jayne.Errors;
using Estate.Jayne.Protocol;
using Estate.Jayne.SerenityClient;

namespace Estate.Jayne.Services.Impl
{
    internal class SerenityServiceImpl : ISerenityService
    {
        private readonly IProtocolDeserializer<SetupWorkerResponseProto> _setupWorkerProtocolDeserializer;
        private readonly IProtocolDeserializer<DeleteWorkerResponseProto> _deleteWorkerProtocolDeserializer;
        private readonly IProtocolSerializer _protocolSerializer;
        private readonly SerenityNativeClient _client;

        public SerenityServiceImpl(IProtocolSerializer protocolSerializer,
            IProtocolDeserializer<SetupWorkerResponseProto> setupWorkerProtocolDeserializer,
            IProtocolDeserializer<DeleteWorkerResponseProto> deleteWorkerProtocolDeserializer)
        {
            Requires.NotDefault(nameof(protocolSerializer), protocolSerializer);
            Requires.NotDefault(nameof(setupWorkerProtocolDeserializer), setupWorkerProtocolDeserializer);
            Requires.NotDefault(nameof(deleteWorkerProtocolDeserializer), deleteWorkerProtocolDeserializer);

            _protocolSerializer = protocolSerializer;
            _setupWorkerProtocolDeserializer = setupWorkerProtocolDeserializer;
            _deleteWorkerProtocolDeserializer = deleteWorkerProtocolDeserializer;

            //init the native client
            var nativeLibDir = Environment.GetEnvironmentVariable("ESTATE_SERENITY_CLIENT_DIR");
            if (string.IsNullOrWhiteSpace(nativeLibDir))
                throw new Exception("Missing ESTATE_SERENITY_CLIENT_DIR environment variable value");

            _client = new SerenityNativeClient(nativeLibDir);

            var configFile = Environment.GetEnvironmentVariable("ESTATE_SERENITY_CLIENT_CONFIG_FILE")!;
            if (string.IsNullOrWhiteSpace(configFile))
                throw new Exception("Missing ESTATE_SERENITY_CLIENT_CONFIG_FILE environment variable value");

            _client.Init(configFile);
        }

        public async Task SetupWorkerAsync(CancellationToken cancellationToken, string logContext,
            ulong workerId,
            ulong workerVersion, ulong? previousWorkerVersion, byte[] workerIndex, string[] code)
        {
            var payload = _protocolSerializer.SerializeSetupWorkerRequest(logContext, workerId, workerVersion,
                previousWorkerVersion, workerIndex, code);

            var response = await TrySendRequestAsync(cancellationToken, logContext, workerId, payload,
                _setupWorkerProtocolDeserializer,
                _client.SendSetupWorkerRequest);

            switch (response.ErrorType)
            {
                case SetupWorkerErrorUnionProto.NONE:
                    //OK
                    break;
                case SetupWorkerErrorUnionProto.ErrorCodeResponseProto:
                {
                    throw new EstateNativeCodeException(response.ErrorAsErrorCodeResponseProto().ErrorCode);
                }
                case SetupWorkerErrorUnionProto.ExceptionResponseProto:
                {
                    var ex = response.ErrorAsExceptionResponseProto();
                    throw new EstateNativeCodeScriptException(ex.Message, ex.Stack);
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public async Task DeleteWorkerAsync(CancellationToken cancellationToken,
            string logContext, ulong workerId,
            ulong workerVersion)
        {
            var payload = _protocolSerializer.SerializeDeleteWorkerRequest(logContext, workerId, workerVersion);

            var response = await TrySendRequestAsync(cancellationToken, logContext, workerId, payload,
                _deleteWorkerProtocolDeserializer,
                _client.SendDeleteWorkerRequest);

            if (response.Error.HasValue)
                throw new EstateNativeCodeException(response.Error.Value.ErrorCode);
        }

        private static byte[] CopyToBytes(UIntPtr bytesUPtr, uint sizeUInt)
        {
            IntPtr bytesPtr = unchecked((IntPtr) (long) (ulong) bytesUPtr);
            int size = unchecked((int) sizeUInt);
            byte[] bytes = new byte[size];
            Marshal.Copy(bytesPtr, bytes, 0, size);
            return bytes;
        }

        private async Task<TProto> TrySendRequestAsync<TProto>(CancellationToken cancellationToken,
            string logContext,
            ulong workerId,
            byte[] payload,
            IProtocolDeserializer<TProto> protocolDeserializer,
            SerenityNativeClient.SendRequestDelegate sendRequestDelegate)
            where TProto : struct
        {
            var tcs = new TaskCompletionSource<TProto>();

            void OnResponse(ushort code, UIntPtr bytesUPtr, ulong sizeUInt)
            {
                try
                {
                    if (!SerenityNativeCode.IsOk(code))
                    {
                        throw new EstateNativeCodeException(code);
                    }

                    cancellationToken.ThrowIfCancellationRequested();
                    var bytes = CopyToBytes(bytesUPtr, (uint) sizeUInt);
                    var response = protocolDeserializer.Deserialize(bytes);
                    tcs.SetResult(response);
                }
                catch (OperationCanceledException)
                {
                    tcs.TrySetCanceled();
                }
                catch (Exception e)
                {
                    if (!tcs.TrySetException(e))
                    {
                        Log.Critical("An underlying exception occurred when making a native call but it couldn't be set on the task completion source");
                        throw;
                    }
                }
            }

            unsafe
            {
                fixed (byte* payloadP = payload)
                {
                    try
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                        sendRequestDelegate(logContext, workerId, (UIntPtr) payloadP, (uint) payload.Length, OnResponse);
                    }
                    catch (OperationCanceledException)
                    {
                        tcs.TrySetCanceled();
                    }
                    catch (Exception e)
                    {
                        if (!tcs.TrySetException(e))
                        {
                            Log.Critical(
                                "An underlying exception occurred when making a native call but it couldn't be set on the task completion source");
                            throw;
                        }
                    }
                }
            }

            return await tcs.Task;
        }
    }
}
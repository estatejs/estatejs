using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Extensions.Logging;
using Estate.Jayne.Common;

// ReSharper disable InconsistentNaming
// ReSharper disable FieldCanBeMadeReadOnly.Global
// ReSharper disable MemberCanBePrivate.Global
// ReSharper disable BuiltInTypeReferenceStyle

namespace Estate.Jayne.SerenityClient
{
    public class SerenityNativeClient : IDisposable
    {
        private const string SerenityClientLibraryName = "libestate-serenity-client.so";

        private readonly IntPtr _clientLibHandle;

        public delegate void InitDelegate(string config_file);
        public delegate void OnResponseCallbackDelegate(UInt16 code, UIntPtr response_bytes, UInt64 response_size);
        public delegate void SendRequestDelegate(string log_context, UInt64 worker_id, UIntPtr request_bytes, UInt32 request_size, OnResponseCallbackDelegate on_response);

        public readonly InitDelegate Init;
        public readonly SendRequestDelegate SendSetupWorkerRequest;
        public readonly SendRequestDelegate SendDeleteWorkerRequest;

        public SerenityNativeClient(string libDirectory)
        {
            Requires.NotNullOrWhitespace(nameof(libDirectory), libDirectory);
            if (!Directory.Exists(libDirectory))
                throw new DirectoryNotFoundException($"The lib directory {libDirectory} does not exist");
            string path = Path.Combine(libDirectory, SerenityClientLibraryName);
            if (!File.Exists(path))
                throw new FileNotFoundException($"The native library file {path} does not exist");

            _clientLibHandle = NativeLibrary.Load(path);

            Bind(_clientLibHandle, "init", ref Init);
            Bind(_clientLibHandle, "send_setup_worker_request", ref SendSetupWorkerRequest);
            Bind(_clientLibHandle, "send_delete_worker_request", ref SendDeleteWorkerRequest);
        }

        private static void Bind<TFunc>(IntPtr libHandle, string functionName, ref TFunc field)
        {
            if (!NativeLibrary.TryGetExport(libHandle, functionName, out IntPtr handle))
                throw new NativeBindingException($"failed to get export {functionName}");

            field = Marshal.GetDelegateForFunctionPointer<TFunc>(handle);
        }

        private void ReleaseUnmanagedResources()
        {
            NativeLibrary.Free(_clientLibHandle);
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        ~SerenityNativeClient()
        {
            ReleaseUnmanagedResources();
        }
    }
}
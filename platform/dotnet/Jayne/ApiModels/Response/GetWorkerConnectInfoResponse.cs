// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

namespace Estate.Jayne.ApiModels.Response
{
    public class GetWorkerConnectInfoResponse : BaseResponse
    {
        public string workerIndexStr { get; set; }
        public string userKey { get; set; }
        public string compressedWorkerTypeDefinitionsStr { get; set; }
    }
}
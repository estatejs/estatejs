// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

namespace Estate.Jayne.ApiModels.Response
{
    public class DeployWorkerResponse : BaseResponse
    {
        public string workerIdStr { get; set; }
        public string workerVersionStr { get; set; }
        public WorkerClassMapping[] workerClassMappings { get; set; }
    }
}
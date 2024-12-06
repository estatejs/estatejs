// ReSharper disable InconsistentNaming

using System.ComponentModel.DataAnnotations;

#pragma warning disable 8618
namespace Estate.Jayne.ApiModels.Request
{
    public class DeleteWorkerRequest : BaseRequest
    {
        [Required]
        public string workerName { get; set; }
    }
}
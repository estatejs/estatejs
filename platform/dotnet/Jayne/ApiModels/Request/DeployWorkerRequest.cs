// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

using System.ComponentModel.DataAnnotations;
using Estate.Jayne.Common;
using Estate.Jayne.Models;

namespace Estate.Jayne.ApiModels.Request
{
    public class DeployWorkerRequest : BaseRequest
    {
        public string workerIdStr { get; set; }
        public string currentWorkerVersionStr { get; set; }
        
        [Required]
        [StringLength(maximumLength: 50, MinimumLength = 3)]
        public string workerName { get; set; }
        
        [Required]
        [EnumDataType(typeof(WorkerLanguage))]
        public WorkerLanguage language { get; set; }
        
        [Required]
        [StringLength(maximumLength: 10 * 1024)]
        public string compressedWorkerTypeDefinitionsStr { get; set; }

        [Required]
        [EnsureMinimumElements(1, ErrorMessage = "At least one file is required")]
        public WorkerFileContent[] workerFiles { get; set; }
        
        public WorkerClassMapping[] workerClassMappings { get; set; }
        public ushort? lastClassId { get; set; }
    }
}
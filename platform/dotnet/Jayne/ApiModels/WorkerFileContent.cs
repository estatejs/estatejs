// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

using System.ComponentModel.DataAnnotations;

namespace Estate.Jayne.ApiModels
{
    public class WorkerFileContent
    {
        [Required]
        [StringLength(102400)] //100kb
        public string code { get; set; }
        
        [Required]
        [StringLength(100)]
        public string name { get; set; }
    }
}
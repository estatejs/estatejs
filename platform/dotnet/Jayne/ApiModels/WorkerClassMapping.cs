// ReSharper disable InconsistentNaming

using System.ComponentModel.DataAnnotations;

namespace Estate.Jayne.ApiModels
{
    public class WorkerClassMapping
    {
        [Required]
        public string className { get; set; }
        [Required(ErrorMessage = "invalid classId")]
        public ushort classId { get; set; }
    }
}
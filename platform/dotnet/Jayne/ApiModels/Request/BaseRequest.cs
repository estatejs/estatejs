// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

using System.ComponentModel.DataAnnotations;

namespace Estate.Jayne.ApiModels.Request
{
    public abstract class BaseRequest
    {
        [Required]
        //NOTE: This value must match what's defined in Estate C++ @ lib/runtime/include/estate/runtime/limits.h
        [StringLength(10)]
        public string logContext { get; set; }
    }
}
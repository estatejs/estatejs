using System.ComponentModel.DataAnnotations;

// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

namespace Estate.Jayne.ApiModels.Response
{
    public abstract class BaseResponse
    {
        [Required]
        //NOTE: This value must match what's defined in Estate C++ @ lib/runtime/include/estate/runtime/limits.h
        [StringLength(10)]
        public string logContext { get; set; }
    }
}
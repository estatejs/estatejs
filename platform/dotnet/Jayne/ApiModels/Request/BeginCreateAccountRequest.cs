// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

using System.ComponentModel.DataAnnotations;

namespace Estate.Jayne.ApiModels.Request
{
    public class BeginCreateAccountRequest : BaseRequest
    {
        [Required]
        [MinLength(5)] //a@a.c
        public string username { get; set; }
        [Required]
        [MinLength(8)]
        public string password { get; set; }
    }
}
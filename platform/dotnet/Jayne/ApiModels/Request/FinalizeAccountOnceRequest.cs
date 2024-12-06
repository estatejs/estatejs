using System.ComponentModel.DataAnnotations;

namespace Estate.Jayne.ApiModels.Request
{
    public class FinalizeAccountOnceRequest : BaseRequest
    {
        [Required]
        [MinLength(256)]
        public string accountCreationToken { get; set; }
    }
}
// ReSharper disable InconsistentNaming
// ReSharper disable UnusedAutoPropertyAccessor.Global
#pragma warning disable 8618

namespace Estate.Jayne.ApiModels.Response
{
    public class BeginCreateAccountResponse : BaseResponse
    {
        public bool emailVerified { get; set; }
        public string accountCreationToken { get; set; }
        public uint expiresInSeconds { get; set; }
    }
}
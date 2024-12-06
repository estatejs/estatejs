using Estate.Jayne.Common.Error;
using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.SerenityClient
{
    public class EstateNativeCodeException : EstateException
    {
        private readonly ushort _code;
        
        public EstateNativeCodeException(ushort code)
        {
            _code = code;
        }

        protected override ExceptionCategory Category { get; } = ExceptionCategory.External;
        public override IError GetError()
        {
            return ErrorFactory.CreateCodeError(Category, _code.ToString());
        }
    }
}
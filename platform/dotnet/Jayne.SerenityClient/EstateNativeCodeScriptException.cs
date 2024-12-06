using Estate.Jayne.Common.Error;
using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.SerenityClient
{
    public class EstateNativeCodeScriptException : EstateException
    {
        private readonly string _message;
        private readonly string _stack;

        public EstateNativeCodeScriptException(string message, string stack)
        {
            _message = message;
            _stack = stack;
        }
        
        protected override ExceptionCategory Category { get; } = ExceptionCategory.External;
        
        public override IError GetError()
        {
            return ErrorFactory.CreateScriptExceptionError(Category, _message, _stack);
        }
    }
}
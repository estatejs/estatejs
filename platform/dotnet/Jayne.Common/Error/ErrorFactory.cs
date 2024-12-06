using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.Common.Error
{
    public static class ErrorFactory
    {
        public static IError CreateCodeError(ExceptionCategory category, string error)
        {
            return new CodeError(category.ToString(), error);
        }

        public static IError CreateScriptExceptionError(ExceptionCategory category, string message, string stack)
        {
            return new ScriptExceptionError(category.ToString(), message, stack);
        }
    }
}
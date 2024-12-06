using System;
using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.Errors
{
    public static class JayneErrors
    {
        public static ConfigException Config(string message, Exception inner = null)
        {
            return new(message, inner);
        }
        
        public static PlatformException<PlatformErrorCode> Platform(PlatformErrorCode code, Exception inner = null)
        {
            return new(code, inner);
        }
        
        public static BusinessLogicException<BusinessLogicErrorCode> BusinessLogic(BusinessLogicErrorCode code, Exception inner = null)
        {
            return new(code, inner);
        }
    }
}
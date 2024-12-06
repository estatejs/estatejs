using System;
using Newtonsoft.Json;
using Estate.Jayne.Common.Error;

namespace Estate.Jayne.Common.Exceptions
{
    /// <summary>
    /// Thrown when business logic errors occur.
    /// </summary>
    [Serializable]
    public class BusinessLogicException<C> : EstateException
        where C: Enum
    {
        public C Code { get; }
        
        

        protected override ExceptionCategory Category => ExceptionCategory.BusinessLogic;
        
        public override IError GetError()
        {
            return ErrorFactory.CreateCodeError(Category, Code.ToString());
        }
        
        public BusinessLogicException(C code)
        {
            Code = code;
        }

        public BusinessLogicException(C code, Exception inner) : base(string.Empty, inner)
        {
            Code = code;
        }
    }
}
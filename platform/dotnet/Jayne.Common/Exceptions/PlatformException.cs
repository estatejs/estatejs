using System;
using Estate.Jayne.Common.Error;

namespace Estate.Jayne.Common.Exceptions
{
    /// <summary>
    /// Thrown when a non-business logic error occurs.
    /// </summary>
    [Serializable]
    public class PlatformException<C> : EstateException
        where C: Enum
    {
        public C Code { get; }

        protected override ExceptionCategory Category => ExceptionCategory.Platform;
        public override IError GetError()
        {
            return ErrorFactory.CreateCodeError(Category, Code.ToString());
        }

        public PlatformException(C code) : base(string.Empty)
        {
            Code = code;
        }
        public PlatformException(C code, Exception inner) : base(string.Empty, inner)
        {
            Code = code;
        }
    }
}
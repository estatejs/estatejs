using System;
using Estate.Jayne.Common.Error;

namespace Estate.Jayne.Common.Exceptions
{
    [Serializable]
    public abstract class EstateException : Exception
    {
        protected abstract ExceptionCategory Category { get; }
        
        protected EstateException()
        {}

        protected EstateException(string message) : base(message)
        {}

        protected EstateException(string message, Exception inner) : base(message, inner)
        {}
        
        public abstract IError GetError();
    }
}
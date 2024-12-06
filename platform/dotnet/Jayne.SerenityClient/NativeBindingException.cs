using System;
using System.Runtime.Serialization;

namespace Estate.Jayne.SerenityClient
{
    [Serializable]
    public class NativeBindingException : Exception
    {
        public NativeBindingException()
        {
        }

        public NativeBindingException(string message) : base(message)
        {
        }

        public NativeBindingException(string message, Exception inner) : base(message, inner)
        {
        }

        protected NativeBindingException(
            SerializationInfo info,
            StreamingContext context) : base(info, context)
        {
        }
    }
}
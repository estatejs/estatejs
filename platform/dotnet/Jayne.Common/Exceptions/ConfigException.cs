using System;

namespace Estate.Jayne.Common.Exceptions
{
    [Serializable]
    public class ConfigException : Exception
    {
        public ConfigException(string message) : base(message)
        {
        }

        public ConfigException(string message, Exception inner) : base(message, inner)
        {
        }
    }
}
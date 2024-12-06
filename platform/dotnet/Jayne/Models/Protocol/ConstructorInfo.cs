using System.Collections.Generic;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct ConstructorInfo
    {
        public IEnumerable<MethodArgumentInfo> Arguments { get; }

        public ConstructorInfo(IEnumerable<MethodArgumentInfo> arguments)
        {
            Arguments = arguments;
        }
    }
}
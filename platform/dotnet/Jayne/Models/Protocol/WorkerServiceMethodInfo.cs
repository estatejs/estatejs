using System.Collections.Generic;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct ServiceMethodInfo
    {
        public ushort MethodId { get; }
        public string MethodName { get; }
        public string ReturnType { get; }
        public IEnumerable<MethodArgumentInfo> Arguments { get; }

        public ServiceMethodInfo(string methodName, ushort methodId, string returnType, IEnumerable<MethodArgumentInfo> arguments)
        {
            Requires.NotNullOrWhitespace(nameof(methodName), methodName);
            Requires.NotDefault(nameof(methodId), methodId);
            Requires.NotNullOrWhitespace(nameof(returnType), returnType);
            
            MethodName = methodName;
            MethodId = methodId;
            ReturnType = returnType;
            Arguments = arguments;
        }
    }
}
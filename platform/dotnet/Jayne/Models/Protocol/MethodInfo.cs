using System.Collections.Generic;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct MethodInfo
    {
        public string MethodName { get; }
        public string ReturnType { get; }
        public IEnumerable<MethodArgumentInfo> Arguments { get; }
        public MethodKindInfo MethodKind { get; }

        public MethodInfo(string methodName, 
            string returnType, 
            IEnumerable<MethodArgumentInfo> arguments, 
            MethodKindInfo methodKind)
        {
            Requires.NotNullOrWhitespace(nameof(methodName), methodName);
            Requires.NotNullOrWhitespace(nameof(returnType), returnType);
            Requires.NotDefault(nameof(arguments), arguments);
            
            MethodName = methodName;
            ReturnType = returnType;
            Arguments = arguments;
            MethodKind = methodKind;
        }
    }
}
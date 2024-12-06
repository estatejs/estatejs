using System.Collections.Generic;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct FreeFunctionInfo
    {
        public FreeFunctionInfo(string sourceCode, 
            string returnType, 
            string functionName, 
            IEnumerable<MethodArgumentInfo> arguments)
        {
            Requires.NotNullOrWhitespace(nameof(sourceCode), sourceCode);
            Requires.NotNullOrWhitespace(nameof(returnType), returnType);
            Requires.NotNullOrWhitespace(nameof(functionName), functionName);
            // ReSharper disable once PossibleMultipleEnumeration
            Requires.NotDefault(nameof(arguments), arguments);
            SourceCode = sourceCode;
            ReturnType = returnType;
            FunctionName = functionName;
            Arguments = arguments;
        }

        public string SourceCode { get; }
        public string ReturnType { get; }
        public string FunctionName { get; }
        public IEnumerable<MethodArgumentInfo> Arguments { get; }
    }
}
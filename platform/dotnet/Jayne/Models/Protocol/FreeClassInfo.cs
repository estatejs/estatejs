using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct FreeClassInfo : IClassMetadata
    {
        public FreeClassInfo(string sourceCode, 
            ConstructorInfo? ctor, 
            string className, 
            IEnumerable<MethodInfo> methods)
        {
            Requires.NotNullOrWhitespace(nameof(sourceCode), sourceCode);
            Requires.NotNullOrWhitespace(nameof(className), className);
            // ReSharper disable once PossibleMultipleEnumeration
            Requires.NotDefault(nameof(methods), methods);
            SourceCode = sourceCode;
            Ctor = ctor;
            ClassName = className;
            Methods = methods;
        }

        public ConstructorInfo? Ctor { get; }
        public string ClassName { get; }
        public IEnumerable<MethodInfo> Methods { get; }
        public string SourceCode { get; }
    }
}
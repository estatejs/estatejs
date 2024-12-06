using System.Collections.Generic;
using System.Linq;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct DataClassInfo : IClassMetadata
    {
        public string ClassName { get; }
        public IEnumerable<MethodInfo> Methods { get; }
        public ConstructorInfo? Ctor { get; }
        public ushort ClassId { get; }
        public ushort FileNameId { get; }
        public string SourceCode { get; }

        public DataClassInfo(string className, 
            ushort classId,
            ushort fileNameId, 
            string sourceCode,
            IEnumerable<MethodInfo> methods, 
            ConstructorInfo/*[SIC] ctor is required for Data*/  ctor)
        {
            Requires.NotNullOrWhitespace(nameof(className), className);
            Requires.NotDefault(nameof(classId), classId);
            Requires.NotDefault(nameof(fileNameId), fileNameId);
            Requires.NotNullOrWhitespace(nameof(sourceCode), sourceCode);
            // ReSharper disable once PossibleMultipleEnumeration
            Requires.NotDefault(nameof(methods), methods);
            Requires.NotDefault(nameof(ctor), ctor);

            ClassName = className;
            ClassId = classId;
            FileNameId = fileNameId;
            SourceCode = sourceCode;
            Ctor = ctor;
            Methods = methods;
        }
    }
}
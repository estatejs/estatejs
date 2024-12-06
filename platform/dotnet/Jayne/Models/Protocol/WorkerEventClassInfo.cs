using System.Collections.Generic;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct MessageClassInfo : IClassMetadata
    {
        public ConstructorInfo? Ctor { get; }
        public IEnumerable<MethodInfo> Methods { get; }
        public string ClassName { get; }
        public ushort ClassId { get; }
        public ushort FileNameId { get; }
        public string SourceCode { get; }

        public MessageClassInfo(string className, 
            ushort classId, 
            ushort fileNameId,
            string sourceCode, 
            ConstructorInfo? ctor, 
            IEnumerable<MethodInfo> methods)
        {
            Requires.NotNullOrWhitespace(nameof(className), className);
            Requires.NotDefault(nameof(classId), classId);
            Requires.NotDefault(nameof(fileNameId), fileNameId);
            Requires.NotNullOrWhitespace(nameof(sourceCode), sourceCode);
            ClassName = className;
            ClassId = classId;
            FileNameId = fileNameId;
            SourceCode = sourceCode;
            Ctor = ctor;
            Methods = methods;
        }
    }
}
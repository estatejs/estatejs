using System.Collections.Generic;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct ServiceClassInfo
    {
        public string ClassName { get; }
        public ushort ClassId { get; }
        public ushort FileNameId { get; }
        public IEnumerable<ServiceMethodInfo> Methods { get; }

        public ServiceClassInfo(string className, 
            ushort classId, 
            ushort fileNameId,
            IEnumerable<ServiceMethodInfo> methods)
        {
            Requires.NotNullOrWhitespace(nameof(className), className);
            Requires.NotDefault(nameof(classId), classId);
            Requires.NotDefault(nameof(fileNameId), fileNameId);
            // ReSharper disable once PossibleMultipleEnumeration
            Requires.NotDefaultAndAtLeastOne(nameof(methods), methods);
            
            ClassName = className;
            ClassId = classId;
            FileNameId = fileNameId;
            Methods = methods;
        }
    }
}
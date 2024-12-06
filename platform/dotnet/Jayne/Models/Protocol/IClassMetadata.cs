using System.Collections.Generic;

namespace Estate.Jayne.Models.Protocol
{
    public interface IClassMetadata
    {
        string ClassName { get; }
        IEnumerable<MethodInfo> Methods { get; }
        ConstructorInfo? Ctor { get; }
    }
}
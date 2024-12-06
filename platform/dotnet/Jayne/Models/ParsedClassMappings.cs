using System.Collections.Generic;

namespace Estate.Jayne.Models
{
    public readonly struct ParsedClassMappings
    {
        public ParsedClassMappings(Dictionary<string, ushort> mappings)
        {
            Mappings = mappings;
        }

        public Dictionary<string, ushort> Mappings { get; }
    }
}
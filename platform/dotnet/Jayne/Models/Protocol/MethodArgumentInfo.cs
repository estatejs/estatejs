using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct MethodArgumentInfo
    {
        public string Name { get; }
        public string Type { get; }

        public MethodArgumentInfo(string name, string type)
        {
            Requires.NotNullOrWhitespace(nameof(name), name);
            Requires.NotNullOrWhitespace(nameof(type), type);
            Name = name;
            Type = type;
        }
    }
}
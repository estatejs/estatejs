using System;
using System.Text;

namespace Estate.Jayne.Models.PreCompiler
{
    public class ReplacePreCompilerInstruction : IPreCompilerInstruction
    {
        public int Start { get; }
        private readonly int _length;
        private readonly string _replacement;
        
        public ReplacePreCompilerInstruction(int start, int end, string replacement)
        {
            if (start < 0)
                throw new ArgumentException($"{nameof(start)} must be >= 0");
            if (end <= start)
                throw new ArgumentException($"{nameof(end)} must be > {nameof(start)}");
            Start = start;
            _length = end - start;
            _replacement = replacement;
        }
        
        public void Process(StringBuilder code, ref int offset)
        {
            code.Remove(Start + offset, _length);
            code.Insert(Start + offset, _replacement);
            var delta = _replacement.Length - _length;
            offset += delta;
        }
    }
}
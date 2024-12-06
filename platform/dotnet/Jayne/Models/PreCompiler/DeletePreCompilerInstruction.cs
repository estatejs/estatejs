using System;
using System.Text;

namespace Estate.Jayne.Models.PreCompiler
{
    public class DeletePreCompilerInstruction : IPreCompilerInstruction
    {
        public int Start { get; }
        private readonly int _length;

        public DeletePreCompilerInstruction(int start, int end)
        {
            if (start < 0)
                throw new ArgumentException($"{nameof(start)} must be >= 0");
            if (end <= start)
                throw new ArgumentException($"{nameof(end)} must be > {nameof(start)}");
            Start = start;
            _length = end - start;
        }
        
        public void Process(StringBuilder code, ref int offset)
        {
            code.Remove(Start + offset, _length);
            offset -= _length;
        }
    }
}
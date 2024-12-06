using System;
using System.Text;

namespace Estate.Jayne.Models.PreCompiler
{
    public class InsertPreCompilerInstruction : IPreCompilerInstruction
    {
        public int Start { get; }
        private readonly string _what;
        
        public InsertPreCompilerInstruction(int start, string what)
        {
            if (start < 0)
                throw new ArgumentException($"{nameof(start)} must be >= 0");
            Start = start;
            _what = what;
        }
        
        public void Process(StringBuilder code, ref int offset)
        {
            code.Insert(Start + offset, _what);
            offset += _what.Length;
        }
    }
}
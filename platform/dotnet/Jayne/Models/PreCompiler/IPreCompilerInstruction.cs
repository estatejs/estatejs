using System.Text;

namespace Estate.Jayne.Models.PreCompiler
{
    public interface IPreCompilerInstruction
    {
        int Start { get; }
        /// <summary>
        /// Apply the directive's change to the code and update the offset.
        /// This assumes the code string is processed top to bottom.
        /// </summary>
        void Process(StringBuilder code, ref int offset);
    }
}
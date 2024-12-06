using System.Collections.Generic;
using System.Linq;
using System.Text;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.PreCompiler
{
    public class PreCompilerDirective
    {
        private readonly WorkerFileContent _fileContent;
        private readonly IEnumerable<IPreCompilerInstruction> _instructions;
        
        public PreCompilerDirective(WorkerFileContent fileContent, IEnumerable<IPreCompilerInstruction> instructions)
        {
            Requires.NotDefault(nameof(fileContent), fileContent);
            Requires.NotDefault(nameof(instructions), instructions);
            _fileContent = fileContent;
            _instructions = instructions.OrderBy(instruction => instruction.Start);
        }

        public WorkerFileContent PreCompile()
        {
            var code = new StringBuilder(_fileContent.code);
            int offset = 0;

            foreach (var instruction in _instructions)
                instruction.Process(code, ref offset);

            return new WorkerFileContent
            {
                code = code.ToString(),
                name = _fileContent.name
            };
        }
    }
}
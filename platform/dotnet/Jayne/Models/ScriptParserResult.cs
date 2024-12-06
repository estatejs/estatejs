using System.Collections.Generic;
using Estate.Jayne.Common;
using Estate.Jayne.Models.PreCompiler;
using Estate.Jayne.Models.Protocol;

namespace Estate.Jayne.Models
{
    
    public class ScriptParserResult
    {
        public ScriptParserResult(IEnumerable<PreCompilerDirective> preCompilerDirectives, WorkerIndexInfo workerIndex)
        {
            Requires.NotDefault(nameof(preCompilerDirectives), preCompilerDirectives);
            Requires.NotDefault(nameof(workerIndex), workerIndex);
            PreCompilerDirectives = preCompilerDirectives;
            WorkerIndex = workerIndex;
        }

        public IEnumerable<PreCompilerDirective> PreCompilerDirectives { get; }
        public WorkerIndexInfo WorkerIndex { get; }
    }
}
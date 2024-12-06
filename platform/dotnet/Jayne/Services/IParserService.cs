using System.Collections.Generic;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Models;
using Estate.Jayne.Models.Protocol;

namespace Estate.Jayne.Services
{
    internal interface IParserService
    {
        WorkerLanguage Language { get; }
        ScriptParserResult ParseWorkerCode(ulong workerId, ulong version, string workerName, 
            IEnumerable<WorkerFileContent> workerFiles, ParsedClassMappings? classMappings, ushort? lastClassId);
        ParsedClassMappings? ParseClassMappings(WorkerClassMapping[] classMappings);
        WorkerClassMapping[] CreateClassMappings(WorkerIndexInfo workerIndex);
    }
}
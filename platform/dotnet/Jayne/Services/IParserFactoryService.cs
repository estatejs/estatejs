using Estate.Jayne.ApiModels;
using Estate.Jayne.Models;

namespace Estate.Jayne.Services
{
    interface IParserFactoryService
    {
        IParserService GetParser(WorkerLanguage workerLanguage);
    }
}
using System;
using Microsoft.Extensions.DependencyInjection;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Common;
using Estate.Jayne.Errors;
using Estate.Jayne.Models;

namespace Estate.Jayne.Services.Impl
{
    class ParserFactoryServiceImpl : IParserFactoryService
    {
        private readonly IServiceProvider _serviceProvider;

        public ParserFactoryServiceImpl(IServiceProvider serviceProvider)
        {
            _serviceProvider = serviceProvider;
        }
        
        public IParserService GetParser(WorkerLanguage workerLanguage)
        {
            switch (workerLanguage)
            {
                case WorkerLanguage.JavaScript:
                    return _serviceProvider.GetRequiredService<JavaScriptParserServiceImpl>();
                default:
                    Log.Error("Invalid worker language: " + workerLanguage);
                    throw JayneErrors.BusinessLogic(BusinessLogicErrorCode.InvalidWorkerLanguage);
            }
        }
    }
}
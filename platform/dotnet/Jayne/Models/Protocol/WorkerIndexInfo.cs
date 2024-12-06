using System;
using System.Collections.Generic;
using Estate.Jayne.ApiModels;
using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct WorkerIndexInfo
    {
        public ulong WorkerId { get; }
        public ulong WorkerVersion { get; }
        public string WorkerName { get; }
        public WorkerLanguage WorkerLanguage { get; }
        public IEnumerable<WorkerFileNameInfo> FileNames { get; }
        public IEnumerable<FreeFunctionInfo> FreeFunctions { get; }
        public IEnumerable<FreeClassInfo> FreeClasses { get; }
        public IEnumerable<ServiceClassInfo> ServiceClasses { get; }
        public IEnumerable<DataClassInfo> DataClasses { get; }
        public IEnumerable<MessageClassInfo> MessageClasses { get; }

        public WorkerIndexInfo(ulong workerId,
            ulong workerVersion,
            string workerName,
            WorkerLanguage workerLanguage,
            IEnumerable<WorkerFileNameInfo> fileNames,
            IEnumerable<FreeFunctionInfo> freeFunctions,
            IEnumerable<FreeClassInfo> freeClasses,
            IEnumerable<ServiceClassInfo> workerServiceClasses,
            IEnumerable<DataClassInfo> dataClasses,
            IEnumerable<MessageClassInfo> messageClasses)
        {
            Requires.NotDefault(nameof(workerId), workerId);
            Requires.NotDefault(nameof(workerVersion), workerVersion);
            Requires.NotNullOrWhitespace(nameof(workerName), workerName);
            Requires.NotDefaultAndAtLeastOne(nameof(fileNames), fileNames);

            WorkerId = workerId;
            WorkerVersion = workerVersion;
            WorkerName = workerName;
            WorkerLanguage = workerLanguage;
            FileNames = fileNames;
            FreeFunctions = freeFunctions ?? Array.Empty<FreeFunctionInfo>();
            FreeClasses = freeClasses ?? Array.Empty<FreeClassInfo>();
            ServiceClasses = workerServiceClasses ?? Array.Empty<ServiceClassInfo>();
            DataClasses = dataClasses ?? Array.Empty<DataClassInfo>();
            MessageClasses = messageClasses ?? Array.Empty<MessageClassInfo>();
        }
    }
}
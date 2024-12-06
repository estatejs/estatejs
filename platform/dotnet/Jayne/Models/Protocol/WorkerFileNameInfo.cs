using Estate.Jayne.Common;

namespace Estate.Jayne.Models.Protocol
{
    public readonly struct WorkerFileNameInfo
    {
        public ushort FileNameId { get; }
        public string FileName { get; }
        
        public WorkerFileNameInfo(ushort fileNameId, string fileName)
        {
            Requires.NotDefault(nameof(fileNameId), fileNameId);
            Requires.NotDefault(nameof(fileName), fileName);
            FileNameId = fileNameId;
            FileName = fileName;
        }
    }
}
using System;

namespace Estate.Jayne.Config
{
    public class PlatformLimitsConfig
    {
        public uint DefaultMaxAccounts { get; set; }
        public uint DefaultWorkersPerUser { get; set; }
        public uint DefaultMaxWorkers { get; set; }
        public TimeSpan UpdateFrequency { get; set; }
    }
}
namespace Estate.Jayne.Models
{
    public readonly struct PlatformLimits
    {
        public PlatformLimits(uint maxAccounts, uint workersPerUser, uint maxWorkers)
        {
            MaxAccounts = maxAccounts;
            WorkersPerUser = workersPerUser;
            MaxWorkers = maxWorkers;
        }

        public uint MaxAccounts { get; }
        public uint WorkersPerUser { get; }
        public uint MaxWorkers { get; }
    }
}
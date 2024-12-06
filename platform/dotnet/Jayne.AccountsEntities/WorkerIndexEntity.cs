namespace Estate.Jayne.AccountsEntities
{
    public partial class WorkerIndexEntity
    {
        public ulong WorkerId { get; set; }
        public byte[] Index { get; set; }

        public virtual WorkerEntity Worker { get; set; }
    }
}
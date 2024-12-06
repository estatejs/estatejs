namespace Estate.Jayne.AccountsEntities
{
    public class WorkerTypeDefinitionEntity
    {
        public ulong WorkerId { get; set; }
        public byte[] CompressedTypeDefinitions { get; set; }
        public virtual WorkerEntity Worker { get; set; }
    }
}
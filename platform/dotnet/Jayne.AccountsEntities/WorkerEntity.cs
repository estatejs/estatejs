namespace Estate.Jayne.AccountsEntities
{
    public partial class WorkerEntity
    {
        public ulong Id { get; set; }
        public ulong AccountRowId { get; set; }
        public string Name { get; set; }
        public ulong Version { get; set; }
        public string UserKey { get; set; }
        public virtual AccountEntity AccountRow { get; set; }
        public virtual WorkerIndexEntity WorkerIndex { get; set; }
        public virtual WorkerTypeDefinitionEntity WorkerTypeDefinition { get; set; }
    }
}
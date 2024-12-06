using System;
using System.Collections.Generic;

namespace Estate.Jayne.AccountsEntities
{
    public partial class AccountEntity
    {
        public AccountEntity()
        {
            Worker = new HashSet<WorkerEntity>();
        }

        public ulong RowId { get; set; }
        public string UserId { get; set; }
        public string Email { get; set; }
        public string AdminKey { get; set; }
        public DateTime Created { get; set; }
        public DateTime? Deleted { get; set; }

        public virtual ICollection<WorkerEntity> Worker { get; set; }
    }
}

using Microsoft.EntityFrameworkCore;
using MySqlConnector;
using Estate.Jayne.Common;

namespace Estate.Jayne.AccountsEntities
{
    public partial class AccountsContext : DbContext
    {
        public static string WorkerNameUniqueIndexName { get; } = "worker.account_worker_name_unq";

        public static bool WhenDuplicateWorkerName(DbUpdateException dux)
        {
            return WhenDuplicateKeyOnIndex(dux, WorkerNameUniqueIndexName);
        }

        private static bool WhenDuplicateKeyOnIndex(DbUpdateException dux, string indexName)
        {
            Requires.NotDefault(nameof(dux), dux);
            Requires.NotNullOrWhitespace(nameof(indexName), indexName);

            if (dux.InnerException is MySqlException mex)
                return mex.Number == 1062 && mex.Message.EndsWith("for key '" + indexName + "'");

            return false;
        }

        public AccountsContext(DbContextOptions<AccountsContext> options)
            : base(options)
        {
        }

        public virtual DbSet<AccountEntity> Account { get; set; }
        public virtual DbSet<WorkerEntity> Worker { get; set; }
        public virtual DbSet<WorkerIndexEntity> WorkerIndex { get; set; }
        public virtual DbSet<WorkerTypeDefinitionEntity> WorkerTypeDefinition { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.Entity<AccountEntity>(entity =>
            {
                entity.HasKey(e => e.RowId)
                    .HasName("PRIMARY");

                entity.ToTable("account");

                entity.HasIndex(e => e.AdminKey)
                    .HasDatabaseName("account_admin_key_unq")
                    .IsUnique();

                entity.HasIndex(e => e.Email)
                    .HasDatabaseName("account_email_unq")
                    .IsUnique();

                entity.HasIndex(e => e.UserId)
                    .HasDatabaseName("account_user_id_unq")
                    .IsUnique();

                entity.Property(e => e.RowId)
                    .HasColumnName("row_id")
                    .HasColumnType("bigint(20) unsigned");

                entity.Property(e => e.AdminKey)
                    .IsRequired()
                    .HasColumnName("admin_key")
                    .HasColumnType("varchar(256)")
                    .HasCharSet("utf8mb4")
                    .UseCollation("utf8mb4_unicode_ci");

                entity.Property(e => e.Created)
                    .HasColumnName("created")
                    .HasColumnType("datetime");

                entity.Property(e => e.Deleted)
                    .HasColumnName("deleted")
                    .HasColumnType("datetime");

                entity.Property(e => e.Email)
                    .IsRequired()
                    .HasColumnName("email")
                    .HasColumnType("varchar(320)")
                    .HasCharSet("utf8mb4")
                    .UseCollation("utf8mb4_unicode_ci");

                entity.Property(e => e.UserId)
                    .IsRequired()
                    .HasColumnName("user_id")
                    .HasColumnType("varchar(64)")
                    .HasCharSet("utf8mb4")
                    .UseCollation("utf8mb4_unicode_ci");
            });

            modelBuilder.Entity<WorkerEntity>(entity =>
            {
                entity.ToTable("worker");

                entity.HasIndex(e => e.AccountRowId)
                    .HasDatabaseName("fk_account_row_id_idx");

                entity.HasIndex(e => new {e.AccountRowId, e.Name})
                    .HasDatabaseName("account_worker_name_unq")
                    .IsUnique();

                entity.HasIndex(e => e.UserKey)
                    .HasDatabaseName("worker_user_key_unq")
                    .IsUnique();

                entity.Property(e => e.Id)
                    .HasColumnName("id")
                    .HasColumnType("bigint(20) unsigned");

                entity.Property(e => e.AccountRowId)
                    .HasColumnName("account_row_id")
                    .HasColumnType("bigint(20) unsigned");

                entity.Property(e => e.Name)
                    .IsRequired()
                    .HasColumnName("name")
                    .HasColumnType("varchar(50)")
                    .HasCharSet("utf8mb4")
                    .UseCollation("utf8mb4_unicode_ci");

                entity.Property(e => e.UserKey)
                    .IsRequired()
                    .HasColumnName("user_key")
                    .HasColumnType("varchar(256)")
                    .HasCharSet("utf8mb4")
                    .UseCollation("utf8mb4_unicode_ci");

                entity.Property(e => e.Version)
                    .HasColumnName("version")
                    .HasColumnType("bigint(20) unsigned");

                entity.HasOne(d => d.AccountRow)
                    .WithMany(p => p.Worker)
                    .HasForeignKey(d => d.AccountRowId)
                    .HasConstraintName("fk_account_row_id");
            });

            modelBuilder.Entity<WorkerIndexEntity>(entity =>
            {
                entity.HasKey(e => e.WorkerId)
                    .HasName("PRIMARY");

                entity.ToTable("worker_index");

                entity.HasIndex(e => e.WorkerId)
                    .HasDatabaseName("fk_worker_id_idx");

                entity.Property(e => e.WorkerId)
                    .HasColumnName("worker_id")
                    .HasColumnType("bigint(20) unsigned");

                entity.Property(e => e.Index)
                    .IsRequired()
                    .HasColumnName("index")
                    .HasColumnType("blob");

                entity.HasOne(d => d.Worker)
                    .WithOne(p => p.WorkerIndex)
                    .HasForeignKey<WorkerIndexEntity>(d => d.WorkerId)
                    .HasConstraintName("fk_worker_id");
            });

            modelBuilder.Entity<WorkerTypeDefinitionEntity>(entity =>
            {
                entity.HasKey(e => e.WorkerId)
                    .HasName("PRIMARY");

                entity.ToTable("worker_type_definition");

                entity.HasIndex(e => e.WorkerId)
                    .HasDatabaseName("fk_wtd_worker_id_idx");

                entity.Property(e => e.WorkerId)
                    .HasColumnName("worker_id")
                    .HasColumnType("bigint(20) unsigned");

                entity.Property(e => e.CompressedTypeDefinitions)
                    .IsRequired()
                    .HasColumnName("compressed_type_definitions")
                    .HasColumnType("blob");

                entity.HasOne(d => d.Worker)
                    .WithOne(p => p.WorkerTypeDefinition)
                    .HasForeignKey<WorkerTypeDefinitionEntity>(d => d.WorkerId)
                    .HasConstraintName("fk_wtd_worker_id");
            });

            OnModelCreatingPartial(modelBuilder);
        }

        partial void OnModelCreatingPartial(ModelBuilder modelBuilder);
    }
}
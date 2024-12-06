using System.Threading;
using System.Threading.Tasks;
using Estate.Jayne.Models;

namespace Estate.Jayne.Services
{
    public interface IPlatformLimitsService
    {
        Task InitializeAsync();
        Task<PlatformLimits> GetLimitsAsync(CancellationToken cancellationToken);
    }
}
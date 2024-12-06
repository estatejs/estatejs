using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Estate.Jayne.ApiModels.Request;
using Estate.Jayne.Common;
using Estate.Jayne.Filters;
using Estate.Jayne.Systems;

namespace Estate.Jayne.Controllers
{
    [ApiController]
    [Route("tools-worker-admin")]
    [AdminKeyAuthorize]
    public class WorkerAdminController : ControllerBase
    {
        private readonly IWorkerAdminSystem _workerAdminSystem;

        public WorkerAdminController(IWorkerAdminSystem workerAdminSystem)
        {
            Requires.NotDefault(nameof(workerAdminSystem), workerAdminSystem);
            _workerAdminSystem = workerAdminSystem;
        }

        [HttpPost("delete_worker")]
        public async Task<IActionResult> DeleteWorkerAsync(CancellationToken cancellationToken,
            [FromBody] DeleteWorkerRequest request)
        {
            if (!ModelState.IsValid)
                return BadRequest();

            await _workerAdminSystem.DeleteWorkerAsync(cancellationToken, request);

            return Ok();
        }

        [HttpGet("list_workers")]
        public async Task<IActionResult> ListWorkersAsync(CancellationToken cancellationToken,
            [FromQuery] string logContext)
        {
            var response = await _workerAdminSystem.ListWorkersAsync(cancellationToken, logContext);
            return Ok(response);
        }

        [HttpPost("deploy_worker")]
        public async Task<IActionResult> DeployWorkerAsync(CancellationToken cancellationToken,
            [FromBody] DeployWorkerRequest request)
        {
            if (!ModelState.IsValid)
                return BadRequest();

            var response = await _workerAdminSystem.DeployWorkerAsync(cancellationToken, request);

            return Ok(response);
        }

        [HttpGet("get_worker_connection_info/{workerName}")]
        public async Task<IActionResult> GetWorkerConnectionInfoAsync(CancellationToken cancellationToken, string workerName,
            [FromQuery] string logContext)
        {
            if (string.IsNullOrWhiteSpace(logContext) || string.IsNullOrWhiteSpace(workerName))
                return BadRequest();

            var response = await _workerAdminSystem.GetWorkerConnectionInfoAsync(cancellationToken, logContext, workerName);

            return Ok(response);
        }
    }
}
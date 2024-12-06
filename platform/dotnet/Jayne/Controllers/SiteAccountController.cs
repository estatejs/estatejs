using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Estate.Jayne.ApiModels.Request;
using Estate.Jayne.ApiModels.Response;
using Estate.Jayne.Common;
using Estate.Jayne.Systems;

namespace Estate.Jayne.Controllers
{
    [ApiController]
    [Route("site-account")]
    public class SiteController : ControllerBase
    {
        private readonly IDeveloperAccountSystem _developerAccountSystem;

        public SiteController(IDeveloperAccountSystem developerAccountSystem)
        {
            Requires.NotDefault(nameof(developerAccountSystem), developerAccountSystem);
            _developerAccountSystem = developerAccountSystem;
        }
        
        [HttpPost("finalize_account_once")]
        public async Task<IActionResult> FinalizeAccountOnceAsync(CancellationToken cancellationToken, [FromBody] FinalizeAccountOnceRequest request)
        {
            if (!ModelState.IsValid)
                return BadRequest();
            await _developerAccountSystem.FinalizeAccountOnceAsync(cancellationToken, request.logContext, request.accountCreationToken);
            return Ok();
        }
    }
}
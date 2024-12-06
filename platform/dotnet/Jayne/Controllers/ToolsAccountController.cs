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
    [Route("tools-account")]
    public class AccountController : ControllerBase
    {
        private readonly IDeveloperAccountSystem _developerAccountSystem;

        public AccountController(IDeveloperAccountSystem developerAccountSystem)
        {
            Requires.NotDefault(nameof(developerAccountSystem), developerAccountSystem);
            _developerAccountSystem = developerAccountSystem;
        }

        [HttpPost("begin_create_account")]
        public async Task<IActionResult> BeginCreateAccountAsync(CancellationToken cancellationToken, [FromBody] BeginCreateAccountRequest request)
        {
            if (!ModelState.IsValid)
                return BadRequest();                    
            
            var response = await _developerAccountSystem.BeginCreateAccountAsync(cancellationToken, request.logContext, request.username, request.password);
            return Ok(response);
        }

        [HttpGet("is_email_verified")]
        public async Task<IActionResult> IsEmailVerifiedAsync([FromQuery] string accountCreationToken)
        {
            return await _developerAccountSystem.IsEmailVerifiedAsync(accountCreationToken) ? (IActionResult) Ok() : (IActionResult) NotFound();
        }

        [Authorize]
        [HttpDelete("delete_account")]
        public async Task<IActionResult> DeleteAccountAsync(CancellationToken cancellationToken, [FromQuery] string logContext)
        {
            if (string.IsNullOrWhiteSpace(logContext))
                return BadRequest();
            
            await _developerAccountSystem.DeleteAccountAsync(cancellationToken, logContext);
            
            return Ok();
        }
        
        [Authorize]
        [HttpGet("login_existing_account")]
        public async Task<IActionResult> LoginExistingAccountAsync(CancellationToken cancellationToken, [FromQuery] string logContext)
        {
            if (string.IsNullOrWhiteSpace(logContext))
                return BadRequest();
            
            var response = await _developerAccountSystem.LoginExistingAccountAsync(cancellationToken, logContext);
            return Ok(response);
        }
    }
}
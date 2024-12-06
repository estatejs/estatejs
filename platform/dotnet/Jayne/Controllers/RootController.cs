using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Estate.Jayne.Common;

namespace Estate.Jayne.Controllers
{
    [ApiController]
    [Route("/")]
    public class RootController : ControllerBase
    {
        [HttpGet]
        public IActionResult Root()
        {
            return Ok();
        }
    }
}
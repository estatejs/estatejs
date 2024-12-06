using System.Net;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;
using Estate.Jayne.Common;

namespace Estate.Jayne.Middlewares
{
    public class EstateApiVersionMiddleware
    {
        private readonly string _headerName;
        private readonly uint _requiredValue;
        private readonly bool _logError;
        private readonly RequestDelegate _next;
        private readonly ILogger<EstateApiVersionMiddleware> _logger;

        public EstateApiVersionMiddleware(RequestDelegate next, ILogger<EstateApiVersionMiddleware> logger, EstateApiVersionMiddlewareOptions options)
        {
            Requires.NotDefault(nameof(next), next);
            Requires.NotDefault(nameof(logger), logger);
            Requires.NotDefault(nameof(options), options);
            this._next = next;
            this._logger = logger;
            _headerName = options.HeaderName;
            _requiredValue = options.RequiredValue;
            _logError = options.LogError;
        }

        public async Task Invoke(HttpContext context)
        {
            var values = context.Request.Headers[_headerName];
            if (values.Count != 1)
            {
                //didn't supply anything, return Ok but nothing else for Kubernetes/GCP health-check
                context.Response.Clear();
                context.Response.StatusCode = (int) HttpStatusCode.OK;
                if (_logError)
                    _logger.LogError($"Missing required header {_headerName} but returning OK response anyway.");
                return;
            }
            
            if (!int.TryParse(values[0], out var protocolVersion) ||
                protocolVersion != _requiredValue)
            {
                context.Response.Clear();
                context.Response.StatusCode = (int) HttpStatusCode.Conflict;
                if (_logError)
                    _logger.LogError($"Header {_headerName} contained an invalid protocol version.");
                return;
            }
            
            await this._next(context);
        }
    }
}
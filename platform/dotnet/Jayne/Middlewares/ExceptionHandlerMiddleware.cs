using System.Net;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using Estate.Jayne.Common;
using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.Middlewares
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class ExceptionHandlerMiddleware
    {
        private readonly RequestDelegate _next;
        private readonly ILogger<ExceptionHandlerMiddleware> _logger;

        public ExceptionHandlerMiddleware(RequestDelegate next, ILogger<ExceptionHandlerMiddleware> logger)
        {
            Requires.NotDefault(nameof(next), next);
            Requires.NotDefault(nameof(logger), logger);
            _next = next;
            _logger = logger;
        }

        public async Task Invoke(HttpContext context)
        {
            try
            {
                await _next(context);
            }
            catch (EstateException wex)
            {
                if (context.Response.HasStarted)
                {
                    _logger.LogError("The response has already started, the exception middleware will not be executed.");
                    throw;
                }
                
                context.Response.Clear();
                context.Response.StatusCode = (int) HttpStatusCode.BadRequest;
                
                await context.Response.WriteAsync(JsonConvert.SerializeObject(wex.GetError()));
            }
        }
    }
}
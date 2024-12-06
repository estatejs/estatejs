using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;
using Estate.Jayne.Common;

namespace Estate.Jayne.Middlewares
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class LoggerMiddleware
    {
        private readonly ILoggerFactory _loggerFactory;
        private readonly RequestDelegate _next;

        public LoggerMiddleware(RequestDelegate next, ILoggerFactory loggerFactory)
        {
            Requires.NotDefault(nameof(next), next);
            Requires.NotDefault(nameof(loggerFactory), loggerFactory);
            _next = next;
            _loggerFactory = loggerFactory;
        }

        public async Task Invoke(HttpContext context)
        {
            Log.Init(_loggerFactory.CreateLogger("user"));
            await this._next(context);
        }
    }
}
using System;
using System.Security.Principal;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Filters;
using Microsoft.Extensions.DependencyInjection;
using Estate.Jayne.Common;
using Estate.Jayne.Services;

namespace Estate.Jayne.Filters
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method)]
    public class AdminKeyAuthorizeAttribute : Attribute, IAsyncActionFilter
    {
        private const string AdminKeyHeaderName = "X-WorkerAdminKey";

        public async Task OnActionExecutionAsync(ActionExecutingContext context, ActionExecutionDelegate next)
        {
            if (!context.HttpContext.Request.Headers.TryGetValue(AdminKeyHeaderName, out var compAdminKey))
            {
                context.Result = new UnauthorizedResult();
                return;
            }

            var workerKeyCache = context.HttpContext.RequestServices.GetRequiredService<IWorkerKeyCacheService>();
            var ownerUserId = await workerKeyCache.TryGetWorkerOwnerUserIdByAdminKeyAsync(compAdminKey);
            if (ownerUserId == null)
            {
                Log.Error("Unauthorized request using key " + compAdminKey);
                context.Result = new UnauthorizedResult();
                return;
            }
            
            Log.AppendContext("ownerUserId", ownerUserId);
            context.HttpContext.User = new GenericPrincipal(new GenericIdentity(ownerUserId), null);

            await next();
        }
    }
}
using Microsoft.AspNetCore.Http;
using Estate.Jayne.Common;
using Estate.Jayne.Errors;

namespace Estate.Jayne.Util
{
    internal static class HttpContextEx
    {
        public static string GetUserId(this IHttpContextAccessor accessor)
        {
            var userId = accessor.HttpContext?.User?.Identity?.Name;

            if (userId == null)
            {
                Log.Error("Unable to get user id from http context");
                throw JayneErrors.Platform(PlatformErrorCode.UnableToGetUserId);
            }

            return userId;
        }
    }
}
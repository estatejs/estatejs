using System;
using System.Linq;
using System.Security.Claims;
using Estate.Jayne.Common;

namespace Estate.Jayne.Util
{
    public static class ClaimsPrincipalEx
    {
        public static string GetUserId(this ClaimsPrincipal o)
        {
            Requires.NotDefault(nameof(o), o);
            
            if(!o.Identity.IsAuthenticated)
                throw new Exception("Must be authenticated");
            
            return o.Claims.Single(claim => claim.Type == "user_id").Value;
        }
    }
}
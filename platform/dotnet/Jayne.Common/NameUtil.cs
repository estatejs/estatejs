using System;

namespace Estate.Jayne.Common
{
    public static class NameUtil
    {
        public static bool IsValidName(string str, int min, int max)
        {
            if (string.IsNullOrWhiteSpace(str))
                return false;
            
            if (str.Length < min || str.Length > max)
                return false;
            
            foreach (var c in str)
            {
                if (!(Char.IsLetterOrDigit(c) || c == '-' || c == '_'))
                {
                    return false;
                }
            }

            return true;
        }
    }
}
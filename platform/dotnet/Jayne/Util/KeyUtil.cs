using System;
using System.Security.Cryptography;
using System.Text;
using System.Web;

namespace Estate.Jayne.Util
{
    public static class KeyUtil
    {
        private static string GenerateKey(int bytes)
        {
            var tokenData = RandomNumberGenerator.GetBytes(bytes);
            return Convert.ToBase64String(tokenData);
        }

        public static string GenerateUserId()
        {
            return GenerateKey(48); //64 characters
        }

        public static string GenerateAdminKey()
        {
            return GenerateKey(192);
        }

        public static string GenerateUserKey()
        {
            return GenerateKey(192);
        }

        public static string GenerateAccountCreationToken()
        {
            /* This encoding is required because it's used in a URL. */
            var key = new StringBuilder(GenerateKey(192));
            for (var i = 0; i < key.Length; ++i)
            {
                var c = key[i];
                key[i] = c switch
                {
                    '+' => '.',
                    '/' => '_',
                    '=' => '-',
                    _ => key[i]
                };
            }

            return key.ToString();
        }
    }
}
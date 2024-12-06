using System;

namespace Estate.Jayne.Config
{
    public class WorkerKeyCacheServiceConfig
    {
        public string RedisEndpoint { get; set; }
        public TimeSpan AccountCreationTokenTTL { get; set; }
        public TimeSpan EmailVerifiedTTL { get; set; }

        public void Validate()
        {
            if (string.IsNullOrWhiteSpace(RedisEndpoint))
                throw new Exception("Missing " + nameof(RedisEndpoint));
            if (AccountCreationTokenTTL <= TimeSpan.Zero)
                throw new Exception("Missing or invalid " + nameof(AccountCreationTokenTTL));
            if (EmailVerifiedTTL <= TimeSpan.Zero)
                throw new Exception("Missing or invalid " + nameof(EmailVerifiedTTL));
        }
    }
}
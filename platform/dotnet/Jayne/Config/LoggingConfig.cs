using System;
using System.Security;

namespace Estate.Jayne.Config
{
    public enum EstateLogLevel
    {
        trace,
        debug,
        info,
        warn,
        error,
        critical,
        none
    }

    public static class EstateLogLevelEx
    {
        public static Microsoft.Extensions.Logging.LogLevel ToMSLogLevel(this EstateLogLevel level)
        {
            switch (level)
            {
                case EstateLogLevel.trace:
                    return Microsoft.Extensions.Logging.LogLevel.Trace;
                case EstateLogLevel.debug:
                    return Microsoft.Extensions.Logging.LogLevel.Debug;
                case EstateLogLevel.info:
                    return Microsoft.Extensions.Logging.LogLevel.Information;
                case EstateLogLevel.warn:
                    return Microsoft.Extensions.Logging.LogLevel.Warning;
                case EstateLogLevel.error:
                    return Microsoft.Extensions.Logging.LogLevel.Error;
                case EstateLogLevel.critical:
                    return Microsoft.Extensions.Logging.LogLevel.Critical;
                default:
                    return Microsoft.Extensions.Logging.LogLevel.None;
            }
        }
    }
    
    public class LoggingConfig
    {
        public EstateLogLevel MinLevel { get; set; }
        public EstateLogLevel UserLevel { get; set; }
        public EstateLogLevel SystemLevel { get; set; }
        public EstateLogLevel MicrosoftHostingLifetimeLevel { get; set; }
        public EstateLogLevel MicrosoftAspNetCoreLevel { get; set; }
        public EstateLogLevel MicrosoftEntityFrameworkCoreDatabaseCommandLevel { get; set; }
    }
}
using System;
using System.Text;
using System.Threading;
using Microsoft.Extensions.Logging;

namespace Estate.Jayne.Common
{
    public static class Log
    {
        class LogContext
        {
            public ILogger logger;
            public StringBuilder context;

            public LogContext(ILogger logger)
            {
                this.logger = logger;
            }
        }
        
        private static readonly AsyncLocal<LogContext> AsyncLocal = new AsyncLocal<LogContext>();

        public static bool IsInitialized => AsyncLocal.Value != null;

        private static ILogger Logger
        {
            get
            {
                if(!IsInitialized)
                    throw new InvalidOperationException("attempt to access before init");
                
                return AsyncLocal.Value?.logger;
            }
        }

        public static void Init(ILogger logger)
        {
            if(IsInitialized)
                throw new InvalidOperationException("attempt to re-init");
            
            AsyncLocal.Value = new LogContext(logger);
        }
        
        public static void AppendContext(string key, string value)
        {
            if(!IsInitialized)
                throw new InvalidOperationException("attempt to access before init");
            
            var context = AsyncLocal.Value?.context;
            if (context == null)
                AsyncLocal.Value.context = context = new StringBuilder();
            else
                context.Append(", ");
            
            context.Append(key);
            context.Append("=");
            context.Append(value);
        }

        public static void ClearContext()
        {
            AsyncLocal.Value.context = null;
        }

        private static string GetMessageWithContext(string message)
        {
            if (AsyncLocal.Value.context != null)
            {
                return AsyncLocal.Value.context + " " + message;
            }
            
            return message;
        }

        /// <summary>
        /// Returns the currently enabled log level
        /// </summary>
        public static LogLevel GetLogLevel()
        {
            var logger = Logger!;
            if (logger.IsEnabled(LogLevel.Trace))
                return LogLevel.Trace;
            if (logger.IsEnabled(LogLevel.Debug))
                return LogLevel.Debug;
            if (logger.IsEnabled(LogLevel.Information))
                return LogLevel.Information;
            if (logger.IsEnabled(LogLevel.Warning))
                return LogLevel.Warning;
            if (logger.IsEnabled(LogLevel.Error))
                return LogLevel.Error;
            if (logger.IsEnabled(LogLevel.Critical))
                return LogLevel.Critical;
            return LogLevel.None;
        }
        
        /// <summary>
        /// Logs that describe an unrecoverable application or system crash, or a catastrophic failure that requires
        /// immediate attention.
        /// </summary>
        public static void Critical(Exception ex, string message, params object[] args) => Logger.Log(LogLevel.Critical, ex, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that describe an unrecoverable application or system crash, or a catastrophic failure that requires
        /// immediate attention.
        /// </summary>
        public static void Critical(string message, params object[] args) => Logger.Log(LogLevel.Critical, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that highlight when the current flow of execution is stopped due to a failure. These should indicate a
        /// failure in the current activity, not an application-wide failure.
        /// </summary>
        public static void Error(Exception ex, string message, params object[] args) => Logger.Log(LogLevel.Error, ex, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that highlight when the current flow of execution is stopped due to a failure. These should indicate a
        /// failure in the current activity, not an application-wide failure.
        /// </summary>
        public static void Error(string message, params object[] args) => Logger.Log(LogLevel.Error, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that highlight an abnormal or unexpected event in the application flow, but do not otherwise cause the
        /// application execution to stop.
        /// </summary>
        public static void Warning(Exception ex, string message, params object[] args) => Logger.Log(LogLevel.Warning, ex, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that highlight an abnormal or unexpected event in the application flow, but do not otherwise cause the
        /// application execution to stop.
        /// </summary>
        public static void Warning(string message, params object[] args) => Logger.Log(LogLevel.Warning, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that track the general flow of the application. These logs should have long-term value.
        /// </summary>
        public static void Info(string message, params object[] args) => Logger.Log(LogLevel.Information, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that are used for interactive investigation during development.  These logs should primarily contain
        /// information useful for debugging and have no long-term value.
        /// </summary>
        public static void Debug(string message, params object[] args) => Logger.Log(LogLevel.Debug, GetMessageWithContext(message), args);
        /// <summary>
        /// Logs that contain the most detailed messages. These messages may contain sensitive application data.
        /// These messages are disabled by default and should never be enabled in a production environment.
        /// </summary>
        public static void Trace(string message, params object[] args) => Logger.Log(LogLevel.Trace, GetMessageWithContext(message), args);
    }
}
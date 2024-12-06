using System;
using StackExchange.Redis;

namespace Estate.Jayne.Util
{
    public static class RedisUtil
    {
        private static (string, ushort) ParseHostPort(string configEndpoint)
        {
            if (configEndpoint.StartsWith("tcp://"))
                configEndpoint = configEndpoint.Substring(6);
            var pair = configEndpoint.Split(":");
            return (pair[0], ushort.Parse(pair[1]));
        }

        public static ConfigurationOptions ParseConfigurationOptions(string configString)
        {
            if (configString.StartsWith("tcp://"))
            {
                var configOptions = new ConfigurationOptions();
                var (host, port) = ParseHostPort(configString);
                Console.WriteLine("Redis host {0} and port {1} from {2}", host, port, configString);
                configOptions.EndPoints.Add(host, port);
                return configOptions;
            }

            return ConfigurationOptions.Parse(configString);
        }
    }
}
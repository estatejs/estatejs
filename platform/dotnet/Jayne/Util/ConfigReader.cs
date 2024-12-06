using System;
using System.IO;
using Newtonsoft.Json.Linq;
using Estate.Jayne.Common;
using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.Util
{
    public class ConfigReader
    {
        private readonly string _configFile;
        private readonly JObject _jObject;

        public static ConfigReader FromFileInEnvironmentVariable(string envVar)
        {
            var envVal = Environment.GetEnvironmentVariable(envVar);
            if (string.IsNullOrWhiteSpace(envVal))
                throw new Exception($"Missing {envVar} value");

            var json = File.ReadAllText(envVal);
            var jObject = JObject.Parse(json);
            return new ConfigReader(jObject, envVal);
        }

        private ConfigReader(JObject jObject, string configFile)
        {
            Requires.NotDefault(nameof(jObject), jObject);
            Requires.NotNullOrWhitespace(nameof(configFile), configFile);
            _jObject = jObject;
            _configFile = configFile;
        }

        public TConfig ReadConfig<TConfig>()
        {
            var name = typeof(TConfig).Name;
            if (_jObject.ContainsKey(name))
            {
                return _jObject[name].ToObject<TConfig>();
            }

            throw new ConfigException($"Could not find {name} in {_configFile}");
        }
    }
}
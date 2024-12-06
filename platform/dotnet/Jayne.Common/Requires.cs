using System;
using System.Collections.Generic;
using System.Linq;

namespace Estate.Jayne.Common
{
    public static class Requires
    {
        public static void NotDefaultAndAtLeastOne<T>(string name, IEnumerable<T> enumerable)
        {
            // ReSharper disable once PossibleMultipleEnumeration
            NotDefault(name, enumerable);
            // ReSharper disable once PossibleMultipleEnumeration
            if(!enumerable.Any())
                throw new ArgumentException($"{name} was empty");
        }
        
        public static void NotDefault<T>(string name, T value)
        {
            if (EqualityComparer<T>.Default.Equals(value, default))
            {
                throw new ArgumentNullException($"{name} was null");
            }
        }
        
        public static void NotNullOrWhitespace(string name, string value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                throw new ArgumentException($"{name} was null, empty, or comprised completely of whitespace.");
            }
        }
        
        public static void LengthRange(string name, string value, int minLength, int? maxLength = null)
        {
            NotNullOrWhitespace(name, value);
            
            if(value.Length < minLength)
                throw new ArgumentException($"{name} was shorter than {minLength} characters");
            
            if(maxLength.HasValue && value.Length > maxLength.Value)
                throw new ArgumentException($"{name} was longer than {maxLength} characters");
        }
    }
}

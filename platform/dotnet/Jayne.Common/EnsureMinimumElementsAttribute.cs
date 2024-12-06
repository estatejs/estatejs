using System.Collections;
using System.ComponentModel.DataAnnotations;

namespace Estate.Jayne.Common
{
    public class EnsureMinimumElementsAttribute : ValidationAttribute
    {
        private readonly int _minElements;
        public EnsureMinimumElementsAttribute(int minElements)
        {
            this._minElements = minElements;
        }

        public override bool IsValid(object value)
        {
            if (value is IList list)
                return list.Count >= this._minElements;
            
            return false;
        }
    }
}
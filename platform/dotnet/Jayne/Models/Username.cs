using Estate.Jayne.Common;
using Estate.Jayne.Errors;

namespace Estate.Jayne.Models
{
    public readonly struct Username
    {
        public const int MinLength = 3;
        public const int MaxLength = 20;

        public string Value { get; }

        public Username(string value)
        {
            if (!NameUtil.IsValidName(value, MinLength, MaxLength))
                throw Errors.JayneErrors.BusinessLogic(BusinessLogicErrorCode.InvalidUsername);

            Value = value;
        }
    }
}
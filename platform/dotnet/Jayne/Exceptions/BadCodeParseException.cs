using System;
using Newtonsoft.Json;
using Estate.Jayne.Common.Error;
using Estate.Jayne.Common.Exceptions;

namespace Estate.Jayne.Exceptions
{
    [Serializable]
    public class BadCodeParseException : EstateException
    {
        public string FileName { get; }
        public string Error { get; }

        public BadCodeParseException(string fileName, string error) : base($"In file {fileName}: {error}")
        {
            FileName = fileName;
            Error = error;
        }

        protected override ExceptionCategory Category { get; } = ExceptionCategory.CodeParse;

        public override IError GetError()
        {
            return ErrorFactory.CreateCodeError(Category, Message);
        }
    }
}
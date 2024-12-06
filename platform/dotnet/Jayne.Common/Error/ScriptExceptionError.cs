namespace Estate.Jayne.Common.Error
{
    public class ScriptExceptionError : IError
    {
        public ScriptExceptionError(string category, string message, string stack)
        {
            this.category = category;
            this.message = message;
            this.stack = stack;
        }

        //note: this must match what's defined in Tools/model/jayne-error.ts
        public string type { get; } = "script_exception";
        public string category { get; }
        public string message { get; }
        public string stack { get; }
    }
}
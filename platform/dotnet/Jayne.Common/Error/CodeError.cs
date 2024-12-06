namespace Estate.Jayne.Common.Error
{
    public class CodeError : IError
    {
        public CodeError(string category, string error)
        {
            this.category = category;
            this.error = error;
        }

        //note: this must match what's defined in Tools/model/jayne-error.ts
        public string type { get; } = "code";
        public string category { get; }
        public string error { get; }
    }
}
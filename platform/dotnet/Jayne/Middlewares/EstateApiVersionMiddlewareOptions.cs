namespace Estate.Jayne.Middlewares
{
    public class EstateApiVersionMiddlewareOptions
    {
        public string HeaderName { get; set; }
        public uint RequiredValue { get; set; }
        public bool LogError { get; set; }
    }
}
namespace Estate.Jayne.SerenityClient
{
    public static class SerenityNativeCode
    {
        //NOTE: This must match what's in cpp/lib/runtime/include/estate/runtime/code.h
        private const ushort Ok = 1;
        public static bool IsOk(ushort code) => code == Ok;
    }
}
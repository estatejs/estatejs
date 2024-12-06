using System;

namespace Estate.Jayne.ApiModels.Request
{
    public static class DeployWorkerRequestEx
    {
        public static byte[] GetCompressedWorkerTypeDefinitionsOrDefault(this DeployWorkerRequest obj)
        {
            if (string.IsNullOrWhiteSpace(obj.compressedWorkerTypeDefinitionsStr))
                return default;
            var value = Convert.FromBase64String(obj.compressedWorkerTypeDefinitionsStr);
            return value.Length < 1 ? null : value;
        }
    }
}
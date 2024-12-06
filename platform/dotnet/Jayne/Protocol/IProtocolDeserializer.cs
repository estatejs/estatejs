namespace Estate.Jayne.Protocol
{
    internal interface IProtocolDeserializer<out TProto>
    {
        TProto Deserialize(byte[] bytes);
    }
}
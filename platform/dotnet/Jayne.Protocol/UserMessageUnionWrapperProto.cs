// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct UserMessageUnionWrapperProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static UserMessageUnionWrapperProto GetRootAsUserMessageUnionWrapperProto(ByteBuffer _bb) { return GetRootAsUserMessageUnionWrapperProto(_bb, new UserMessageUnionWrapperProto()); }
  public static UserMessageUnionWrapperProto GetRootAsUserMessageUnionWrapperProto(ByteBuffer _bb, UserMessageUnionWrapperProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public UserMessageUnionWrapperProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public UserMessageUnionProto ValueType { get { int o = __p.__offset(4); return o != 0 ? (UserMessageUnionProto)__p.bb.Get(o + __p.bb_pos) : UserMessageUnionProto.NONE; } }
  public TTable? Value<TTable>() where TTable : struct, IFlatbufferObject { int o = __p.__offset(6); return o != 0 ? (TTable?)__p.__union<TTable>(o + __p.bb_pos) : null; }
  public DataUpdateMessageProto ValueAsDataUpdateMessageProto() { return Value<DataUpdateMessageProto>().Value; }
  public MessageMessageProto ValueAsMessageMessageProto() { return Value<MessageMessageProto>().Value; }

  public static Offset<UserMessageUnionWrapperProto> CreateUserMessageUnionWrapperProto(FlatBufferBuilder builder,
      UserMessageUnionProto value_type = UserMessageUnionProto.NONE,
      int valueOffset = 0) {
    builder.StartTable(2);
    UserMessageUnionWrapperProto.AddValue(builder, valueOffset);
    UserMessageUnionWrapperProto.AddValueType(builder, value_type);
    return UserMessageUnionWrapperProto.EndUserMessageUnionWrapperProto(builder);
  }

  public static void StartUserMessageUnionWrapperProto(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddValueType(FlatBufferBuilder builder, UserMessageUnionProto valueType) { builder.AddByte(0, (byte)valueType, 0); }
  public static void AddValue(FlatBufferBuilder builder, int valueOffset) { builder.AddOffset(1, valueOffset, 0); }
  public static Offset<UserMessageUnionWrapperProto> EndUserMessageUnionWrapperProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<UserMessageUnionWrapperProto>(o);
  }
};

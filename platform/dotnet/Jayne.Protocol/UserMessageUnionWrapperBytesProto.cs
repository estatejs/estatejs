// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct UserMessageUnionWrapperBytesProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static UserMessageUnionWrapperBytesProto GetRootAsUserMessageUnionWrapperBytesProto(ByteBuffer _bb) { return GetRootAsUserMessageUnionWrapperBytesProto(_bb, new UserMessageUnionWrapperBytesProto()); }
  public static UserMessageUnionWrapperBytesProto GetRootAsUserMessageUnionWrapperBytesProto(ByteBuffer _bb, UserMessageUnionWrapperBytesProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public UserMessageUnionWrapperBytesProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public byte Bytes(int j) { int o = __p.__offset(4); return o != 0 ? __p.bb.Get(__p.__vector(o) + j * 1) : (byte)0; }
  public int BytesLength { get { int o = __p.__offset(4); return o != 0 ? __p.__vector_len(o) : 0; } }
#if ENABLE_SPAN_T
  public Span<byte> GetBytesBytes() { return __p.__vector_as_span<byte>(4, 1); }
#else
  public ArraySegment<byte>? GetBytesBytes() { return __p.__vector_as_arraysegment(4); }
#endif
  public byte[] GetBytesArray() { return __p.__vector_as_array<byte>(4); }
  public UserMessageUnionWrapperProto? GetBytesAsUserMessageUnionWrapperProto() { int o = __p.__offset(4); return o != 0 ? (UserMessageUnionWrapperProto?)(new UserMessageUnionWrapperProto()).__assign(__p.__indirect(__p.__vector(o)), __p.bb) : null; }

  public static Offset<UserMessageUnionWrapperBytesProto> CreateUserMessageUnionWrapperBytesProto(FlatBufferBuilder builder,
      VectorOffset bytesOffset = default(VectorOffset)) {
    builder.StartTable(1);
    UserMessageUnionWrapperBytesProto.AddBytes(builder, bytesOffset);
    return UserMessageUnionWrapperBytesProto.EndUserMessageUnionWrapperBytesProto(builder);
  }

  public static void StartUserMessageUnionWrapperBytesProto(FlatBufferBuilder builder) { builder.StartTable(1); }
  public static void AddBytes(FlatBufferBuilder builder, VectorOffset bytesOffset) { builder.AddOffset(0, bytesOffset.Value, 0); }
  public static VectorOffset CreateBytesVector(FlatBufferBuilder builder, byte[] data) { builder.StartVector(1, data.Length, 1); for (int i = data.Length - 1; i >= 0; i--) builder.AddByte(data[i]); return builder.EndVector(); }
  public static VectorOffset CreateBytesVectorBlock(FlatBufferBuilder builder, byte[] data) { builder.StartVector(1, data.Length, 1); builder.Add(data); return builder.EndVector(); }
  public static void StartBytesVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(1, numElems, 1); }
  public static Offset<UserMessageUnionWrapperBytesProto> EndUserMessageUnionWrapperBytesProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<UserMessageUnionWrapperBytesProto>(o);
  }
};


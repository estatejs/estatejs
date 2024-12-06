// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct MethodArgumentProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static MethodArgumentProto GetRootAsMethodArgumentProto(ByteBuffer _bb) { return GetRootAsMethodArgumentProto(_bb, new MethodArgumentProto()); }
  public static MethodArgumentProto GetRootAsMethodArgumentProto(ByteBuffer _bb, MethodArgumentProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public MethodArgumentProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public string Name { get { int o = __p.__offset(4); return o != 0 ? __p.__string(o + __p.bb_pos) : null; } }
#if ENABLE_SPAN_T
  public Span<byte> GetNameBytes() { return __p.__vector_as_span<byte>(4, 1); }
#else
  public ArraySegment<byte>? GetNameBytes() { return __p.__vector_as_arraysegment(4); }
#endif
  public byte[] GetNameArray() { return __p.__vector_as_array<byte>(4); }
  public string Type { get { int o = __p.__offset(6); return o != 0 ? __p.__string(o + __p.bb_pos) : null; } }
#if ENABLE_SPAN_T
  public Span<byte> GetTypeBytes() { return __p.__vector_as_span<byte>(6, 1); }
#else
  public ArraySegment<byte>? GetTypeBytes() { return __p.__vector_as_arraysegment(6); }
#endif
  public byte[] GetTypeArray() { return __p.__vector_as_array<byte>(6); }

  public static Offset<MethodArgumentProto> CreateMethodArgumentProto(FlatBufferBuilder builder,
      StringOffset nameOffset = default(StringOffset),
      StringOffset typeOffset = default(StringOffset)) {
    builder.StartTable(2);
    MethodArgumentProto.AddType(builder, typeOffset);
    MethodArgumentProto.AddName(builder, nameOffset);
    return MethodArgumentProto.EndMethodArgumentProto(builder);
  }

  public static void StartMethodArgumentProto(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddName(FlatBufferBuilder builder, StringOffset nameOffset) { builder.AddOffset(0, nameOffset.Value, 0); }
  public static void AddType(FlatBufferBuilder builder, StringOffset typeOffset) { builder.AddOffset(1, typeOffset.Value, 0); }
  public static Offset<MethodArgumentProto> EndMethodArgumentProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    builder.Required(o, 4);  // name
    builder.Required(o, 6);  // type
    return new Offset<MethodArgumentProto>(o);
  }
};


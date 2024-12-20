// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct BooleanValueProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static BooleanValueProto GetRootAsBooleanValueProto(ByteBuffer _bb) { return GetRootAsBooleanValueProto(_bb, new BooleanValueProto()); }
  public static BooleanValueProto GetRootAsBooleanValueProto(ByteBuffer _bb, BooleanValueProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public BooleanValueProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public bool Value { get { int o = __p.__offset(4); return o != 0 ? 0!=__p.bb.Get(o + __p.bb_pos) : (bool)false; } }

  public static Offset<BooleanValueProto> CreateBooleanValueProto(FlatBufferBuilder builder,
      bool value = false) {
    builder.StartTable(1);
    BooleanValueProto.AddValue(builder, value);
    return BooleanValueProto.EndBooleanValueProto(builder);
  }

  public static void StartBooleanValueProto(FlatBufferBuilder builder) { builder.StartTable(1); }
  public static void AddValue(FlatBufferBuilder builder, bool value) { builder.AddBool(0, value, false); }
  public static Offset<BooleanValueProto> EndBooleanValueProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<BooleanValueProto>(o);
  }
};


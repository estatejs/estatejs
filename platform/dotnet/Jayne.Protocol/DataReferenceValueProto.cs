// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct DataReferenceValueProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static DataReferenceValueProto GetRootAsDataReferenceValueProto(ByteBuffer _bb) { return GetRootAsDataReferenceValueProto(_bb, new DataReferenceValueProto()); }
  public static DataReferenceValueProto GetRootAsDataReferenceValueProto(ByteBuffer _bb, DataReferenceValueProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public DataReferenceValueProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public ushort ClassId { get { int o = __p.__offset(4); return o != 0 ? __p.bb.GetUshort(o + __p.bb_pos) : (ushort)0; } }
  public string PrimaryKey { get { int o = __p.__offset(6); return o != 0 ? __p.__string(o + __p.bb_pos) : null; } }
#if ENABLE_SPAN_T
  public Span<byte> GetPrimaryKeyBytes() { return __p.__vector_as_span<byte>(6, 1); }
#else
  public ArraySegment<byte>? GetPrimaryKeyBytes() { return __p.__vector_as_arraysegment(6); }
#endif
  public byte[] GetPrimaryKeyArray() { return __p.__vector_as_array<byte>(6); }

  public static Offset<DataReferenceValueProto> CreateDataReferenceValueProto(FlatBufferBuilder builder,
      ushort class_id = 0,
      StringOffset primary_keyOffset = default(StringOffset)) {
    builder.StartTable(2);
    DataReferenceValueProto.AddPrimaryKey(builder, primary_keyOffset);
    DataReferenceValueProto.AddClassId(builder, class_id);
    return DataReferenceValueProto.EndDataReferenceValueProto(builder);
  }

  public static void StartDataReferenceValueProto(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddClassId(FlatBufferBuilder builder, ushort classId) { builder.AddUshort(0, classId, 0); }
  public static void AddPrimaryKey(FlatBufferBuilder builder, StringOffset primaryKeyOffset) { builder.AddOffset(1, primaryKeyOffset.Value, 0); }
  public static Offset<DataReferenceValueProto> EndDataReferenceValueProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    builder.Required(o, 6);  // primary_key
    return new Offset<DataReferenceValueProto>(o);
  }
};


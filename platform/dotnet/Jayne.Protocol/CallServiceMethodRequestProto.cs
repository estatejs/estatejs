// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct CallServiceMethodRequestProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static CallServiceMethodRequestProto GetRootAsCallServiceMethodRequestProto(ByteBuffer _bb) { return GetRootAsCallServiceMethodRequestProto(_bb, new CallServiceMethodRequestProto()); }
  public static CallServiceMethodRequestProto GetRootAsCallServiceMethodRequestProto(ByteBuffer _bb, CallServiceMethodRequestProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public CallServiceMethodRequestProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public ushort ClassId { get { int o = __p.__offset(4); return o != 0 ? __p.bb.GetUshort(o + __p.bb_pos) : (ushort)0; } }
  public string PrimaryKey { get { int o = __p.__offset(6); return o != 0 ? __p.__string(o + __p.bb_pos) : null; } }
#if ENABLE_SPAN_T
  public Span<byte> GetPrimaryKeyBytes() { return __p.__vector_as_span<byte>(6, 1); }
#else
  public ArraySegment<byte>? GetPrimaryKeyBytes() { return __p.__vector_as_arraysegment(6); }
#endif
  public byte[] GetPrimaryKeyArray() { return __p.__vector_as_array<byte>(6); }
  public ushort MethodId { get { int o = __p.__offset(8); return o != 0 ? __p.bb.GetUshort(o + __p.bb_pos) : (ushort)0; } }
  public ValueProto? Arguments(int j) { int o = __p.__offset(10); return o != 0 ? (ValueProto?)(new ValueProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int ArgumentsLength { get { int o = __p.__offset(10); return o != 0 ? __p.__vector_len(o) : 0; } }
  public InboundDataDeltaProto? ReferencedDataDeltas(int j) { int o = __p.__offset(12); return o != 0 ? (InboundDataDeltaProto?)(new InboundDataDeltaProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int ReferencedDataDeltasLength { get { int o = __p.__offset(12); return o != 0 ? __p.__vector_len(o) : 0; } }

  public static Offset<CallServiceMethodRequestProto> CreateCallServiceMethodRequestProto(FlatBufferBuilder builder,
      ushort class_id = 0,
      StringOffset primary_keyOffset = default(StringOffset),
      ushort method_id = 0,
      VectorOffset argumentsOffset = default(VectorOffset),
      VectorOffset referenced_data_deltasOffset = default(VectorOffset)) {
    builder.StartTable(5);
    CallServiceMethodRequestProto.AddReferencedDataDeltas(builder, referenced_data_deltasOffset);
    CallServiceMethodRequestProto.AddArguments(builder, argumentsOffset);
    CallServiceMethodRequestProto.AddPrimaryKey(builder, primary_keyOffset);
    CallServiceMethodRequestProto.AddMethodId(builder, method_id);
    CallServiceMethodRequestProto.AddClassId(builder, class_id);
    return CallServiceMethodRequestProto.EndCallServiceMethodRequestProto(builder);
  }

  public static void StartCallServiceMethodRequestProto(FlatBufferBuilder builder) { builder.StartTable(5); }
  public static void AddClassId(FlatBufferBuilder builder, ushort classId) { builder.AddUshort(0, classId, 0); }
  public static void AddPrimaryKey(FlatBufferBuilder builder, StringOffset primaryKeyOffset) { builder.AddOffset(1, primaryKeyOffset.Value, 0); }
  public static void AddMethodId(FlatBufferBuilder builder, ushort methodId) { builder.AddUshort(2, methodId, 0); }
  public static void AddArguments(FlatBufferBuilder builder, VectorOffset argumentsOffset) { builder.AddOffset(3, argumentsOffset.Value, 0); }
  public static VectorOffset CreateArgumentsVector(FlatBufferBuilder builder, Offset<ValueProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateArgumentsVectorBlock(FlatBufferBuilder builder, Offset<ValueProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartArgumentsVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static void AddReferencedDataDeltas(FlatBufferBuilder builder, VectorOffset referencedDataDeltasOffset) { builder.AddOffset(4, referencedDataDeltasOffset.Value, 0); }
  public static VectorOffset CreateReferencedDataDeltasVector(FlatBufferBuilder builder, Offset<InboundDataDeltaProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateReferencedDataDeltasVectorBlock(FlatBufferBuilder builder, Offset<InboundDataDeltaProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartReferencedDataDeltasVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static Offset<CallServiceMethodRequestProto> EndCallServiceMethodRequestProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    builder.Required(o, 6);  // primary_key
    return new Offset<CallServiceMethodRequestProto>(o);
  }
};


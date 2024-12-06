// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct MessageProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static MessageProto GetRootAsMessageProto(ByteBuffer _bb) { return GetRootAsMessageProto(_bb, new MessageProto()); }
  public static MessageProto GetRootAsMessageProto(ByteBuffer _bb, MessageProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public MessageProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public ushort ClassId { get { int o = __p.__offset(4); return o != 0 ? __p.bb.GetUshort(o + __p.bb_pos) : (ushort)0; } }
  public WorkerReferenceUnionProto SourceType { get { int o = __p.__offset(6); return o != 0 ? (WorkerReferenceUnionProto)__p.bb.Get(o + __p.bb_pos) : WorkerReferenceUnionProto.NONE; } }
  public TTable? Source<TTable>() where TTable : struct, IFlatbufferObject { int o = __p.__offset(8); return o != 0 ? (TTable?)__p.__union<TTable>(o + __p.bb_pos) : null; }
  public DataReferenceValueProto SourceAsDataReferenceValueProto() { return Source<DataReferenceValueProto>().Value; }
  public ServiceReferenceValueProto SourceAsServiceReferenceValueProto() { return Source<ServiceReferenceValueProto>().Value; }
  public PropertyProto? Properties(int j) { int o = __p.__offset(10); return o != 0 ? (PropertyProto?)(new PropertyProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int PropertiesLength { get { int o = __p.__offset(10); return o != 0 ? __p.__vector_len(o) : 0; } }
  public DataHandleProto? ReferencedDataHandles(int j) { int o = __p.__offset(12); return o != 0 ? (DataHandleProto?)(new DataHandleProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int ReferencedDataHandlesLength { get { int o = __p.__offset(12); return o != 0 ? __p.__vector_len(o) : 0; } }

  public static Offset<MessageProto> CreateMessageProto(FlatBufferBuilder builder,
      ushort class_id = 0,
      WorkerReferenceUnionProto source_type = WorkerReferenceUnionProto.NONE,
      int sourceOffset = 0,
      VectorOffset propertiesOffset = default(VectorOffset),
      VectorOffset referenced_data_handlesOffset = default(VectorOffset)) {
    builder.StartTable(5);
    MessageProto.AddReferencedDataHandles(builder, referenced_data_handlesOffset);
    MessageProto.AddProperties(builder, propertiesOffset);
    MessageProto.AddSource(builder, sourceOffset);
    MessageProto.AddClassId(builder, class_id);
    MessageProto.AddSourceType(builder, source_type);
    return MessageProto.EndMessageProto(builder);
  }

  public static void StartMessageProto(FlatBufferBuilder builder) { builder.StartTable(5); }
  public static void AddClassId(FlatBufferBuilder builder, ushort classId) { builder.AddUshort(0, classId, 0); }
  public static void AddSourceType(FlatBufferBuilder builder, WorkerReferenceUnionProto sourceType) { builder.AddByte(1, (byte)sourceType, 0); }
  public static void AddSource(FlatBufferBuilder builder, int sourceOffset) { builder.AddOffset(2, sourceOffset, 0); }
  public static void AddProperties(FlatBufferBuilder builder, VectorOffset propertiesOffset) { builder.AddOffset(3, propertiesOffset.Value, 0); }
  public static VectorOffset CreatePropertiesVector(FlatBufferBuilder builder, Offset<PropertyProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreatePropertiesVectorBlock(FlatBufferBuilder builder, Offset<PropertyProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartPropertiesVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static void AddReferencedDataHandles(FlatBufferBuilder builder, VectorOffset referencedDataHandlesOffset) { builder.AddOffset(4, referencedDataHandlesOffset.Value, 0); }
  public static VectorOffset CreateReferencedDataHandlesVector(FlatBufferBuilder builder, Offset<DataHandleProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateReferencedDataHandlesVectorBlock(FlatBufferBuilder builder, Offset<DataHandleProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartReferencedDataHandlesVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static Offset<MessageProto> EndMessageProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    builder.Required(o, 8);  // source
    return new Offset<MessageProto>(o);
  }
};


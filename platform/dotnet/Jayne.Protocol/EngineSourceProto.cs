// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct EngineSourceProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static EngineSourceProto GetRootAsEngineSourceProto(ByteBuffer _bb) { return GetRootAsEngineSourceProto(_bb, new EngineSourceProto()); }
  public static EngineSourceProto GetRootAsEngineSourceProto(ByteBuffer _bb, EngineSourceProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public EngineSourceProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public string CodeFiles(int j) { int o = __p.__offset(4); return o != 0 ? __p.__string(__p.__vector(o) + j * 4) : null; }
  public int CodeFilesLength { get { int o = __p.__offset(4); return o != 0 ? __p.__vector_len(o) : 0; } }

  public static Offset<EngineSourceProto> CreateEngineSourceProto(FlatBufferBuilder builder,
      VectorOffset code_filesOffset = default(VectorOffset)) {
    builder.StartTable(1);
    EngineSourceProto.AddCodeFiles(builder, code_filesOffset);
    return EngineSourceProto.EndEngineSourceProto(builder);
  }

  public static void StartEngineSourceProto(FlatBufferBuilder builder) { builder.StartTable(1); }
  public static void AddCodeFiles(FlatBufferBuilder builder, VectorOffset codeFilesOffset) { builder.AddOffset(0, codeFilesOffset.Value, 0); }
  public static VectorOffset CreateCodeFilesVector(FlatBufferBuilder builder, StringOffset[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateCodeFilesVectorBlock(FlatBufferBuilder builder, StringOffset[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartCodeFilesVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static Offset<EngineSourceProto> EndEngineSourceProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<EngineSourceProto>(o);
  }
};

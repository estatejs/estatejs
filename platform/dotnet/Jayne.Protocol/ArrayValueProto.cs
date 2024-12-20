// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct ArrayValueProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static ArrayValueProto GetRootAsArrayValueProto(ByteBuffer _bb) { return GetRootAsArrayValueProto(_bb, new ArrayValueProto()); }
  public static ArrayValueProto GetRootAsArrayValueProto(ByteBuffer _bb, ArrayValueProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public ArrayValueProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public ArrayItemValueProto? Items(int j) { int o = __p.__offset(4); return o != 0 ? (ArrayItemValueProto?)(new ArrayItemValueProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int ItemsLength { get { int o = __p.__offset(4); return o != 0 ? __p.__vector_len(o) : 0; } }

  public static Offset<ArrayValueProto> CreateArrayValueProto(FlatBufferBuilder builder,
      VectorOffset itemsOffset = default(VectorOffset)) {
    builder.StartTable(1);
    ArrayValueProto.AddItems(builder, itemsOffset);
    return ArrayValueProto.EndArrayValueProto(builder);
  }

  public static void StartArrayValueProto(FlatBufferBuilder builder) { builder.StartTable(1); }
  public static void AddItems(FlatBufferBuilder builder, VectorOffset itemsOffset) { builder.AddOffset(0, itemsOffset.Value, 0); }
  public static VectorOffset CreateItemsVector(FlatBufferBuilder builder, Offset<ArrayItemValueProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateItemsVectorBlock(FlatBufferBuilder builder, Offset<ArrayItemValueProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartItemsVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static Offset<ArrayValueProto> EndArrayValueProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<ArrayValueProto>(o);
  }
};


// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct SubscribeDataUpdatesResponseProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static SubscribeDataUpdatesResponseProto GetRootAsSubscribeDataUpdatesResponseProto(ByteBuffer _bb) { return GetRootAsSubscribeDataUpdatesResponseProto(_bb, new SubscribeDataUpdatesResponseProto()); }
  public static SubscribeDataUpdatesResponseProto GetRootAsSubscribeDataUpdatesResponseProto(ByteBuffer _bb, SubscribeDataUpdatesResponseProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public SubscribeDataUpdatesResponseProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }


  public static void StartSubscribeDataUpdatesResponseProto(FlatBufferBuilder builder) { builder.StartTable(0); }
  public static Offset<SubscribeDataUpdatesResponseProto> EndSubscribeDataUpdatesResponseProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<SubscribeDataUpdatesResponseProto>(o);
  }
};


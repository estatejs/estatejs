// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct GetWorkerProcessEndpointRequestProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static GetWorkerProcessEndpointRequestProto GetRootAsGetWorkerProcessEndpointRequestProto(ByteBuffer _bb) { return GetRootAsGetWorkerProcessEndpointRequestProto(_bb, new GetWorkerProcessEndpointRequestProto()); }
  public static GetWorkerProcessEndpointRequestProto GetRootAsGetWorkerProcessEndpointRequestProto(ByteBuffer _bb, GetWorkerProcessEndpointRequestProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public GetWorkerProcessEndpointRequestProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public string LogContext { get { int o = __p.__offset(4); return o != 0 ? __p.__string(o + __p.bb_pos) : null; } }
#if ENABLE_SPAN_T
  public Span<byte> GetLogContextBytes() { return __p.__vector_as_span<byte>(4, 1); }
#else
  public ArraySegment<byte>? GetLogContextBytes() { return __p.__vector_as_arraysegment(4); }
#endif
  public byte[] GetLogContextArray() { return __p.__vector_as_array<byte>(4); }
  public ulong WorkerId { get { int o = __p.__offset(6); return o != 0 ? __p.bb.GetUlong(o + __p.bb_pos) : (ulong)0; } }

  public static Offset<GetWorkerProcessEndpointRequestProto> CreateGetWorkerProcessEndpointRequestProto(FlatBufferBuilder builder,
      StringOffset log_contextOffset = default(StringOffset),
      ulong worker_id = 0) {
    builder.StartTable(2);
    GetWorkerProcessEndpointRequestProto.AddWorkerId(builder, worker_id);
    GetWorkerProcessEndpointRequestProto.AddLogContext(builder, log_contextOffset);
    return GetWorkerProcessEndpointRequestProto.EndGetWorkerProcessEndpointRequestProto(builder);
  }

  public static void StartGetWorkerProcessEndpointRequestProto(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddLogContext(FlatBufferBuilder builder, StringOffset logContextOffset) { builder.AddOffset(0, logContextOffset.Value, 0); }
  public static void AddWorkerId(FlatBufferBuilder builder, ulong workerId) { builder.AddUlong(1, workerId, 0); }
  public static Offset<GetWorkerProcessEndpointRequestProto> EndGetWorkerProcessEndpointRequestProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    builder.Required(o, 4);  // log_context
    return new Offset<GetWorkerProcessEndpointRequestProto>(o);
  }
};


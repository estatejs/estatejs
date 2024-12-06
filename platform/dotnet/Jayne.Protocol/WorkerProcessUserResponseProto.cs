// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct WorkerProcessUserResponseProto : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static WorkerProcessUserResponseProto GetRootAsWorkerProcessUserResponseProto(ByteBuffer _bb) { return GetRootAsWorkerProcessUserResponseProto(_bb, new WorkerProcessUserResponseProto()); }
  public static WorkerProcessUserResponseProto GetRootAsWorkerProcessUserResponseProto(ByteBuffer _bb, WorkerProcessUserResponseProto obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public WorkerProcessUserResponseProto __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public byte Response(int j) { int o = __p.__offset(4); return o != 0 ? __p.bb.Get(__p.__vector(o) + j * 1) : (byte)0; }
  public int ResponseLength { get { int o = __p.__offset(4); return o != 0 ? __p.__vector_len(o) : 0; } }
#if ENABLE_SPAN_T
  public Span<byte> GetResponseBytes() { return __p.__vector_as_span<byte>(4, 1); }
#else
  public ArraySegment<byte>? GetResponseBytes() { return __p.__vector_as_arraysegment(4); }
#endif
  public byte[] GetResponseArray() { return __p.__vector_as_array<byte>(4); }
  public UserResponseUnionWrapperProto? GetResponseAsUserResponseUnionWrapperProto() { int o = __p.__offset(4); return o != 0 ? (UserResponseUnionWrapperProto?)(new UserResponseUnionWrapperProto()).__assign(__p.__indirect(__p.__vector(o)), __p.bb) : null; }
  public DataDeltaBytesProto? Deltas(int j) { int o = __p.__offset(6); return o != 0 ? (DataDeltaBytesProto?)(new DataDeltaBytesProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int DeltasLength { get { int o = __p.__offset(6); return o != 0 ? __p.__vector_len(o) : 0; } }
  public MessageBytesProto? Events(int j) { int o = __p.__offset(8); return o != 0 ? (MessageBytesProto?)(new MessageBytesProto()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int EventsLength { get { int o = __p.__offset(8); return o != 0 ? __p.__vector_len(o) : 0; } }
  public byte ConsoleLog(int j) { int o = __p.__offset(10); return o != 0 ? __p.bb.Get(__p.__vector(o) + j * 1) : (byte)0; }
  public int ConsoleLogLength { get { int o = __p.__offset(10); return o != 0 ? __p.__vector_len(o) : 0; } }
#if ENABLE_SPAN_T
  public Span<byte> GetConsoleLogBytes() { return __p.__vector_as_span<byte>(10, 1); }
#else
  public ArraySegment<byte>? GetConsoleLogBytes() { return __p.__vector_as_arraysegment(10); }
#endif
  public byte[] GetConsoleLogArray() { return __p.__vector_as_array<byte>(10); }
  public ConsoleLogProto? GetConsoleLogAsConsoleLogProto() { int o = __p.__offset(10); return o != 0 ? (ConsoleLogProto?)(new ConsoleLogProto()).__assign(__p.__indirect(__p.__vector(o)), __p.bb) : null; }

  public static Offset<WorkerProcessUserResponseProto> CreateWorkerProcessUserResponseProto(FlatBufferBuilder builder,
      VectorOffset responseOffset = default(VectorOffset),
      VectorOffset deltasOffset = default(VectorOffset),
      VectorOffset eventsOffset = default(VectorOffset),
      VectorOffset console_logOffset = default(VectorOffset)) {
    builder.StartTable(4);
    WorkerProcessUserResponseProto.AddConsoleLog(builder, console_logOffset);
    WorkerProcessUserResponseProto.AddEvents(builder, eventsOffset);
    WorkerProcessUserResponseProto.AddDeltas(builder, deltasOffset);
    WorkerProcessUserResponseProto.AddResponse(builder, responseOffset);
    return WorkerProcessUserResponseProto.EndWorkerProcessUserResponseProto(builder);
  }

  public static void StartWorkerProcessUserResponseProto(FlatBufferBuilder builder) { builder.StartTable(4); }
  public static void AddResponse(FlatBufferBuilder builder, VectorOffset responseOffset) { builder.AddOffset(0, responseOffset.Value, 0); }
  public static VectorOffset CreateResponseVector(FlatBufferBuilder builder, byte[] data) { builder.StartVector(1, data.Length, 1); for (int i = data.Length - 1; i >= 0; i--) builder.AddByte(data[i]); return builder.EndVector(); }
  public static VectorOffset CreateResponseVectorBlock(FlatBufferBuilder builder, byte[] data) { builder.StartVector(1, data.Length, 1); builder.Add(data); return builder.EndVector(); }
  public static void StartResponseVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(1, numElems, 1); }
  public static void AddDeltas(FlatBufferBuilder builder, VectorOffset deltasOffset) { builder.AddOffset(1, deltasOffset.Value, 0); }
  public static VectorOffset CreateDeltasVector(FlatBufferBuilder builder, Offset<DataDeltaBytesProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateDeltasVectorBlock(FlatBufferBuilder builder, Offset<DataDeltaBytesProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartDeltasVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static void AddEvents(FlatBufferBuilder builder, VectorOffset eventsOffset) { builder.AddOffset(2, eventsOffset.Value, 0); }
  public static VectorOffset CreateEventsVector(FlatBufferBuilder builder, Offset<MessageBytesProto>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateEventsVectorBlock(FlatBufferBuilder builder, Offset<MessageBytesProto>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartEventsVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static void AddConsoleLog(FlatBufferBuilder builder, VectorOffset consoleLogOffset) { builder.AddOffset(3, consoleLogOffset.Value, 0); }
  public static VectorOffset CreateConsoleLogVector(FlatBufferBuilder builder, byte[] data) { builder.StartVector(1, data.Length, 1); for (int i = data.Length - 1; i >= 0; i--) builder.AddByte(data[i]); return builder.EndVector(); }
  public static VectorOffset CreateConsoleLogVectorBlock(FlatBufferBuilder builder, byte[] data) { builder.StartVector(1, data.Length, 1); builder.Add(data); return builder.EndVector(); }
  public static void StartConsoleLogVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(1, numElems, 1); }
  public static Offset<WorkerProcessUserResponseProto> EndWorkerProcessUserResponseProto(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<WorkerProcessUserResponseProto>(o);
  }
};

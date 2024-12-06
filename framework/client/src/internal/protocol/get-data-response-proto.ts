// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { NestedDataProto } from './nested-data-proto.js';


export class GetDataResponseProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):GetDataResponseProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsGetDataResponseProto(bb:flatbuffers.ByteBuffer, obj?:GetDataResponseProto):GetDataResponseProto {
  return (obj || new GetDataResponseProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsGetDataResponseProto(bb:flatbuffers.ByteBuffer, obj?:GetDataResponseProto):GetDataResponseProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new GetDataResponseProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

data(index: number, obj?:NestedDataProto):NestedDataProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? (obj || new NestedDataProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

dataLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

static startGetDataResponseProto(builder:flatbuffers.Builder) {
  builder.startObject(1);
}

static addData(builder:flatbuffers.Builder, dataOffset:flatbuffers.Offset) {
  builder.addFieldOffset(0, dataOffset, 0);
}

static createDataVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startDataVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static endGetDataResponseProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createGetDataResponseProto(builder:flatbuffers.Builder, dataOffset:flatbuffers.Offset):flatbuffers.Offset {
  GetDataResponseProto.startGetDataResponseProto(builder);
  GetDataResponseProto.addData(builder, dataOffset);
  return GetDataResponseProto.endGetDataResponseProto(builder);
}
}
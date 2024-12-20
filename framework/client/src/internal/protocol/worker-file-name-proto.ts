// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

export class WorkerFileNameProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):WorkerFileNameProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsWorkerFileNameProto(bb:flatbuffers.ByteBuffer, obj?:WorkerFileNameProto):WorkerFileNameProto {
  return (obj || new WorkerFileNameProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsWorkerFileNameProto(bb:flatbuffers.ByteBuffer, obj?:WorkerFileNameProto):WorkerFileNameProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new WorkerFileNameProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

fileNameId():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.readUint16(this.bb_pos + offset) : 0;
}

fileName():string|null
fileName(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
fileName(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

static startWorkerFileNameProto(builder:flatbuffers.Builder) {
  builder.startObject(2);
}

static addFileNameId(builder:flatbuffers.Builder, fileNameId:number) {
  builder.addFieldInt16(0, fileNameId, 0);
}

static addFileName(builder:flatbuffers.Builder, fileNameOffset:flatbuffers.Offset) {
  builder.addFieldOffset(1, fileNameOffset, 0);
}

static endWorkerFileNameProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  builder.requiredField(offset, 6) // file_name
  return offset;
}

static createWorkerFileNameProto(builder:flatbuffers.Builder, fileNameId:number, fileNameOffset:flatbuffers.Offset):flatbuffers.Offset {
  WorkerFileNameProto.startWorkerFileNameProto(builder);
  WorkerFileNameProto.addFileNameId(builder, fileNameId);
  WorkerFileNameProto.addFileName(builder, fileNameOffset);
  return WorkerFileNameProto.endWorkerFileNameProto(builder);
}
}

// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { DataClassProto } from './data-class-proto.js';
import { FreeClassProto } from './free-class-proto.js';
import { FreeFunctionProto } from './free-function-proto.js';
import { MessageClassProto } from './message-class-proto.js';
import { ServiceClassProto } from './service-class-proto.js';
import { WorkerFileNameProto } from './worker-file-name-proto.js';


export class WorkerIndexProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):WorkerIndexProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsWorkerIndexProto(bb:flatbuffers.ByteBuffer, obj?:WorkerIndexProto):WorkerIndexProto {
  return (obj || new WorkerIndexProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsWorkerIndexProto(bb:flatbuffers.ByteBuffer, obj?:WorkerIndexProto):WorkerIndexProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new WorkerIndexProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

isDebug():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

workerId():flatbuffers.Long {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.readUint64(this.bb_pos + offset) : this.bb!.createLong(0, 0);
}

workerVersion():flatbuffers.Long {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? this.bb!.readUint64(this.bb_pos + offset) : this.bb!.createLong(0, 0);
}

workerName():string|null
workerName(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
workerName(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

fileNames(index: number, obj?:WorkerFileNameProto):WorkerFileNameProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? (obj || new WorkerFileNameProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

fileNamesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

workerLanguage():number {
  const offset = this.bb!.__offset(this.bb_pos, 14);
  return offset ? this.bb!.readInt8(this.bb_pos + offset) : 0;
}

freeFunctions(index: number, obj?:FreeFunctionProto):FreeFunctionProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 16);
  return offset ? (obj || new FreeFunctionProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

freeFunctionsLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 16);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

freeClasses(index: number, obj?:FreeClassProto):FreeClassProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 18);
  return offset ? (obj || new FreeClassProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

freeClassesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 18);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

serviceClasses(index: number, obj?:ServiceClassProto):ServiceClassProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 20);
  return offset ? (obj || new ServiceClassProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

serviceClassesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 20);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

dataClasses(index: number, obj?:DataClassProto):DataClassProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 22);
  return offset ? (obj || new DataClassProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

dataClassesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 22);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

messageClasses(index: number, obj?:MessageClassProto):MessageClassProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 24);
  return offset ? (obj || new MessageClassProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

messageClassesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 24);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

static startWorkerIndexProto(builder:flatbuffers.Builder) {
  builder.startObject(11);
}

static addIsDebug(builder:flatbuffers.Builder, isDebug:boolean) {
  builder.addFieldInt8(0, +isDebug, +false);
}

static addWorkerId(builder:flatbuffers.Builder, workerId:flatbuffers.Long) {
  builder.addFieldInt64(1, workerId, builder.createLong(0, 0));
}

static addWorkerVersion(builder:flatbuffers.Builder, workerVersion:flatbuffers.Long) {
  builder.addFieldInt64(2, workerVersion, builder.createLong(0, 0));
}

static addWorkerName(builder:flatbuffers.Builder, workerNameOffset:flatbuffers.Offset) {
  builder.addFieldOffset(3, workerNameOffset, 0);
}

static addFileNames(builder:flatbuffers.Builder, fileNamesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(4, fileNamesOffset, 0);
}

static createFileNamesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startFileNamesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addWorkerLanguage(builder:flatbuffers.Builder, workerLanguage:number) {
  builder.addFieldInt8(5, workerLanguage, 0);
}

static addFreeFunctions(builder:flatbuffers.Builder, freeFunctionsOffset:flatbuffers.Offset) {
  builder.addFieldOffset(6, freeFunctionsOffset, 0);
}

static createFreeFunctionsVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startFreeFunctionsVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addFreeClasses(builder:flatbuffers.Builder, freeClassesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(7, freeClassesOffset, 0);
}

static createFreeClassesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startFreeClassesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addServiceClasses(builder:flatbuffers.Builder, serviceClassesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(8, serviceClassesOffset, 0);
}

static createServiceClassesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startServiceClassesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addDataClasses(builder:flatbuffers.Builder, dataClassesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(9, dataClassesOffset, 0);
}

static createDataClassesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startDataClassesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addMessageClasses(builder:flatbuffers.Builder, messageClassesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(10, messageClassesOffset, 0);
}

static createMessageClassesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startMessageClassesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static endWorkerIndexProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  builder.requiredField(offset, 10) // worker_name
  builder.requiredField(offset, 12) // file_names
  return offset;
}

static finishWorkerIndexProtoBuffer(builder:flatbuffers.Builder, offset:flatbuffers.Offset) {
  builder.finish(offset);
}

static finishSizePrefixedWorkerIndexProtoBuffer(builder:flatbuffers.Builder, offset:flatbuffers.Offset) {
  builder.finish(offset, undefined, true);
}

static createWorkerIndexProto(builder:flatbuffers.Builder, isDebug:boolean, workerId:flatbuffers.Long, workerVersion:flatbuffers.Long, workerNameOffset:flatbuffers.Offset, fileNamesOffset:flatbuffers.Offset, workerLanguage:number, freeFunctionsOffset:flatbuffers.Offset, freeClassesOffset:flatbuffers.Offset, serviceClassesOffset:flatbuffers.Offset, dataClassesOffset:flatbuffers.Offset, messageClassesOffset:flatbuffers.Offset):flatbuffers.Offset {
  WorkerIndexProto.startWorkerIndexProto(builder);
  WorkerIndexProto.addIsDebug(builder, isDebug);
  WorkerIndexProto.addWorkerId(builder, workerId);
  WorkerIndexProto.addWorkerVersion(builder, workerVersion);
  WorkerIndexProto.addWorkerName(builder, workerNameOffset);
  WorkerIndexProto.addFileNames(builder, fileNamesOffset);
  WorkerIndexProto.addWorkerLanguage(builder, workerLanguage);
  WorkerIndexProto.addFreeFunctions(builder, freeFunctionsOffset);
  WorkerIndexProto.addFreeClasses(builder, freeClassesOffset);
  WorkerIndexProto.addServiceClasses(builder, serviceClassesOffset);
  WorkerIndexProto.addDataClasses(builder, dataClassesOffset);
  WorkerIndexProto.addMessageClasses(builder, messageClassesOffset);
  return WorkerIndexProto.endWorkerIndexProto(builder);
}
}

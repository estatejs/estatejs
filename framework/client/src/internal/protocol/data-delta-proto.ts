// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { NestedPropertyProto } from './nested-property-proto.js';


export class DataDeltaProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):DataDeltaProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsDataDeltaProto(bb:flatbuffers.ByteBuffer, obj?:DataDeltaProto):DataDeltaProto {
  return (obj || new DataDeltaProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsDataDeltaProto(bb:flatbuffers.ByteBuffer, obj?:DataDeltaProto):DataDeltaProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new DataDeltaProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

classId():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.readUint16(this.bb_pos + offset) : 0;
}

objectVersion():flatbuffers.Long {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.readUint64(this.bb_pos + offset) : this.bb!.createLong(0, 0);
}

primaryKey():string|null
primaryKey(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
primaryKey(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

deleted():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

properties(index: number, obj?:NestedPropertyProto):NestedPropertyProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? (obj || new NestedPropertyProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

propertiesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

deletedProperties(index: number):string
deletedProperties(index: number,optionalEncoding:flatbuffers.Encoding):string|Uint8Array
deletedProperties(index: number,optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 14);
  return offset ? this.bb!.__string(this.bb!.__vector(this.bb_pos + offset) + index * 4, optionalEncoding) : null;
}

deletedPropertiesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 14);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

static startDataDeltaProto(builder:flatbuffers.Builder) {
  builder.startObject(6);
}

static addClassId(builder:flatbuffers.Builder, classId:number) {
  builder.addFieldInt16(0, classId, 0);
}

static addObjectVersion(builder:flatbuffers.Builder, objectVersion:flatbuffers.Long) {
  builder.addFieldInt64(1, objectVersion, builder.createLong(0, 0));
}

static addPrimaryKey(builder:flatbuffers.Builder, primaryKeyOffset:flatbuffers.Offset) {
  builder.addFieldOffset(2, primaryKeyOffset, 0);
}

static addDeleted(builder:flatbuffers.Builder, deleted:boolean) {
  builder.addFieldInt8(3, +deleted, +false);
}

static addProperties(builder:flatbuffers.Builder, propertiesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(4, propertiesOffset, 0);
}

static createPropertiesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startPropertiesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addDeletedProperties(builder:flatbuffers.Builder, deletedPropertiesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(5, deletedPropertiesOffset, 0);
}

static createDeletedPropertiesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startDeletedPropertiesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static endDataDeltaProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  builder.requiredField(offset, 8) // primary_key
  return offset;
}

static createDataDeltaProto(builder:flatbuffers.Builder, classId:number, objectVersion:flatbuffers.Long, primaryKeyOffset:flatbuffers.Offset, deleted:boolean, propertiesOffset:flatbuffers.Offset, deletedPropertiesOffset:flatbuffers.Offset):flatbuffers.Offset {
  DataDeltaProto.startDataDeltaProto(builder);
  DataDeltaProto.addClassId(builder, classId);
  DataDeltaProto.addObjectVersion(builder, objectVersion);
  DataDeltaProto.addPrimaryKey(builder, primaryKeyOffset);
  DataDeltaProto.addDeleted(builder, deleted);
  DataDeltaProto.addProperties(builder, propertiesOffset);
  DataDeltaProto.addDeletedProperties(builder, deletedPropertiesOffset);
  return DataDeltaProto.endDataDeltaProto(builder);
}
}

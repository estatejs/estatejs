// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

export class NestedPropertyProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):NestedPropertyProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsNestedPropertyProto(bb:flatbuffers.ByteBuffer, obj?:NestedPropertyProto):NestedPropertyProto {
  return (obj || new NestedPropertyProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsNestedPropertyProto(bb:flatbuffers.ByteBuffer, obj?:NestedPropertyProto):NestedPropertyProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new NestedPropertyProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

name():string|null
name(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
name(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

valueBytes(index: number):number|null {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.readUint8(this.bb!.__vector(this.bb_pos + offset) + index) : 0;
}

valueBytesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

valueBytesArray():Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? new Uint8Array(this.bb!.bytes().buffer, this.bb!.bytes().byteOffset + this.bb!.__vector(this.bb_pos + offset), this.bb!.__vector_len(this.bb_pos + offset)) : null;
}

static startNestedPropertyProto(builder:flatbuffers.Builder) {
  builder.startObject(2);
}

static addName(builder:flatbuffers.Builder, nameOffset:flatbuffers.Offset) {
  builder.addFieldOffset(0, nameOffset, 0);
}

static addValueBytes(builder:flatbuffers.Builder, valueBytesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(1, valueBytesOffset, 0);
}

static createValueBytesVector(builder:flatbuffers.Builder, data:number[]|Uint8Array):flatbuffers.Offset {
  builder.startVector(1, data.length, 1);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addInt8(data[i]!);
  }
  return builder.endVector();
}

static startValueBytesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(1, numElems, 1);
}

static endNestedPropertyProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  builder.requiredField(offset, 4) // name
  return offset;
}

static createNestedPropertyProto(builder:flatbuffers.Builder, nameOffset:flatbuffers.Offset, valueBytesOffset:flatbuffers.Offset):flatbuffers.Offset {
  NestedPropertyProto.startNestedPropertyProto(builder);
  NestedPropertyProto.addName(builder, nameOffset);
  NestedPropertyProto.addValueBytes(builder, valueBytesOffset);
  return NestedPropertyProto.endNestedPropertyProto(builder);
}
}

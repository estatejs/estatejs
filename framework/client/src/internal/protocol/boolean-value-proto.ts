// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

export class BooleanValueProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):BooleanValueProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsBooleanValueProto(bb:flatbuffers.ByteBuffer, obj?:BooleanValueProto):BooleanValueProto {
  return (obj || new BooleanValueProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsBooleanValueProto(bb:flatbuffers.ByteBuffer, obj?:BooleanValueProto):BooleanValueProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new BooleanValueProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

value():boolean {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? !!this.bb!.readInt8(this.bb_pos + offset) : false;
}

static startBooleanValueProto(builder:flatbuffers.Builder) {
  builder.startObject(1);
}

static addValue(builder:flatbuffers.Builder, value:boolean) {
  builder.addFieldInt8(0, +value, +false);
}

static endBooleanValueProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createBooleanValueProto(builder:flatbuffers.Builder, value:boolean):flatbuffers.Offset {
  BooleanValueProto.startBooleanValueProto(builder);
  BooleanValueProto.addValue(builder, value);
  return BooleanValueProto.endBooleanValueProto(builder);
}
}

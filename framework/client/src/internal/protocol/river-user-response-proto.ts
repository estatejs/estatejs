// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { ConsoleLogBytesProto } from './console-log-bytes-proto.js';
import { UserMessageUnionWrapperBytesProto } from './user-message-union-wrapper-bytes-proto.js';
import { UserResponseUnionWrapperBytesProto } from './user-response-union-wrapper-bytes-proto.js';


export class RiverUserResponseProto {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
__init(i:number, bb:flatbuffers.ByteBuffer):RiverUserResponseProto {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsRiverUserResponseProto(bb:flatbuffers.ByteBuffer, obj?:RiverUserResponseProto):RiverUserResponseProto {
  return (obj || new RiverUserResponseProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsRiverUserResponseProto(bb:flatbuffers.ByteBuffer, obj?:RiverUserResponseProto):RiverUserResponseProto {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new RiverUserResponseProto()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

response(obj?:UserResponseUnionWrapperBytesProto):UserResponseUnionWrapperBytesProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? (obj || new UserResponseUnionWrapperBytesProto()).__init(this.bb!.__indirect(this.bb_pos + offset), this.bb!) : null;
}

messages(index: number, obj?:UserMessageUnionWrapperBytesProto):UserMessageUnionWrapperBytesProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? (obj || new UserMessageUnionWrapperBytesProto()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

messagesLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

consoleLog(obj?:ConsoleLogBytesProto):ConsoleLogBytesProto|null {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? (obj || new ConsoleLogBytesProto()).__init(this.bb!.__indirect(this.bb_pos + offset), this.bb!) : null;
}

static startRiverUserResponseProto(builder:flatbuffers.Builder) {
  builder.startObject(3);
}

static addResponse(builder:flatbuffers.Builder, responseOffset:flatbuffers.Offset) {
  builder.addFieldOffset(0, responseOffset, 0);
}

static addMessages(builder:flatbuffers.Builder, messagesOffset:flatbuffers.Offset) {
  builder.addFieldOffset(1, messagesOffset, 0);
}

static createMessagesVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startMessagesVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static addConsoleLog(builder:flatbuffers.Builder, consoleLogOffset:flatbuffers.Offset) {
  builder.addFieldOffset(2, consoleLogOffset, 0);
}

static endRiverUserResponseProto(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

}

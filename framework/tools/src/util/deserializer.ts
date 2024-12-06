import {requiresTruthy} from "./requires";
import {WorkerIndexProto} from "../protocol/worker-index-proto";
import {ByteBuffer} from "flatbuffers";

export function deserializeWorkerIndex(workerIndexStr: string): WorkerIndexProto {
    requiresTruthy('workerIndexStr', workerIndexStr);

    let b = Buffer.from(workerIndexStr, 'base64');
    let c = new ByteBuffer(b);

    return WorkerIndexProto.getRootAsWorkerIndexProto(c);
}
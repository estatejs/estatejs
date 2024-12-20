import {ByteBuffer} from "flatbuffers";

import {requiresTruthy} from "../util/requires.js";
import {Unsigned} from "../util/unsigned.js";
import {openOuterspaceConnectionAsync, OuterspaceClient} from "./outerspace-client.js";
import {logVerbose, logDebug, logError, sysLogError} from "../util/logging.js";
import {RIVER_SERVICE_URL} from "../config.js";
import {WorkerRegistry} from "../util/registry.js";
import {options} from "../options.js";
import {
    createCallServiceMethodRequest,
    createGetDataRequest,
    createSaveDataRequest,
    createSubscribeMessageRequest,
    createSubscribeDataUpdatesRequest,
    createUnsubscribeMessageRequest,
    createUnsubscribeDataUpdatesRequest,
    readResponse
} from "../util/serde.js";
import {UserResponseUnionProto} from "../protocol/user-response-union-proto.js";
import {CallServiceMethodResponseProto} from "../protocol/call-service-method-response-proto.js";
import {MessageMessageProto} from "../protocol/message-message-proto.js";
import {UnsubscribeDataUpdatesResponseProto} from "../protocol/unsubscribe-data-updates-response-proto.js";
import {UnsubscribeMessageResponseProto} from "../protocol/unsubscribe-message-response-proto.js";
import {GetDataResponseProto} from "../protocol/get-data-response-proto.js";
import {SubscribeDataUpdatesResponseProto} from "../protocol/subscribe-data-updates-response-proto.js";
import {UserMessageUnionProto} from "../protocol/user-message-union-proto.js";
import {DataUpdateMessageProto} from "../protocol/data-update-message-proto.js";
import {SaveDataResponseProto} from "../protocol/save-data-response-proto.js";
import {SubscribeMessageResponseProto} from "../protocol/subscribe-message-response-proto.js";
import {RiverUserMessageProto} from "../protocol/river-user-message-proto.js";
import {UserMessageUnionWrapperProto} from "../protocol/user-message-union-wrapper-proto.js";

import {
    MethodId,
    OkResponseType,
    UserResponse, MessageInstanceKey,
    WorkerKey,
    DataKey,
    ServiceKey
} from "../internal-model-types.js";
import {Data} from "../../public/model-types.js"

function validateWorkerTarget(expected: WorkerKey, other: WorkerKey, what: string) {
    requiresTruthy('expected', expected);
    requiresTruthy('other', other);

    if (expected.equals(other))
        return; //OK

    if (expected.workerId.equals(other.workerId)) {
        //the version has changed, making this client incompatible.
        const worker_name = WorkerRegistry.instance.getName(expected);
        throw new Error(sysLogError(`The worker client for ${worker_name} is out of date and must be re-generated.`));
    }

    if (WorkerRegistry.instance.isRegistered(other)) {
        //data meant for a different worker (estate programming error)
        const worker_name = WorkerRegistry.instance.getName(expected);
        const other_worker_name = WorkerRegistry.instance.getName(other);
        throw new Error(sysLogError(`Unexpectedly received ${what} destined for the worker ${other_worker_name} when ${worker_name} was expected.`));
    }

    const maybe_other_worker_key = WorkerRegistry.instance.tryGetWorkerKeyByWorkerId(other.workerId);
    if (!maybe_other_worker_key) {
        //totally unknown worker, not quite sure how this could happen.
        throw new Error(sysLogError(`Unexpectedly received ${what} destined for unknown worker ${other.value}.`));
    }

    const other_worker_name = WorkerRegistry.instance.getName(maybe_other_worker_key);
    //if the message was destined for a different worker and that worker's version is different than what's registered with this client
    throw new Error(sysLogError(`Unexpectedly received ${what} destined for the worker ${other_worker_name}.`));
}

export type DataUpdateMessageHandler = (logContext: string, worker_key: WorkerKey, objectUpdateMessageProto: DataUpdateMessageProto) => Promise<void>;
export type MessageMessageHandler = (logContext: string, worker_key: WorkerKey, eventMessageProto: MessageMessageProto) => Promise<void>;

export async function openRiverClientAsync(workerKey: WorkerKey, userKey: string,
                                           updateHandler: DataUpdateMessageHandler,
                                           eventHandler: MessageMessageHandler): Promise<RiverClient> {
    return new RiverClient(await openOuterspaceConnectionAsync(RIVER_SERVICE_URL, userKey, function (messageBuffer) {
        requiresTruthy('messageBuffer', messageBuffer);

        const riverUserMessage = RiverUserMessageProto.getRootAsRiverUserMessageProto(new ByteBuffer(new Uint8Array(messageBuffer)));

        const logContext = riverUserMessage.logContext();
        if(!logContext)
            throw new Error(sysLogError('Unable to read message because it contained an empty log context'));

        const otherWorkerKey = new WorkerKey(Unsigned.fromLong(riverUserMessage.workerId()), Unsigned.fromLong(riverUserMessage.workerVersion()));

        const message = riverUserMessage.message();
        if(!message)
            throw new Error(sysLogError("Failed to read user message from River because it was empty"));

        const userMessageWrapperBytes = message.bytesArray();
        if(!userMessageWrapperBytes)
            throw new Error(sysLogError("Failed to read user message from River because it was empty"));

        const userMessageUnionWrapper = UserMessageUnionWrapperProto.getRootAsUserMessageUnionWrapperProto(new ByteBuffer(userMessageWrapperBytes!));

        switch (userMessageUnionWrapper.valueType()) {
            case UserMessageUnionProto.DataUpdateMessageProto: {
                validateWorkerTarget(workerKey, otherWorkerKey, "an object update");
                const update = new DataUpdateMessageProto();
                if (userMessageUnionWrapper.value(update)) {
                    return updateHandler(logContext, workerKey, update);
                } else {
                    throw new Error(sysLogError("Failed to read worker object update because it contained invalid data"));
                }
            }
            case UserMessageUnionProto.MessageMessageProto: {
                validateWorkerTarget(workerKey, otherWorkerKey, "an event");
                const event = new MessageMessageProto();
                if (userMessageUnionWrapper.value(event)) {
                    return eventHandler(logContext, workerKey, event);
                } else {
                    throw new Error(sysLogError("Failed to read event because it contained invalid data"));
                }
            }
            case UserMessageUnionProto.NONE:
                throw new Error(sysLogError(`Message contained an invalid message type`));
        }
    }));
}

function logSend(logContext: string, method: string) {
    logDebug(logContext, `Calling "${method}"`);
}

function logReceive<T extends OkResponseType>(logContext: string, method: string, userResponse: UserResponse<T>) {
    if (userResponse.response) {
        const messageCount = userResponse.messages ? userResponse.messages.length : 0;
        logVerbose(logContext, `Received: ${method} OK with ${messageCount} messages`);
    } else if (userResponse.platformException) {
        logError(logContext, `Received: PlatformError ${userResponse.platformException.codeString}`);
    } else if (userResponse.exception) {
        const scriptException = userResponse.exception;
        logError(logContext, `Received: ScriptException "${scriptException.message}" at ${scriptException.stack}`);
    }
}

export class RiverClient {
    constructor(private readonly outerspaceClient: OuterspaceClient) {
        requiresTruthy('outerspaceClient', outerspaceClient);
    }

    shutdown() {
        this.outerspaceClient.shutdown();
    }

    async getDataAsync(logContext: string, dataKey: DataKey): Promise<UserResponse<GetDataResponseProto>> {
        const _ = 'getDataAsync';
        requiresTruthy('dataKey', dataKey);
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createGetDataRequest(logContext, dataKey);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<GetDataResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.GetDataResponseProto, GetDataResponseProto, false);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<GetDataResponseProto>(logContext, _, response);
        return response;
    };

    async callServiceMethodAsync(logContext: string, serviceKey: ServiceKey, methodId: MethodId, args: any[]): Promise<UserResponse<CallServiceMethodResponseProto>> {
        const _ = 'callServiceMethodAsync';
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createCallServiceMethodRequest(logContext, serviceKey, methodId, args);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<CallServiceMethodResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.CallServiceMethodResponseProto, CallServiceMethodResponseProto, true);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<CallServiceMethodResponseProto>(logContext, _, response);
        return response;
    };

    async saveDataAsync(logContext: string, data: Data[]): Promise<UserResponse<SaveDataResponseProto>> {
        const _ = 'saveDataAsync';
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createSaveDataRequest(logContext, data);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<SaveDataResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.SaveDataResponseProto, SaveDataResponseProto, true);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<SaveDataResponseProto>(logContext, _, response);
        return response;
    };

    async subscribeMessageAsync(logContext: string, eventInstanceKey: MessageInstanceKey): Promise<UserResponse<SubscribeMessageResponseProto>> {
        const _ = 'subscribeMessageAsync';
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createSubscribeMessageRequest(logContext, eventInstanceKey);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<SubscribeMessageResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.SubscribeMessageResponseProto, SubscribeMessageResponseProto, false);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<SubscribeMessageResponseProto>(logContext, _, response);
        return response;
    };

    async unsubscribeMessageAsync(logContext: string, eventInstanceKey: MessageInstanceKey): Promise<UserResponse<UnsubscribeMessageResponseProto>> {
        const _ = 'unsubscribeMessageAsync';
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createUnsubscribeMessageRequest(logContext, eventInstanceKey);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<UnsubscribeMessageResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.UnsubscribeMessageResponseProto, UnsubscribeMessageResponseProto, false);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<UnsubscribeMessageResponseProto>(logContext, _, response);
        return response;
    };

    async subscribeDataUpdates(logContext: string, dataKeys: DataKey[]): Promise<UserResponse<SubscribeDataUpdatesResponseProto>> {
        const _ = 'subscribeDataUpdatesAsync';
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createSubscribeDataUpdatesRequest(logContext, dataKeys);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<SubscribeDataUpdatesResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.SubscribeDataUpdatesResponseProto, SubscribeDataUpdatesResponseProto, false);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<SubscribeDataUpdatesResponseProto>(logContext, _, response);
        return response;
    };

    async unsubscribeDataUpdates(logContext: string, dataKeys: DataKey[]): Promise<UserResponse<UnsubscribeDataUpdatesResponseProto>> {
        const _ = 'unsubscribeDataUpdatesAsync';
        if (options.getWorkerOptions().enableMessageTracing)
            logSend(logContext, _);
        const requestBuffer = createUnsubscribeDataUpdatesRequest(logContext, dataKeys);
        const responseBuffer = await this.outerspaceClient.sendRequestAsync(logContext, requestBuffer);
        const response = readResponse<UnsubscribeDataUpdatesResponseProto>(logContext, responseBuffer,
            UserResponseUnionProto.UnsubscribeDataUpdatesResponseProto, UnsubscribeDataUpdatesResponseProto, false);
        if (options.getWorkerOptions().enableMessageTracing)
            logReceive<UnsubscribeDataUpdatesResponseProto>(logContext, _, response);
        return response;
    };
}

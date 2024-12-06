import {
    KeyMap,
    MessageClassKey, MessageClassRegistration, MessageInstanceKey,
    WorkerIdString,
    WorkerKey,
    DataClassKey, DataClassRegistration,
    DataKey, WorkerRegistration, ServiceClassKey, ServiceClassRegistration,
    ServiceKey
} from "../internal-model-types.js";
import {RiverClient} from "../service/river-client.js";
import {sysLogError} from "./logging.js";
import {requiresPositiveUnsigned, requiresTruthy} from "./requires.js";
import {Unsigned} from "./unsigned.js";
import {
    MessageListener,
    Data,
    DataUpdateListener,
    Service,
    ServiceConstructor
} from "../../public/model-types.js";

class FakeWeakRef<T extends object> {
    constructor(private readonly value: T) {
    }

    deref(): T {
        return this.value;
    }
}

declare type WeakRefShim<T extends object> = FakeWeakRef<T> | WeakRef<T>;

function createWeakRef<T extends object>(o: T): WeakRefShim<T> {
    if (typeof WeakRef !== "undefined") {
        return new WeakRef(o);
    } else {
        return new FakeWeakRef(o);
    }
}

export class WorkerRegistry {
    private _workerIdToWorkerKey = new Map<WorkerIdString, WorkerKey>();
    private _workerKeyToWorkerRegistration = new KeyMap<WorkerKey, WorkerRegistration>();
    private _workerKeyToRiverClient = new KeyMap<WorkerKey, RiverClient>();

    private static _instance: WorkerRegistry | null = null;

    public static get instance(): WorkerRegistry {
        if (!this._instance)
            this._instance = new WorkerRegistry();
        return this._instance;
    }

    public setRiverClient(workerKey: WorkerKey, riverClient: RiverClient) {
        requiresTruthy('workerKey', workerKey);
        requiresTruthy('riverClient', riverClient);
        this.assertIsRegistered(workerKey);
        this._workerKeyToRiverClient.set(workerKey, riverClient);
    }

    public tryGetRiverClient(workerKey: WorkerKey): RiverClient | undefined {
        requiresTruthy('workerKey', workerKey);
        this.assertIsRegistered(workerKey);
        return this._workerKeyToRiverClient.tryGet(workerKey);
    }

    public clearRiverClients() {
        for(const riverClient of this._workerKeyToRiverClient.values()){
            riverClient.shutdown();
        }
        this._workerKeyToRiverClient.clear();
    }

    private assertIsRegistered(workerKey: WorkerKey) {
        if (!this.isRegistered(workerKey))
            throw new Error(`Worker ${workerKey.value} is not registered`);
    }

    public register(workerRegistration: WorkerRegistration) {
        requiresTruthy('workerRegistration', workerRegistration);

        if (this._workerKeyToWorkerRegistration.has(workerRegistration.workerKey))
            throw new Error(sysLogError(`Cannot re-register worker ${workerRegistration.workerKey.value} aka '${workerRegistration.name}'`));
        if (this._workerIdToWorkerKey.has(workerRegistration.workerKey.workerId.toString()))
            throw new Error(sysLogError(`Failed to register another version of the worker '${workerRegistration.name}`));

        this._workerKeyToWorkerRegistration.set(workerRegistration.workerKey, workerRegistration);
        this._workerIdToWorkerKey.set(workerRegistration.workerKey.workerId.toString(), workerRegistration.workerKey);
    }

    public isRegistered(workerKey: WorkerKey): boolean {
        return this._workerKeyToWorkerRegistration.has(workerKey);
    }

    public getUserKey(workerKey: WorkerKey): string {
        requiresTruthy('workerKey', workerKey);
        return this._workerKeyToWorkerRegistration.get(workerKey).userKey;
    }

    public getName(workerKey: WorkerKey): string {
        requiresTruthy('workerKey', workerKey);
        return this._workerKeyToWorkerRegistration.get(workerKey).name;
    }

    public tryGetWorkerKeyByWorkerId(workerId: Unsigned): WorkerKey | undefined {
        requiresTruthy('workerId', workerId);
        return this._workerIdToWorkerKey.get(workerId.toString());
    }
}

export class DataRegistry {
    private _classKeyToClassRegistration = new KeyMap<DataClassKey, DataClassRegistration>();
    private _ctorToClassKey = new Map<Function, DataClassKey>();
    private _objectKeyToWeakObject = new KeyMap<DataKey, WeakRefShim<Data>>();
    private _weakObjectToVersion = new WeakMap<Data, Unsigned>();
    private _weakObjectToObjectKey = new WeakMap<Data, DataKey>();
    private _weakObjectToUpdatedListeners = new WeakMap<Data, Array<DataUpdateListener>>();
    private static _instance: DataRegistry | null = null;

    public static get instance(): DataRegistry {
        if (!this._instance)
            this._instance = new DataRegistry();
        return this._instance;
    }

    public tryGetVersion(data: Data): Unsigned | undefined {
        return this._weakObjectToVersion.get(data);
    }

    public setVersion(data: Data, version: Unsigned) {
        requiresTruthy('data', data);
        requiresPositiveUnsigned('version', version);
        this._weakObjectToVersion.set(data, version);
    }

    public setInstance(dataKey: DataKey, data: Data) {
        requiresTruthy('dataKey', dataKey);
        requiresTruthy('data', data);
        const existingRef = this._objectKeyToWeakObject.tryGet(dataKey);
        if (existingRef && existingRef.deref())
            throw new Error(sysLogError(`The object ${dataKey.getFullyQualifiedName()} already exists. 
            Call E.g. await client.getDataAsync(${dataKey.classKey.getName()}, "${dataKey.primaryKey}") to retrieve it.`));
        this._weakObjectToObjectKey.set(data, dataKey);
        this._objectKeyToWeakObject.set(dataKey, createWeakRef<Data>(data));
    }

    public getDataKey(data: Data): DataKey {
        const objectKey = this._weakObjectToObjectKey.get(data);
        if (!objectKey)
            throw new Error(sysLogError('Could not find object key for worker object when it was expected to exist.'));
        return objectKey;
    }

    public tryGetInstance(dataKey: DataKey): Data | null {
        requiresTruthy('dataKey', dataKey);
        const ref = this._objectKeyToWeakObject.tryGet(dataKey);
        if (!ref)
            return null;
        const target = ref.deref();
        if (!target)
            return null;
        return target;
    }

    public addUpdatedListener(data: Data, listener: DataUpdateListener) {
        requiresTruthy('data', data);
        requiresTruthy('listener', listener);
        let listeners = this._weakObjectToUpdatedListeners.get(data);
        if (!listeners) {
            this._weakObjectToUpdatedListeners.set(data, listeners = []);
        }
        listeners.push(listener);
    }

    public removeUpdatedListener(data: Data, listener: DataUpdateListener) {
        requiresTruthy('data', data);
        requiresTruthy('listener', listener);
        let listeners = this._weakObjectToUpdatedListeners.get(data);
        if (!listeners)
            return;
        const i = listeners.indexOf(listener);
        if (i > -1) {
            listeners.splice(i, 1);
        }
    }

    public tryGetUpdatedListeners(data: Data): DataUpdateListener[] | undefined {
        return this._weakObjectToUpdatedListeners.get(data);
    }

    public clearUpdatedListeners(data: Data) {
        requiresTruthy('data', data);
        this._weakObjectToUpdatedListeners.delete(data);
    }

    public clearAllUpdateListeners() {
        this._weakObjectToUpdatedListeners = new WeakMap<Data, Array<DataUpdateListener>>();
    }

    public remove(dataKey: DataKey) {
        const ref = this._objectKeyToWeakObject.tryGet(dataKey);
        let object;
        if (ref)
            object = ref.deref();
        if (object) {
            //if they're weak, why manually remove them here?
            //because WeakMaps are pseudo-weak and I think it's better to just clean them up deterministically.
            this._weakObjectToVersion.delete(object);
            this._weakObjectToObjectKey.delete(object);
            this._weakObjectToUpdatedListeners.delete(object);
        }
        this._objectKeyToWeakObject.delete(dataKey);
    }

    public register(classRegistration: DataClassRegistration) {
        requiresTruthy('classRegistration', classRegistration);
        if (this._classKeyToClassRegistration.has(classRegistration.classKey))
            throw new Error(`Cannot re-register class ${classRegistration.classKey.value}`);
        this._classKeyToClassRegistration.set(classRegistration.classKey, classRegistration);
        this._ctorToClassKey.set(classRegistration.ctor, classRegistration.classKey);
    }

    public getClassKey(ctor: Function): DataClassKey {
        const classKey = this._ctorToClassKey.get(ctor);
        if (!classKey)
            throw new Error("Data class key not found for constructor, the type has not been registered.");
        return classKey!;
    }

    public getDataRegistration(dataClassKey: DataClassKey): DataClassRegistration {
        return this._classKeyToClassRegistration.get(dataClassKey);
    }

    public getCtor(dataClassKey: DataClassKey): any {
        const class_reg = this.getDataRegistration(dataClassKey);
        return class_reg.ctor;
    }

    public getName(dataClassKey: DataClassKey) {
        return this._classKeyToClassRegistration.get(dataClassKey).name;
    }
}

export class ServiceRegistry {
    private _classKeyToClassRegistration = new KeyMap<ServiceClassKey, ServiceClassRegistration>();
    private _ctorToClassKey = new Map<Function, ServiceClassKey>();
    private static _instance: ServiceRegistry | null = null;

    public static get instance(): ServiceRegistry {
        if (!this._instance)
            this._instance = new ServiceRegistry();
        return this._instance;
    }

    public register(classReg: ServiceClassRegistration) {
        requiresTruthy('classReg', classReg);
        if (this._classKeyToClassRegistration.has(classReg.classKey))
            throw new Error(`Cannot re-register class ${classReg.classKey.value}`);
        this._classKeyToClassRegistration.set(classReg.classKey, classReg);
        this._ctorToClassKey.set(classReg.ctor, classReg.classKey);
    }

    public isRegistered(ctor: Function): boolean {
        return this._ctorToClassKey.has(ctor);
    }

    public getClassKey(ctor: Function): ServiceClassKey {
        const classKey = this._ctorToClassKey.get(ctor);
        if (!classKey)
            throw new Error("Service class key not found for constructor, the type has not been registered.");
        return classKey!;
    }

    public getRegistration(classKey: ServiceClassKey): ServiceClassRegistration {
        return this._classKeyToClassRegistration.get(classKey);
    }

    public getCtor(classKey: ServiceClassKey): Function {
        const classRegistration = this.getRegistration(classKey);
        return classRegistration.ctor;
    }

    public createInstance(serviceKey: ServiceKey) {
        const ctor = <ServiceConstructor<Service>>this.getCtor(serviceKey.classKey);
        return new ctor(serviceKey.primaryKey);
    }

    public getName(classKey: ServiceClassKey): string {
        return this._classKeyToClassRegistration.get(classKey).name;
    }
}

export class MessageRegistry {
    private _classKeyToClassRegistration = new KeyMap<MessageClassKey, MessageClassRegistration>();
    private _ctorToClassKey = new Map<Function, MessageClassKey>();
    private _instanceKeyToListeners = new KeyMap<MessageInstanceKey, Array<MessageListener>>();

    private static _instance: MessageRegistry | null = null;
    public static get instance(): MessageRegistry {
        if (!this._instance) {
            this._instance = new MessageRegistry();
        }
        return this._instance;
    }

    public addListener(eventInstanceKey: MessageInstanceKey, listener: MessageListener) {
        requiresTruthy('eventInstanceKey', eventInstanceKey);
        requiresTruthy('listener', listener);
        let listeners = this._instanceKeyToListeners.tryGet(eventInstanceKey);
        if (!listeners) {
            this._instanceKeyToListeners.set(eventInstanceKey, listeners = []);
        }
        listeners.push(listener);
    }

    public removeListener(eventInstanceKey: MessageInstanceKey, listener: MessageListener) {
        requiresTruthy('eventInstanceKey', eventInstanceKey);
        requiresTruthy('listener', listener);
        let listeners = this._instanceKeyToListeners.tryGet(eventInstanceKey);
        if (!listeners)
            return;
        const i = listeners.indexOf(listener);
        if (i > -1) {
            listeners.splice(i, 1);
        }
    }

    public tryGetListeners(eventInstanceKey: MessageInstanceKey): MessageListener[] | undefined {
        return this._instanceKeyToListeners.tryGet(eventInstanceKey);
    }

    public clearListeners(eventInstanceKey: MessageInstanceKey) {
        requiresTruthy('eventInstanceKey', eventInstanceKey);
        this._instanceKeyToListeners.delete(eventInstanceKey);
    }

    public clearAllListeners() {
        this._instanceKeyToListeners.clear();
    }

    public register(classReg: MessageClassRegistration) {
        requiresTruthy('classReg', classReg);
        if (this._classKeyToClassRegistration.has(classReg.classKey))
            throw new Error(`Cannot re-register class ${classReg.classKey.value}`);
        this._classKeyToClassRegistration.set(classReg.classKey, classReg);
        this._ctorToClassKey.set(classReg.ctor, classReg.classKey);
    }

    public isRegistered(ctor: Function): boolean {
        return this._ctorToClassKey.has(ctor);
    }

    public getClassKey(ctor: Function): MessageClassKey {
        const classKey = this._ctorToClassKey.get(ctor);
        if (!classKey) {
            throw new Error("Message class key not found for constructor, the type has not been registered.");
        }
        return classKey!;
    }

    public getRegistration(classKey: MessageClassKey): MessageClassRegistration {
        return this._classKeyToClassRegistration.get(classKey);
    }

    public getCtor(classKey: MessageClassKey): Function {
        const classRegistration = this.getRegistration(classKey);
        return classRegistration.ctor;
    }

    public getName(classKey: MessageClassKey): string {
        return this._classKeyToClassRegistration.get(classKey).name;
    }
}
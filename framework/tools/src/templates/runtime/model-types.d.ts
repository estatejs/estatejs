import { __Message_Tag, __Data_Primary_Key, __Service_Primary_Key } from "./symbol.js";
export declare class Service {
    [__Service_Primary_Key]: string;
    constructor(primaryKey: string);
    get primaryKey(): string;
}
export declare type ServiceConstructor<T extends Service> = new (primaryKey: string) => T;
export declare class Message {
    [__Message_Tag]: true;
}
export declare type MessageConstructor<T extends Message> = new () => T;
export declare class Data {
    [__Data_Primary_Key]: string;
    constructor(primaryKey: string);
    get primaryKey(): string;
}
export declare type DataConstructor<T extends Data> = new (...any: any) => T;
export declare type DataUpdateListener = (e: DataUpdatedEvent) => Promise<void> | void;
export declare type MessageListener = (e: Message) => Promise<void> | void;
export declare class DataUpdatedEvent {
    readonly target: Data;
    readonly deleted: boolean;
    constructor(target: Data, deleted: boolean);
}

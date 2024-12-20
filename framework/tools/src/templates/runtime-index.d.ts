/**
 * Base class for your Services.
 * - Synonymous with a microservice in traditional systems.
 * - Used for executing business logic inside the Worker with direct, in-memory access to the datastore.
 * - You can create any number of properties on Services containing most kinds of JavaScript data (numbers, strings, Maps, Sets, etc..)
 * - Clients call methods defined on Services using code-generated classes.
 * - Changes made to the properties on Services are automatically saved unless an error or exception occur.
 * - Each service method call involves a round-trip to Estate backend and is executed inside a single database transaction.
 * - Use the `system` object inside service methods to manage your Data and send Messages.
 * @see system.getService
 * @see system.revert
 * @see system.delete
 * */
export class Service {
    constructor(primaryKey: string);
    get primaryKey() : string;
}

/**
 * Base class for your Data.
 * - Synonymous with a database table in traditional systems.
 * - Has a string named primaryKey that uniquely identifies the object. Use this key (and the Data extended type name) when calling system.getData.
 * - You can create any number of properties containing most kinds of JavaScript data (numbers, string, Maps, Sets, etc..)
 * - You can pass Data instances to Service methods and return them.
 * - When inside a Service transaction (a service method call), you may make any number of changes to the properties.
 * - All Data changes must be either saved or reverted before the Service transaction completes, otherwise the transaction is rolled back.
 * - Clients can receive server-sent realtime push updates to the Data instances they care about using a combination of worker.subscribeUpdatesAsync followed by worker.addUpdateListener.
 * @see system.getData
 * @see system.saveDataGraphs
 * @see system.saveData
 * @see system.revert
 * @see system.delete
 * @see worker.getDataAsync
 * @see worker.saveDataAsync
 * @see worker.subscribeUpdatesAsync
 * @see worker.addUpdateListener
 * */
export class Data {
    constructor(primaryKey: string);
    get primaryKey() : string;
}

/**
 * Base class for your Messages.
 * - Used to push realtime data to your clients from your Services.
 * - An example might be an event named `UserAdded` which you fire when your service (see Service above) adds a new user. This event could be subscribed to by clients using worker.subscribeEventAsync.
 * - Events are fired from your services inside service methods using E.g. `system.sendMessage(new UserAdded("Scott"))` where `UserAdded` is a worker class that extends Message.
 * - You can create any number of properties on events using most JavaScript types (the same as Data and Services).
 * - Does not have a primary key as it's not stored in the database.
 * @see system.sendMessage
 * @see worker.subscribeEventAsync
 * */
export class Message {
}

/**
 * Generates a universally-unique identifier.
 * @param {boolean} dashes - (default: false) Whether to break up the UUID string with dashes to make to more readable to humans.
 * @returns {string} The UUID with or without dashes.
 * */
export function createUuid(dashes: boolean | undefined) : string;

/**
 * Functions used to interact with the Worker machine.
 * */
export class system {
    /**
     * Saves one or more Data to the database.
     * - All changes made to Data must be saved or reverted by the end of the transaction. Otherwise, the service call will fail with a Engine_UnsavedChanges error.
     * @see Data
     * @param {Data[]} data - One or more Data to save.
     * @returns {boolean} True if there were any changes to save, False otherwise.
     * */
    static saveData(...data: Data[]) : boolean;

    /**
     * Saves one or more Data and all the Data they reference (the entire tree) to their datastores.
     * @param {Data[]} dataRoots - One or more Data, along with all the Data they reference, to save.
     * @returns {boolean} True if there were any changes to save, False otherwise.
     * */
    static saveDataGraphs(...dataRoots: Data[]) : boolean;

    /**
     * Undoes any changes made since the last time a Data or Service was saved in the current transaction.
     * - Only changes made since the last saveData/saveDataGraphs call are reverted.
     * - Only undoes changes made as part of the save transaction, that is, the same service call.
     * - If there hasn't been a saveData/saveDataGraphs call then all changes made since the beginning of the
     * transaction will be reverted. This is always the case for Services since they cannot be saved manually.
     * - This function does not revert any changes made to referenced Data/Services (I.e. It leaves the rest of the object graph alone).
     * @param {T} serviceOrObject - The Service or Data to revert.
     * */
    static revert<T extends Service | Data>(serviceOrObject: T) : void;

    /**
     * Gets an existing Service instance from the database or creates a new one if it does not exist.
     * @param {new() => T} serviceType - The class type that extends Service. (E.g. MyService)
     * @param {string} primaryKey - The primary key used to uniquely identify the Service instance.
     * @returns {T} The Service instance.
     * */
    static getService<T extends Service>(serviceType : new() => T, primaryKey: string) : T;

    /**
     * Gets an existing Data instance from the database.
     * - New Data instances are not created automatically with this call. You can create new Data instances just like you would any other JavaScript object: E.g. `let player = new Player("Scott")` where `Player` is a worker class that extends the Data base class.
     * @param {new() => T} dataType - The class type that extends Data. (E.g. `Player` that extends Data)
     * @param {string} primaryKey - A string that uniquely identifies the Data instance.
     * @returns {T} The Data instance.
     * */
    static getData<T extends Data>(dataType : new() => T, primaryKey: string) : T;

    /**
     * Permanently deletes a Data or Service instance from the database. Once it has been deleted it cannot be retrieved.
     * @param {T} serviceOrObject - The Service or Data to permanently delete.
     * @param {boolean} purge - (Optional, default = false) Normal deletion leaves behind a very small marker that prevents deleted services and objects from being re-created accidentally. Passing true to this argument prevents this marker from being written.
     * For service deletions specifically, careful consideration should be made when setting this to true because it could result in accidental re-creation because services are created-on-first-use.
     * */
    static delete<T extends Service | Data>(serviceOrObject: T, purge: boolean) : void;

    /**
     * Fires a Message instance to all clients that are subscribed.
     * - Any Data or Services referred to by properties on the Message that have had changes during the current service method call must be manually saved before calling sendMessage. Otherwise, an exception is thrown. This ensures a consistent state for client-side message subscribers.
     * - Messages will not be fired when a service call returns an error or an exception is unhandled by your code.
     * @see saveDataGraphs
     * @param {TS} source - A reference to a Service or Data which will be used as the source of this message.
     * @param {TE} message - The Message object to send to all subscribed clients.
     * */
    static sendMessage<TS extends Service | Data, TM extends Message>(source: TS, message : TM) : void;
}

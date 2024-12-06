import {Data, Message, Service} from "./model-types";

/**
 * Functions used to interact with the Worker machine at runtime.
 * */
export class system {
    /**
     * Saves one or more Data to the database.
     * - All changes made to Data must be saved or reverted by the end of the transaction. Otherwise, the service call will fail with an Engine_UnsavedChanges error.
     * @see Data
     * @param {Data[]} data - One or more Data to save.
     * @returns {boolean} True if there were any changes to save, False otherwise.
     * */
    static saveData(...data: Data[]): boolean;

    /**
     * Saves one or more Data and all the Data they reference (the entire tree) to their datastore's.
     * @param {Data[]} dataRoots - One or more Data, along with all the Data they reference, to save.
     * @returns {boolean} True if there were any changes to save, False otherwise.
     * */
    static saveDataGraphs(...dataRoots: Data[]): boolean;

    /**
     * Undoes any changes made since the last time a Data or Service was saved in the current transaction.
     * - Only changes made since the last saveData/saveDataGraphs call are reverted.
     * - Only undoes changes made as part of the save transaction, that is, the same service call.
     * - If there hasn't been a saveData/saveDataGraphs call then all changes made since the beginning of the
     * transaction will be reverted. This is always the case for Services since they cannot be saved manually.
     * - This function does not revert any changes made to referenced Data/Services (I.e. It leaves the rest of the object graph alone).
     * @param {T} serviceOrData - The Service or Data to revert.
     * */
    static revert<T extends Service | Data>(serviceOrData: T): void;

    /**
     * Gets an existing Service instance from the database or creates a new one if it does not exist.
     * @param {new(primaryKey: string) => T} serviceType - The class type that extends Service. (E.g. MyService)
     * @param {string} primaryKey - The primary key used to uniquely identify the Service instance.
     * @returns {T} The Service instance.
     * */
    static getService<T extends Service>(serviceType: new (primaryKey: string) => T, primaryKey: string): T;

    /**
     * Gets an existing Data instance from the database.
     * - New Data instances are not created automatically with this call. You can create new Data instances just like you would any other JavaScript object: E.g. `let player = new Player("Scott")` where `Player` is a worker class that extends the Data base class.
     * @param {new(...args: any[]) => T} dataType - The class type that extends Data. (E.g. `Player` that extends Data)
     * @param {string} primaryKey - A string that uniquely identifies the Data instance.
     * @returns {T} The Data instance.
     * */
    static getData<T extends Data>(dataType: new (...args: any[]) => T, primaryKey: string): T;

    /**
     * Permanently deletes a Data or Service instance from the database. Once it has been deleted it cannot be retrieved.
     * @param {T} serviceOrData - The Service or Data to permanently delete.
     * @param {boolean} purge - (Optional, default = false) Normal deletion leaves behind a very small marker that prevents deleted service and data instances from being re-created accidentally. Passing true to this argument prevents this marker from being written.
     * For service deletions specifically, careful consideration should be made when setting this to true because it could result in accidental re-creation because services are created-on-first-use.
     * */
    static delete<T extends Service | Data>(serviceOrData: T, purge?: boolean): void;

    /**
     * Fires a Message instance to all clients that are subscribed.
     * - Any Data or Services referred to by properties on the Message that have had changes during the current service method call must be manually saved before calling sendMessage. Otherwise, an exception is thrown. This ensures a consistent state for client-side message subscribers.
     * - Messages will not be fired when a service call returns an error or an exception is unhandled by your code.
     * @see saveDataGraphs
     * @param {TS} source - A reference to a Service or Data which will be used as the source of this message.
     * @param {TE} message - The Message object to send to all subscribed clients.
     * */
    static sendMessage<TS extends Service | Data, TM extends Message>(source: TS, message: TM): void;
}

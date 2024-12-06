import {getUniqueId} from "./util";
import {Data, Service, Message, system, createUuid} from "../asdf/./a.asdfasdf././../adsf./worker-runtime/index";

export class ExampleData extends Data {
    constructor(name) {
        super(getUniqueId());
        this.name = name;
    }
}

export class AssociatedData extends Data {
    constructor(firstName, lastName) {
        super(firstName + lastName);
        this.firstName = firstName;
        this.lastName = lastName;
    }
}

class ExampleService extends Service {
    constructor(primaryKey) {
        super(primaryKey);
        this._index = {};
        console.log("Log 1");
        console.error("Error 1");
        console.log("Log 2");
        console.error("Error 2");
    }
    getDataTest(dataUuid) {
        return system.getData(ExampleData, dataUuid);
    }
    getDataNotFoundTest() {
        return system.getData(ExampleData, "bogus");
    }
    saveAndMessagesTest(name) {
        if(this.dataExists(name))
            throw new Error(`Data with that name already exists with name ${name}`);
        let data = new ExampleData(name);
        this._index[name] = data;
        system.saveData(data); //data v1
        system.sendMessage(this, new DataAdded(data));
        data.name = "jones";
        system.saveData(data); //data v2
        system.sendMessage(this, new DataAdded(data));
        data.associatedData = new AssociatedData("Scott", "Jones");
        delete data.name;
        system.saveDataGraphs(data); //data v3, associatedData v1
        return data;
    }
    argDeltaDoesntCauseUnsavedDataWhenPreviouslySaved1() {
        const data = new ExampleData("something something whatever nothing");
        system.saveData(data);
        return data;
    }
    argDeltaDoesntCauseUnsavedDataWhenPreviouslySaved2(data) {
        system.sendMessage(this, new DataAdded(data));
        return data;
    }
    dataExists(name) {
        return this._index.hasOwnProperty(name);
    }
    revertObjectToNothingTest() {
        const data = new ExampleData("nothing");
        data.name = "one";
        system.revert(data);
        return data; //return dangling ref
    }
    revertObjectToSaveTest() {
        const data = new ExampleData("to save");
        system.saveData(data);
        data.name = "one";
        system.revert(data);
        return data;
    }
    unsavedChangesTest() {
        const data = new ExampleData("whatever");
        system.saveData(data);
        data.name = "two";
    }
    revertSelfTest() {
        this._something = true;
        system.revert(this);
        return this._something;
    }
    createServiceTest() {
        return system.getService(MetadataService, createUuid(false));
    }
    getServiceTest(id) {
        return system.getService(MetadataService, id);
    }
    invalidCallTest() {
        system.getService();
    }
    inboundDeltaCreateObject(ref) {
        system.saveData(ref);
    }
    inboundDeltaAddProperty(ref) {
        system.saveData(ref);
    }
    inboundDeltaDeleteProperty(ref) {
        system.saveData(ref);
    }
    inboundDeltaNoChanges(ref) {
        system.saveData(ref); //no-op
    }
    deleteSelfTest() {
        system.delete(this);
    }
    deleteSelfTestConfirm() {
        return true; //shouldn't return anything, Serenity should return ObjectDeleted.
    }
    deleteTest1(name) {
        const data = new ExampleData(name);
        system.saveData(data);
        return data;
    }
    deleteTest2(data) {
        system.delete(data);
    }
    deleteTest3(pk) {
        system.getData(ExampleData, pk); //throws Database_ObjectDeleted
    }
    purgeTest1(name) {
        const data = new ExampleData(name);
        system.saveData(data);
        return data;
    }
    purgeTest2(data) {
        system.delete(data, true);
    }
    purgeTest3(pk) {
        system.getData(ExampleData, pk); //throws Database_ObjectNotFound
    }
    evalTest() {
        eval("2+2"); //should throw an exception
    }
}

class DataAdded extends Message {
    constructor(data) {
        super();
        this.data = data;
    }
}

class DataDeleted extends Message {
    constructor(data, dataPrimaryKey) {
        super();
        this.data = data;
        this.dataPrimaryKey = dataPrimaryKey;
    }
}

class MetadataService extends Service {
    constructor(primaryKey) {
        super(primaryKey);
    }
    bogus() {
    }
}

export {DataAdded, DataDeleted};
export {MetadataService};

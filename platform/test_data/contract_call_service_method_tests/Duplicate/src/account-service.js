import {Data, Service} from "worker-runtime";

class Account extends Data {
    constructor(primaryKey) {
        super(primaryKey);
    }
}

class AccountService extends Service {
    constructor(primaryKey) {
        super(primaryKey);
    }
    addDuplicate() {
        //these create new JS objects that point to the same internal Object
        this._duplicate = new Account("duplicate");
        this._duplicate2 = new Account("duplicate");
        this._duplicate3 = this._duplicate;
        this._duplicate4 = this._duplicate2;
        return this._duplicate;
    }
}

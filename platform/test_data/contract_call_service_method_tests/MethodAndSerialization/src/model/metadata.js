import {Data} from "../../worker-runtime";

export class Metadata extends Data {
    constructor(primaryKey) {
        super(primaryKey);
    }
    update(value) {
        this._value = value;
        return this._value;
    }
}

import {Service, system} from "../../worker-runtime";
import {Metadata} from "../model/metadata.js";
//[sic] these are odd, but correct imports that verify imports work correctly
import {fun} from "../services/z/m.js";
import {q_fun} from "./q/q"

class ExampleService extends Service {
    constructor(x) {
        super(x);
    }
    testBoolean(value) {
        this._boolean_property = value;
        if(typeof this._boolean_property !== "boolean")
            throw new Error("not boolean");
        return this._boolean_property;
    }
    testNumber(value) {
        this._number_property = value;
        if(typeof this._number_property !== "number")
            throw new Error("not number");
        return this._number_property;
    }
    testString(value) {
        this._string_property = value;
        if(typeof this._string_property !== "string")
            throw new Error("not string");
        return this._string_property;
    }
    testObject(value) {
        this._object_property = value;
        if(typeof this._object_property !== "object")
            throw new Error("not object");
        return this._object_property;
    }
    testDataReference(value) {
        this._data_reference_property = value;
        if(!(this._data_reference_property instanceof Metadata))
            throw new Error("not Metadata");
        system.saveData(this._data_reference_property);
        return this._data_reference_property;
    }
    testServiceReference(value) {
        this._service_reference_property = value;
        if(!(this._service_reference_property instanceof OtherService))
            throw new Error("not OtherService");
        return this._service_reference_property;
    }
    testNull(value) {
        this._null_property = value;
        if(this._null_property !== null)
            throw new Error("not null");
        return this._null_property;
    }
    testUndefined(value) {
        this._undefined_property = value;
        if(this._undefined_property !== undefined)
            throw new Error("not undefined");
        return this._undefined_property;
    }
    testArray(value) {
        this._array_property = value;
        if(!(this._array_property instanceof Array))
            throw new Error("not Array");
        return this._array_property;
    }
    testMap(value) {
        this._map_property = value;
        if(!(this._map_property instanceof Map))
            throw new Error("not Map");
        return this._map_property;
    }
    testSet(value) {
        this._set_property = value;
        if(!(this._set_property instanceof Set))
            throw new Error("not Set");
        return this._set_property;
    }
    testDate(value) {
        this._date_property = value;
        if(!(this._date_property instanceof Date))
            throw new Error("not Date");
        const dcomp = new Date("2003-03-25T02:52:43.000Z");
        if(this._date_property.getTime() !== dcomp.getTime())
            throw new Error("different date");
        return this._date_property;
    }
}

class OtherService extends Service {
    constructor(x) {
        super(x);
    }
    doSomething() {
        //bogus
    }
}

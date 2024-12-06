import {Data} from "worker-runtime";

//note: all the properties are created dynamically in the tests

class Metadata extends Data {
    constructor(pk) {
        super(pk);
    }
}

class User extends Data {
    constructor(pk) {
        super(pk);
    }
}

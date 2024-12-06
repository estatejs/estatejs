function fail(msg) {
    console.trace(msg);
    throw new Error(msg);
}
let assert = {
    type(v, t) {
        if (typeof v !== t)
            fail(`Expected type to be ${t} but was ${typeof v}`);
    },
    value(v, c) {
        if (v !== c)
            fail(`${v} !== ${c}`);
    },
    typeValue(v, t, c) {
        this.type(v, t);
        this.value(v, c);
    },
    some(ar, pred) {
        if(!ar.some(pred))
            fail("Item not found");
    },
    someTypeValue(ar, t, v) {
        if(!ar.some(_ => typeof _ === t && _ === v))
            fail(`Expected to find type ${t} with value ${v} in ${JSON.stringify(ar)} but could not.`);
    },
    someInstanceOfWorkerType(ar, clazz, clazzName, pk) {
        this.someInstanceOf(ar, clazz, clazzName, _ => typeof _.primaryKey === "string" && _.primaryKey === pk);
    },
    someInstanceOf(ar, clazz, clazzName, and_pred = null) {
        if(!ar.some(_ => typeof _ === "object" && _ instanceof clazz && (and_pred && and_pred(_))))
            fail(`Expected to find an instance of ${clazzName} in ${JSON.stringify(ar)} but could not.`);
    },
    instanceOf(v, clazz, clazzName) {
        if (typeof v !== "object")
            fail(`Expected ${v} to be an object when it was an ${typeof v}`);
        if (!(v instanceof clazz)) {
            fail(`Expected ${v} to be an instance of ${clazzName} but it wasn't.`);
        }
    },
    fail(msg) {
        fail(msg);
    }
};

module.exports = {assert};
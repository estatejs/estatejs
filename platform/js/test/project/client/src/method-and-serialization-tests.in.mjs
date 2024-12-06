let SERVICE;
let USER;
let EXERCISE;

function singleInstanceOf(ar, clazz) {
    let found = null;
    for (let v of ar) {
        if (v instanceof clazz) {
            if (found)
                throw new Error("Duplicate instance of type found")
            found = v;
        }
    }
    return found;
}

async function suite() {
    setLogPrefix("[suite]");
    worker.setOptions(new WorkerOptions(true, true, getLogPrefix()));

    await test('Can create Service', async function () {
        SERVICE = worker.getService(TestService, "default");
        if (!SERVICE)
            throw new Error("couldn't get the service");
    });

    await test('Can pass & receive boolean', async function () {
        const value = await SERVICE.testBooleanAsync(true);
        assert.typeValue(value, "boolean", true);
    });

    await test('Can pass & receive number', async function () {
        const value = await SERVICE.testNumberAsync(5498);
        assert.typeValue(value, "number", 5498);
    });

    await test('Can pass & receive string', async function () {
        const value = await SERVICE.testStringAsync("something");
        assert.typeValue(value, "string", "something");
    });

    await test('Can pass & receive object', async function () {
        const metadata = new Metadata("something");
        const service = new OtherService("nothing");
        const date = new Date();
        const map = new Map();
        map.set("baz", metadata);
        map.set("foo", service);
        map.set("bar", date);
        const value = {
            booleanProperty: true,
            numberProperty: 12314,
            stringProperty: "whatever",
            dateProperty: date,
            objectProperty: {
                nestedBoolean: true,
                nestedNumber: 987987,
                nestedString: "anything",
                nestedDate: date,
            },
            refProperty: metadata,
            svcProperty: service,
            nullProperty: null,
            undefProperty: undefined,
            arrayProperty: [
                true,
                "likewise",
                3.1415927,
                metadata,
                service
            ],
            mapProperty: map,
            setProperty: new Set([
                "something",
                true,
                3.1415927,
                metadata,
                service,
                date
            ])
        };
        const comp = await SERVICE.testObjectAsync(value);
        assert.type(comp, "object");
        assert.typeValue(comp.booleanProperty, "boolean", true);
        assert.typeValue(comp.numberProperty, "number", 12314);
        assert.typeValue(comp.stringProperty, "string", "whatever");
        assert.instanceOf(comp.dateProperty, Date, "Date");
        assert.typeValue(comp.dateProperty.valueOf(), "number", date.valueOf());
        assert.type(comp.objectProperty, "object");
        assert.typeValue(comp.objectProperty.nestedBoolean, "boolean", true);
        assert.typeValue(comp.objectProperty.nestedNumber, "number", 987987);
        assert.typeValue(comp.objectProperty.nestedString, "string", "anything");
        assert.instanceOf(comp.objectProperty.nestedDate, Date, "Date");
        assert.typeValue(comp.objectProperty.nestedDate.valueOf(), "number", date.valueOf());
        assert.instanceOf(comp.refProperty, Metadata, "Metadata");
        assert.typeValue(comp.refProperty.primaryKey, "string", "something");
        assert.instanceOf(comp.svcProperty, OtherService, "OtherService");
        assert.typeValue(comp.svcProperty.primaryKey, "string", "nothing");
        assert.value(comp.nullProperty, null);
        assert.value(comp.undefProperty, undefined);
        assert.instanceOf(comp.arrayProperty, Array, "Array");
        assert.typeValue(comp.arrayProperty[0], "boolean", true);
        assert.typeValue(comp.arrayProperty[1], "string", "likewise");
        assert.typeValue(comp.arrayProperty[2], "number", 3.1415927);
        assert.instanceOf(comp.arrayProperty[3], Metadata, "Metadata");
        assert.typeValue(comp.arrayProperty[3].primaryKey, "string", "something")
        assert.instanceOf(comp.arrayProperty[4], OtherService, "OtherService");
        assert.typeValue(comp.arrayProperty[4].primaryKey, "string", "nothing");
        assert.instanceOf(comp.mapProperty, Map, "Map");
        const baz = comp.mapProperty.get("baz");
        assert.instanceOf(baz, Metadata, "Metadata");
        const foo = comp.mapProperty.get("foo");
        assert.instanceOf(foo, OtherService, "OtherService");
        const bar = comp.mapProperty.get("bar");
        assert.instanceOf(bar, Date, "Date");
        assert.instanceOf(comp.setProperty, Set, "Set");
        if (comp.setProperty.size !== 6)
            throw new Error("incorrect set size");
        const set_items = Array.from(comp.setProperty);
        assert.someTypeValue(set_items, "string", "something");
        assert.someTypeValue(set_items, "boolean", true);
        assert.someTypeValue(set_items, "number", 3.1415927);
        assert.someInstanceOfWorkerType(set_items, Metadata, "Metadata", "something");
        assert.someInstanceOfWorkerType(set_items, OtherService, "OtherService", "nothing");
        assert.someInstanceOf(set_items, Date, "Date", _ => _.valueOf() === date.valueOf());
    });

    await test('Can pass & receive Data', async function () {
        const metadata = new Metadata("totally");
        const comp = await SERVICE.testDataReferenceAsync(metadata);
        assert.instanceOf(comp, Metadata, "Metadata");
        assert.typeValue(comp.primaryKey, "string", "totally");
    });

    await test('Can pass & receive Service', async function () {
        const other = new OtherService("crazy");
        const comp = await SERVICE.testServiceReferenceAsync(other);
        assert.instanceOf(comp, OtherService, "OtherService");
        assert.typeValue(comp.primaryKey, "string", "crazy");
    });

    await test('Can pass & receive null', async function () {
        const v = null;
        const comp = await SERVICE.testNullAsync(v);
        assert.value(comp, null);
    });

    await test('Can pass & receive undefined', async function () {
        const v = undefined;
        const comp = await SERVICE.testUndefinedAsync(v);
        assert.value(comp, undefined);
    });

    await test('Can pass & receive array', async function () {
        const metadata = new Metadata("nothing at all");
        const service = new OtherService("something from nothing");
        const date = new Date();
        const value = [
            "something",
            true,
            3.1415927,
            metadata,
            service,
            date,
            null,
            undefined,
            {
                booleanProperty: true,
                numberProperty: 12314.12312,
                stringProperty: "whatever",
                nullProperty: null,
                undefProperty: undefined,
                dateProperty: date,
                refProperty: metadata,
                svcProperty: service,
            },
            [1, "anything", true],
            new Map([["foo", true], ["bar", "baz"]]),
            new Set([55, true])
        ];
        const comp = await SERVICE.testArrayAsync(value);
        assert.type(comp, "object");
        assert.instanceOf(comp, Array, 'Array');
        assert.typeValue(comp[0], 'string', 'something');
        assert.typeValue(comp[1], 'boolean', true);
        assert.typeValue(comp[2], 'number', 3.1415927);
        assert.instanceOf(comp[3], Metadata, 'Metadata');
        assert.typeValue(comp[3].primaryKey, 'string', 'nothing at all');
        assert.instanceOf(comp[4], OtherService, 'OtherService');
        assert.typeValue(comp[4].primaryKey, 'string', 'something from nothing');
        assert.instanceOf(comp[5], Date, 'Date');
        assert.typeValue(comp[5].valueOf(), 'number', date.valueOf());
        assert.value(comp[6], null);
        assert.value(comp[7], undefined);
        assert.type(comp[8], 'object');
        assert.typeValue(comp[8].booleanProperty, 'boolean', true);
        assert.typeValue(comp[8].numberProperty, 'number', 12314.12312);
        assert.typeValue(comp[8].stringProperty, 'string', 'whatever');
        assert.value(comp[8].nullProperty, null);
        assert.value(comp[8].undefProperty, undefined);
        assert.instanceOf(comp[8].dateProperty, Date, 'Date');
        assert.typeValue(comp[8].dateProperty.valueOf(), 'number', date.valueOf());
        assert.instanceOf(comp[8].refProperty, Metadata, 'Metadata');
        assert.typeValue(comp[8].refProperty.primaryKey, 'string', 'nothing at all');
        assert.instanceOf(comp[8].svcProperty, OtherService, 'OtherService');
        assert.typeValue(comp[8].svcProperty.primaryKey, 'string', 'something from nothing');
        assert.instanceOf(comp[9], Array, 'Array');
        assert.typeValue(comp[9][0], 'number', 1);
        assert.typeValue(comp[9][1], 'string', 'anything');
        assert.typeValue(comp[9][2], 'boolean', true);
        assert.instanceOf(comp[10], Map, 'Map');
        assert.typeValue(comp[10].get("foo"), 'boolean', true);
        assert.typeValue(comp[10].get("bar"), 'string', 'baz');
        assert.instanceOf(comp[11], Set, "Set");
        if (comp[11].size !== 2)
            throw new Error("incorrect set size");
        const ar = Array.from(comp[11]);
        assert.someTypeValue(ar, "number", 55);
        assert.someTypeValue(ar, "boolean", true);
    });

    await test('Can pass & receive Map', async function () {
        const value = new Map();
        const date = new Date();
        const metadata = new Metadata("anything or something");

        {
            /*note: services don't round-trip as the same object*/
            const service = new OtherService("something or nothing");
            value.set("silly", true);
            value.set(null, metadata);
            value.set(undefined, service);
            value.set(true, date);
            value.set(false, "something");
            value.set(3.14, undefined);
            value.set(metadata, service);
            value.set(service, metadata);
            value.set(date, true);
            value.set([1, 2, "foo"], date);
            value.set(new Map([["baz", 55]]), new Map([["bar", 3.14]]));
            value.set(new Set([18]), new Set([1, 55, true, "bar"]));
            value.set({bar_asdlkfhkjh: "baz"}, {foo_asdfkjh: "bar"});
        }

        const comp = await SERVICE.testMapAsync(value);
        assert.instanceOf(comp, Map, "Map");
        if (comp.size !== 13)
            throw new Error("incorrect size");

        assert.typeValue(comp.get("silly"), 'boolean', true);
        assert.instanceOf(comp.get(null), Metadata, 'Metadata');
        assert.typeValue(comp.get(null).primaryKey, 'string', 'anything or something');
        assert.instanceOf(comp.get(undefined), OtherService, 'OtherService');
        assert.typeValue(comp.get(undefined).primaryKey, 'string', 'something or nothing');
        assert.instanceOf(comp.get(true), Date, 'Date');
        assert.typeValue(comp.get(true).valueOf(), 'number', date.valueOf());
        assert.typeValue(comp.get(false), 'string', 'something');
        assert.value(comp.get(3.14), undefined);
        assert.instanceOf(comp.get(metadata), OtherService, 'OtherService');
        assert.typeValue(comp.get(metadata).primaryKey, 'string', 'something or nothing');

        const service = singleInstanceOf(comp.keys(), OtherService);
        if (!service)
            throw new Error("Unable to find service key");
        assert.typeValue(service.primaryKey, "string", "something or nothing");
        assert.instanceOf(comp.get(service), Metadata, 'Metadata');
        assert.typeValue(comp.get(service).primaryKey, 'string', 'anything or something');
        assert.value(comp.get(service), metadata); //round-trips as the same object instance

        const dateComp = singleInstanceOf(comp.keys(), Date);
        assert.instanceOf(dateComp, Date, 'Date');
        assert.typeValue(dateComp.valueOf(), 'number', date.valueOf());
        assert.typeValue(comp.get(dateComp), 'boolean', true);

        const array = singleInstanceOf(comp.keys(), Array);
        if (!array)
            throw new Error('array missing');
        assert.typeValue(array[0], 'number', 1);
        assert.typeValue(array[1], 'number', 2);
        assert.typeValue(array[2], 'string', "foo");
        assert.typeValue(comp.get(array).valueOf(), 'number', date.valueOf());

        const map = singleInstanceOf(comp.keys(), Map);
        if (!map)
            throw new Error('map missing');
        if (map.size !== 1)
            throw new Error('incorrect size');
        assert.typeValue(map.get('baz'), 'number', 55);
        const mapv = comp.get(map);
        assert.instanceOf(mapv, Map, "Map");
        if (mapv.size !== 1)
            throw new Error('incorrect size');
        assert.typeValue(mapv.get('bar'), 'number', 3.14);

        const set = singleInstanceOf(comp.keys(), Set);
        if (!set)
            throw new Error('set missing');
        if (set.size !== 1)
            throw new Error('incorrect size');

        var setk = Array.from(set.keys());
        assert.someTypeValue(setk, 'number', 18);

        const setv = comp.get(set);
        assert.instanceOf(setv, Set, "Set");
        const setvk = Array.from(setv.keys());

        assert.someTypeValue(setvk, "number", 1);
        assert.someTypeValue(setvk, "number", 55);
        assert.someTypeValue(setvk, "boolean", true);
        assert.someTypeValue(setvk, "string", "bar");

        let obj = null;
        for (let _ of comp.keys()) {
            if (_ && _.hasOwnProperty("bar_asdlkfhkjh")) {
                if (obj)
                    throw new Error("duplicate object found");
                obj = _;
            }
        }
        if (!obj)
            throw new Error("object not found");
        assert.typeValue(obj.bar_asdlkfhkjh, "string", "baz");
        const objv = comp.get(obj);
        if (!objv.hasOwnProperty("foo_asdfkjh"))
            throw new Error("object value missing property");
        assert.typeValue(objv.foo_asdfkjh, "string", "bar");
    });

    await test('Can pass & receive Set', async function () {
        const metadata = new Metadata("foo");
        const service = new OtherService("bar");
        const date = new Date();
        const value = new Set();
        value.add(null);
        value.add(undefined);
        value.add(true);
        value.add(false);
        value.add(3.14);
        value.add(metadata);
        value.add(service);
        value.add(date);
        value.add("not sure");
        value.add([55, 66, "foo"]);
        value.add({bar_asdfasdf: "baz"});
        const comp = await SERVICE.testSetAsync(value);
        assert.type(comp, "object");
        assert.instanceOf(comp, Set, "Set");
        if (comp.size !== 11)
            throw new Error("incorrect size");

        const set = Array.from(comp);
        assert.some(set, _ => _ === null);
        assert.some(set, _ => _ === undefined);
        assert.someTypeValue(set, 'boolean', true);
        assert.someTypeValue(set, 'boolean', false);
        assert.someTypeValue(set, 'number', 3.14);
        assert.someInstanceOfWorkerType(set, Metadata, 'Metadata', 'foo');
        assert.someInstanceOfWorkerType(set, OtherService, 'OtherService', 'bar');
        assert.someInstanceOf(set, Date, 'Date', _ => _.valueOf() === date.valueOf());
        assert.someTypeValue(set, 'string', 'not sure');
        assert.someInstanceOf(set, Array, 'Array', _ => typeof _[0] === 'number' && _[0] === 55 &&
            typeof _[1] === 'number' && _[1] === 66 && typeof _[2] === 'string' && _[2] === "foo" );
        assert.someInstanceOf(set, Object, 'Object', _ => _.hasOwnProperty('bar_asdfasdf') && _.bar_asdfasdf === "baz" );
    });

    await test('Can pass & receive Date', async function () {
        const value = new Date("2003-03-25T02:52:43.000Z"); //Lauren's birthday
        const comp = await SERVICE.testDateAsync(value);
        assert.instanceOf(comp, Date, 'Date');
        assert.typeValue(comp.valueOf(), 'number', value.valueOf());
    });
}

run(suite, () => {
    worker.closeConnections();
});
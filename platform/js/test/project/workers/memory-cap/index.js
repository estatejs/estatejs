import {
    Service
} from "./worker-runtime";

class MemoryCapService extends Service {
    constructor(primaryKey) {
        super(primaryKey);
    }
    explode() {
        let x = new Map();
        for(let i = 0; i < 10000000; ++i)
            x.set(i, "x".repeat(1024));
    }
}

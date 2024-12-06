const path = require("path");

import {executeWorkerDeleteAsync} from "estate-tools/command/worker/worker-delete";
import {executeAccountDeleteAsync} from "estate-tools/command/account/account-delete";
import {createLogContext} from "estate-tools/util/log-context"

import * as fs from "fs";
import {EXERCISE_TRACKER_WORKER_NAME, METHOD_AND_SERIALIZATION_WORKER_NAME, MEMORY_CAP_WORKER_NAME} from "./names"

const PROJECT_DIR = path.resolve("../../project").toString();
if(!fs.existsSync(PROJECT_DIR))
    throw new Error("Directory not found: " + PROJECT_DIR);

const LOG_CONTEXT = createLogContext();

console.log("Test cleanup using log context " + LOG_CONTEXT);

async function test(test_name: string, func: any) {
    console.log("++ Test: " + test_name);
    await func();
    console.log("OK");
}

async function suite() {
    await test('worker delete memory cap', async function () {
        await executeWorkerDeleteAsync(MEMORY_CAP_WORKER_NAME, false);
    });

    await test('worker delete exercise tracker', async function () {
        await executeWorkerDeleteAsync(EXERCISE_TRACKER_WORKER_NAME, false);
    });

    await test('worker delete method and serialization', async function () {
        await executeWorkerDeleteAsync(METHOD_AND_SERIALIZATION_WORKER_NAME, false);
    });

    await test('delete account', async function(){
        const email = process.env.ESTATE_TEST_EMAIL;
        if(!email)
            throw new Error("missing ESTATE_TEST_EMAIL");
        const password = process.env.ESTATE_TEST_PASSWORD;
        if(!password)
            throw new Error("missing ESTATE_TEST_PASSWORD");
        if (!await executeAccountDeleteAsync({username: email, password: password, isNew: false}))
            throw new Error("unable to delete test account");
    });
}

suite().then(value => {
    // don't change this. test-browser.sh looks for it to know everything worked.
    // Also, the reason I'm not just using process.exit(1) is because browserify freezes if I use process.
    console.log("All tests completed successfully");
    if (typeof window !== 'undefined') {
        window.close();
    }
}).catch(reason => {
    console.error(reason);
    if (typeof window !== 'undefined') {
        window.close();
    }
});

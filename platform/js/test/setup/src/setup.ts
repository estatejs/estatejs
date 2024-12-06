import {executeEstateInitAsync} from "estate-tools/command/estate-init";
import {createNewAccountAsync} from "estate-tools/command/account/account-login";
import {executeWorkerInitAsync} from "estate-tools/command/worker/worker-init";
import {executeWorkerDeployAsync} from "estate-tools/command/worker/worker-deploy";
import {getOwnedWorkersAsync} from "estate-tools/command/worker/worker-list";
import {executeWorkerConnectAsync} from "estate-tools/command/worker/worker-connect";
import {EstateProject} from "estate-tools/model/estate-project";

const path = require("path");
import {createLogContext} from "estate-tools/util/log-context";
import {JAYNE_SERVICE_URL} from "estate-tools/config";
import {WORKER_RUNTIME_PACKAGE_NAME, WORKER_CONFIG_FILE_NAME} from "estate-tools/constants";

const RIVER_SERVICE_URL = require(`${process.env.ESTATE_FRAMEWORK_DIR}/client/dist/cjs/internal/config`).RIVER_SERVICE_URL;
import fetch from 'cross-fetch';
import * as fs from "fs";
import {
    PROJECT_NAME,
    CLIENT_NAME,
    EXERCISE_TRACKER_WORKER_NAME,
    METHOD_AND_SERIALIZATION_WORKER_NAME,
    MEMORY_CAP_WORKER_NAME
} from "./names"


const LOG_CONTEXT = createLogContext();

console.log("Test setup using log context " + LOG_CONTEXT);

async function test(test_name: string, func: any) {
    console.log("++ Test: " + test_name);
    await func();
    console.log("OK");
}

let PROJECT: EstateProject;

async function setupEstateProject() {
    const projectDir = path.resolve("../..", PROJECT_NAME);
    const projectFile = path.resolve(projectDir, "estate.json");
    if (fs.existsSync(projectFile))
        fs.rmSync(projectFile);

    if (!await executeEstateInitAsync(PROJECT_NAME, projectDir, true, false, true, true, false))
        throw new Error('estate init failed');
    const project = EstateProject.tryOpenFromDir(projectDir);

    if (!project)
        throw new Error('unable to load estate project');

    if (project.singleWorker)
        throw new Error('Unexpectedly single worker project');

    if (!project.singleClient)
        throw new Error('Unexpectedly multi client project');

    PROJECT = project;
}

async function setupWorker(workerName: string) {
    console.log(`Setting up worker ${workerName}..`);

    const workerDir = path.resolve(PROJECT.nWorkerDir, workerName);

    //remove the old worker.json, so I can initialize it
    const workerJsonFile = path.resolve(workerDir,WORKER_CONFIG_FILE_NAME);
    if (fs.existsSync(workerJsonFile))
        fs.rmSync(workerJsonFile);

    if (!await executeWorkerInitAsync(workerName, false, PROJECT))
        throw new Error("init failed");

    //Verify the worker.json files were written
    if (!path.resolve(workerDir, WORKER_CONFIG_FILE_NAME)) {
        throw new Error("worker.json is missing");
    }

    //Deploy the worker
    if (!await executeWorkerDeployAsync(workerName, PROJECT, false, false))
        throw new Error("deploy failed");

    //Ensure the worker is listed in the owned workers
    const ownedWorkers = <string[]>await getOwnedWorkersAsync(true);
    if (!ownedWorkers)
        throw new Error("no owned workers found");
    let found = false;
    for (let ownedWorker of ownedWorkers) {
        if (ownedWorker === workerName)
            found = true;
    }
    if (!found)
        throw new Error(`${workerName} was not found in the list of known workers`);

    // Clean out the .estate directory (containing the code generated worker clients).
    const genDir = path.join(PROJECT.nClientDir, ".estate/generated-clients", workerName);
    if (fs.existsSync(genDir)) {
        fs.rmSync(genDir, {recursive: true});
    }

    //Connect the worker
    if (!await executeWorkerConnectAsync(workerName, CLIENT_NAME,
        true, false, false, false, PROJECT))
        throw new Error("connect failed");
}

async function suite() {
    await test("verify Jayne's health endpoint", async function () {
        // Health endpoint is defined here:
        // build/in/jayne/remote/jayne-deployment.in.yaml
        await fetch(`${JAYNE_SERVICE_URL}`); // should return 200
    })

    await test("verify River's health endpoint", async function () {
        // Health endpoint is defined here:
        // build/in/river/remote/river-deployment.in.yaml
        let url = RIVER_SERVICE_URL;
        url = url.replace("ws:", "http:");
        url = url.replace("wss:", "https:");
        await fetch(url); // should return 200
    })

    await test('create account', async function () {
        const email = process.env.ESTATE_TEST_EMAIL;
        if(!email)
            throw new Error("missing ESTATE_TEST_EMAIL");
        const password = process.env.ESTATE_TEST_PASSWORD;
        if(!password)
            throw new Error("missing ESTATE_TEST_PASSWORD");
        await createNewAccountAsync(LOG_CONTEXT, email, password);
    });

    await test('init estate project', async function () {
        await setupEstateProject();
    });

    await test('worker setup memory cap', async function () {
        await setupWorker(MEMORY_CAP_WORKER_NAME);
    });

    await test('worker setup exercise tracker', async function () {
        await setupWorker(EXERCISE_TRACKER_WORKER_NAME);
    });

    await test('worker setup method and serialization', async function () {
        await setupWorker(METHOD_AND_SERIALIZATION_WORKER_NAME);
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

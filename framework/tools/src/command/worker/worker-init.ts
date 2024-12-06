import path from "path";
import {AdminKeyFile} from "../../model/admin-key-file";
import {logLocalError, logSuccess, logVerbose} from "../../util/logging";
import fs from 'fs';
import {Command} from "commander";
import {WorkerIdentity, WorkerConfig} from "../../model/worker-config";
import {maybeExecuteLoginAsync} from "../account/account-login";
import {EstateProject} from "../../model/estate-project";
import {copyTemplate} from "../../util/template";
import {addNoLoginOption} from "./common-options";
import {validateWorkerNameOrError} from "../../util/validators";
import {writeRuntime} from "../../util/runtime";
import {WORKER_RUNTIME_PACKAGE_NAME} from "../../constants";

export function createWorkerInitCommand(before: any): Command {
    const cmd = new Command('init')
        .argument(`[worker-name]`, "The name of the new worker to initialize. Defaults to the project name if not specified.")
        .description('Initializes a new worker.')
        .action(async (workerName: string, opts: any) => {
            if (before)
                before();
            if (await executeWorkerInitAsync(workerName ?? null, opts.login, null)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });

    return addNoLoginOption(cmd);
}

export async function executeWorkerInitAsync(workerName: string | null, login: boolean, project: EstateProject | null): Promise<boolean> {
    if (!await maybeExecuteLoginAsync(login)) {
        return false;
    }

    //get the key file
    const keyFile = AdminKeyFile.tryOpen();
    if (!keyFile) {
        //already logged
        return false;
    }

    if(!project) {
        project = EstateProject.tryOpenFromCurrentWorkingDirectory('estate worker init');
        if (!project) {
            //already logged
            return false;
        }
    }

    // get the worker name from the project if not specified
    workerName = workerName ?? project.name;

    if(!validateWorkerNameOrError(workerName))
        return false; //already logged

    // make sure one doesn't already exist
    const existingWorker = project.tryOpenWorker(workerName, false);
    if (existingWorker) {
        logLocalError(`A worker by the name of "${workerName}" already exists in the ${existingWorker.loadedFromDir} directory.`);
        return false;
    }

    let workerDir;
    if (project.singleWorker) {
        workerDir = project.nWorkerDir;
    } else {
        workerDir = path.join(project.nWorkerDir, workerName!);
    }

    if(!fs.existsSync(workerDir))
        fs.mkdirSync(workerDir, {recursive: true});

    //Write the worker source config file
    let workerIdentity = new WorkerIdentity(keyFile.adminKey);
    let workerConfig = new WorkerConfig(workerDir, workerName!, [], workerIdentity, null);
    let workerConfigFile = workerConfig.writeToDir(workerDir);
    logVerbose(`Wrote ${workerConfigFile}`);

    const runtimeFiles = writeRuntime(path.resolve(workerDir, WORKER_RUNTIME_PACKAGE_NAME), true, false);
    for(const file of runtimeFiles) {
        logVerbose(`Wrote ${file}`);
    }

    logSuccess(`The "${workerConfig.name}" worker has been initialized in the directory "${workerDir}".`);
    return true;
}
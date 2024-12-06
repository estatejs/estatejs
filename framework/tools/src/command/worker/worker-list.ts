import {Command} from "commander";
import {AdminKeyFile} from "../../model/admin-key-file";
import {logRemoteError} from "../../util/logging";
import {maybeExecuteLoginAsync} from "../account/account-login";
import {JayneSingleton} from "../../service/jayne";
import {createLogContext} from "../../util/log-context";

export function createWorkerListCommand(before: any) : Command {
    return new Command('list')
        .description('Display the list of workers available to your account.')
        .option("--no-login", "Fail when not logged in instead of prompting")
        .action(async (opts: any) => {
            if (before)
                before();

            if (await executeWorkerListAsync(opts.login)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });
}

export async function getOwnedWorkersAsync(login: boolean): Promise<string[] | null> {
    if (!await maybeExecuteLoginAsync(login)) {
        return null;
    }

    //get the admin key file from the login
    let adminKeyFile = AdminKeyFile.tryOpen();
    if (!adminKeyFile) {
        return null;
    }

    const logContext = createLogContext();
    try {
        const response = await JayneSingleton.listWorkersAsync(logContext, adminKeyFile.adminKey);
        if(response.ownedWorkers && response.ownedWorkers.length > 0) {
            return response.ownedWorkers;
        } else {
            console.log("No workers found");
            return null;
        }
    } catch (e: any) {
        logRemoteError(logContext, e.message);
        return null;
    }
}

export async function executeWorkerListAsync(login: boolean): Promise<boolean> {
    const ownedWorkers = await getOwnedWorkersAsync(login);
    if(!ownedWorkers) {
        return true;
    }
    for(let ownedWorker of ownedWorkers) {
        console.log(ownedWorker);
    }
    return true;
}
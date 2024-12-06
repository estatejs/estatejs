import chalk from "chalk";
import {Command} from "commander";
import {logRemoteError, logOk, logSuccess} from "../../util/logging";
import {JayneSingleton} from "../../service/jayne";
import {createLogContext} from "../../util/log-context";
import {executeLoginAsync} from "../account/account-login";
import {AdminKeyFile} from "../../model/admin-key-file";

export function createWorkerDeleteCommand(before: any) : Command {
    return new Command('delete')
        .arguments('<worker-name>')
        .description('Delete the worker from Estate and all its data.')
        .requiredOption("--yes-im-sure-i-want-to-delete-this-worker", "Required because this operation is destructive and cannot be undone (even by Estate support).")
        .action(async (workerName: string) => {
            if (before)
                before();
            if (await executeWorkerDeleteAsync(workerName, true)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });
}

export async function executeWorkerDeleteAsync(workerName: string, login: boolean): Promise<boolean> {
    // Force an interactive login because this is destructive.
    if(login) {
        console.log("");
        console.log(chalk.redBright(`CAUTION: THIS WILL ${chalk.yellowBright("DELETE")} THE WORKER "${chalk.yellowBright(workerName)}" AND ALL ITS DATA.\n\nThis cannot be undone, even by Estate support.`));
        console.log("");
        console.log("Because this is a destructive operation, you must re-login.")
        if (!await executeLoginAsync(null, true, true)) {
            return false;
        }
    }

    //get the admin key file from the login
    let adminKeyFile = AdminKeyFile.tryOpen();
    if (!adminKeyFile) {
        return false;
    }
    let adminKey = adminKeyFile.adminKey;

    const logContext = createLogContext();
    try {
        await JayneSingleton.deleteWorkerAsync(logContext, adminKey, workerName);
        logSuccess(`The "${workerName}" worker has been deleted.`);
        return true;
    } catch (e: any) {
        logRemoteError(logContext, e.message);
        return false;
    }
}
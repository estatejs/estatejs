import {logRemoteError, logOk} from "../../util/logging";
import {AdminKeyFile} from "../../model/admin-key-file";
import {JayneSingleton} from "../../service/jayne";
import {Command} from "commander";
import inquirer from "inquirer";
import {emailValidator} from "../../util/validators";
import {LoginInfo} from "../../model/login-info";
import {loginGoogleIdentity, executeLogoutAsync, executeLoginAsync} from "./account-login";
import {createLogContext} from "../../util/log-context";
import chalk from "chalk";

export function createAccountDeleteCommand(before: any) : Command {
    return new Command('delete')
        .description('Delete your Estate account including all its code and objects.')
        .requiredOption("--yes-im-sure-i-want-to-delete-my-account", "Required because this operation is destructive and cannot be undone (even by Estate support).")
        .action(async () => {
            if (before)
                before();
            const loginInfo = await getLoginInfo();
            if (await executeAccountDeleteAsync(loginInfo)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });
}

function getLoginInfo(): Promise<LoginInfo> {
    console.log("");
    console.log(chalk.redBright(`CAUTION: THIS WILL ${chalk.yellowBright("DELETE")} YOUR ACCOUNT INCLUDING ALL ITS WORKERS AND DATA.\n\nThis cannot be undone, even by Estate support.`));
    console.log("");
    console.log("Because this is a destructive operation, you must re-login.");
    return new Promise<LoginInfo>(resolve => {
        inquirer.prompt([{
            name: 'username',
            message: 'Username (email):',
            validate: emailValidator
        }]).then(usernameAnswer => {
            inquirer.prompt([
                {
                    name: 'password',
                    type: 'password',
                    message: 'Password:',
                    mask: '*'
                }
            ]).then(passwordAnswer => {
                resolve(new LoginInfo(false, usernameAnswer.username, passwordAnswer.password));
            });
        });
    });
}

export async function executeAccountDeleteAsync(loginInfo: LoginInfo): Promise<boolean> {
    const logContext = createLogContext();
    try {
        await loginGoogleIdentity(loginInfo.username, loginInfo.password);

        await JayneSingleton.deleteAccountAsync(logContext);

        await executeLogoutAsync();

        if (AdminKeyFile.exists())
            AdminKeyFile.delete();

        logOk("Estate account deleted");
        return true;
    } catch (e: any) {
        logRemoteError(logContext, e.message);
        return false;
    }
}
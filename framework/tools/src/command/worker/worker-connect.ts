import chalk from "chalk";
import {logIfNotAlready, logRemoteError, logSuccess, logVerbose, logWarning} from "../../util/logging";
import fs from "fs";
import {Command} from "commander";
import {maybeExecuteLoginAsync} from "../account/account-login";
import {JayneSingleton} from "../../service/jayne";
import {AdminKeyFile} from "../../model/admin-key-file";
import {deserializeWorkerIndex} from "../../util/deserializer";
import {createClient, writeTypeDefinitions} from "../../util/client-generator";
import {createLogContext} from "../../util/log-context";
import {addNoLoginOption} from "./common-options";
import {EstateProject} from "../../model/estate-project";
import {
    ClientPackageManager,
    spawnClientPackageInstaller,
    guessClientPackageManager
} from "../../util/shell-package-manager";
import inquirer from "inquirer";
import {requiresAtLeastOneElement, requiresTruthy} from "../../util/requires";
import path from "path";

const CLIENT_LIB_NAME = "warp-client";

function getBuildKeyAndRegenerateCommand(workerName: string): [string, string] {
    let buildKey = `connect-${workerName.toLowerCase()}`;
    return [buildKey, `npm run ${buildKey}`];
}

function updatePackageJson(packageJsonFile: string, workerName: string, clientName: string, updateBuild: boolean, workerPackageName: string, relRootPath: string) {
    const command = `estate worker connect ${workerName} ${clientName} --ci`;

    let packageJsonStr = fs.readFileSync(packageJsonFile, {encoding: "utf8"});
    let indentionRule = getPackageJsonIndentionRule(packageJsonStr);
    let packageJsonObj = JSON.parse(packageJsonStr);

    if (updateBuild) {
        let [buildKey, npmRunCommand] = getBuildKeyAndRegenerateCommand(workerName);

        if (!packageJsonObj.hasOwnProperty("scripts")) {
            packageJsonObj.scripts = {};
        }

        if (!packageJsonObj.scripts.hasOwnProperty(buildKey)) {
            packageJsonObj.scripts[buildKey] = command;
        } else {
            if (packageJsonObj.scripts[buildKey] !== command) {
                packageJsonObj.scripts[buildKey] = command;
            }
        }

        if (!packageJsonObj.scripts.hasOwnProperty("build")) {
            packageJsonObj.scripts.build = "npm run " + buildKey;
        } else {
            if (!packageJsonObj.scripts.build.includes(npmRunCommand)) {
                packageJsonObj.scripts.build = `${npmRunCommand} && ${packageJsonObj.scripts.build}`;
            }
        }
    }

    if (!packageJsonObj.hasOwnProperty("dependencies")) {
        packageJsonObj.dependencies = {};
    }

    packageJsonObj.dependencies[CLIENT_LIB_NAME] = "latest";
    packageJsonObj.dependencies[workerPackageName] = `file:./${relRootPath}`

    const indent = indentionRule.spaces ? indentionRule.count : "\t";
    let updatedPackageJsonStr = JSON.stringify(packageJsonObj, null, indent);
    fs.writeFileSync(packageJsonFile, updatedPackageJsonStr);
}

function getPackageJsonIndentionRule(jsonStr: string): any {
    const def = {spaces: true, count: 2};
    const re = /^(\s*)"name"/m;
    let match = re.exec(jsonStr);
    if (!match) {
        logWarning("Unknown indention found in package.json, using npm default of 2 spaces");
        return def;
    }
    const spaceCount = (match[1].match(/ /g) || []).length;
    const tabCount = (match[1].match(/\t/g) || []).length;
    if (spaceCount && tabCount) {
        logWarning("Inconsistent indention found in package.json, using npm default of 2 spaces");
        return def;
    }
    if (!spaceCount && !tabCount) {
        logWarning("Unknown indention found in package.json, using npm default of 2 spaces");
        return def;
    }

    if (spaceCount)
        return {spaces: true, count: spaceCount};

    if (tabCount)
        return {spaces: false};
}

export function createWorkerConnectCommand(before: any): Command {
    const cmd = new Command('connect')
        .argument('[worker-name]', "The name of the worker to generate code to connect to. Need not be specified in single worker projects. Must be specified in multiple-worker projects.")
        .argument('[client-name]', `The name of the client npm-based project to generate the worker connection code inside of. This is read from the "name" value in the client/package.json or clients/**/package.json files. Doesn't need to be specified in single client projects. Must be specified in multiple-client projects.`)
        .description("Connects a client to a live worker version by generating code.")
        .action(async function (workerName: string | null, clientName: string | null, opts: any) {
            if (before)
                before();

            let login = opts.login;
            let updateBuildScript = opts.updateBuildScript;
            let install = opts.install;
            let syntax = opts.syntax;

            if (opts.ci) {
                login = false;
                updateBuildScript = false;
                install = false;
                syntax = false;
            }

            if (await executeWorkerConnectAsync(
                workerName ?? null,
                clientName ?? null,
                updateBuildScript, login, install, syntax, null)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });

    addNoLoginOption(cmd);

    cmd.option("--syntax", "Output example import syntax after code generation.")
    cmd.option("--no-install", "Don't ask to run client package installation after running connect for the first time on a given client.")
    cmd.option("--no-update-build-script", "Don't update the package.json's build script. You'll need to run 'estate worker connect' manually each time the worker deploys.");
    cmd.option("--ci", "The same as specifying the --no-syntax, --no-login, --no-update-build-script, and --no-install options. This is safe to run in Continuous Integration environments.");
    return cmd;
}

async function promptClientPackageInstallAsync(clientName: string, def: ClientPackageManager): Promise<ClientPackageManager | null> {
    const no = "Do not install client modules";
    const installTypeAnswers = await inquirer.prompt([
        {
            type: 'list',
            name: 'installType',
            message: `Select a package manager:`,
            default: ClientPackageManager[def],
            choices: [
                ClientPackageManager[ClientPackageManager.pnpm],
                ClientPackageManager[ClientPackageManager.yarn],
                ClientPackageManager[ClientPackageManager.npm],
                no
            ]
        }
    ]);

    return installTypeAnswers.installType === no ?
        null :
        ClientPackageManager[installTypeAnswers.installType as keyof typeof ClientPackageManager];
}

function outputImportSyntax(classNames: string[], clientPackageName: string) {
    requiresAtLeastOneElement('classNames', classNames);
    requiresTruthy('clientPackageName', clientPackageName);

    const namesStr = chalk.yellowBright(classNames.slice(0, 3).join(', ')) + chalk.green(' /* ... */)');

    console.log(`${chalk.magentaBright("Usage syntax:")}
 import { worker } from "${chalk.blueBright(CLIENT_LIB_NAME)}";
 import { ${namesStr} } from "${chalk.blueBright(clientPackageName)}";

 ${chalk.green("// Or if you prefer using require:")}
 const { worker } = require("${chalk.blueBright(CLIENT_LIB_NAME)}");
 const { ${namesStr} } = require("${chalk.blueBright(clientPackageName)}");`);
}

export async function executeWorkerConnectAsync(workerName: string | null,
                                              clientName: string | null,
                                              updateBuild: boolean,
                                              login: boolean,
                                              install: boolean,
                                              syntax: boolean,
                                              project: EstateProject | null): Promise<boolean> {
    if (!await maybeExecuteLoginAsync(login)) {
        return false;
    }

    //get the admin key file from the login
    let adminKeyFile = AdminKeyFile.tryOpen();
    if (!adminKeyFile) {
        return false;
    }
    let adminKey = adminKeyFile.adminKey;

    if (!project) {
        project = EstateProject.tryOpenFromCurrentWorkingDirectory('estate worker connect');
        if (!project) {
            //already logged
            return false;
        }
    }

    const clientConfig = project.tryOpenClient(clientName, true);
    if (!clientConfig) {
        return false; //already logged
    }

    const workerConfig = project.tryOpenWorker(workerName, true);
    if (!workerConfig) {
        return false; //already logged
    }

    //get the worker index for the worker name
    const logContext = createLogContext();
    let response;
    try {
        response = await JayneSingleton.getWorkerConnectionInfoAsync(logContext, adminKey, workerConfig.name);
        logVerbose('Worker connection information retrieved from Jayne');
    } catch (e) {
        // @ts-ignore
        logRemoteError(logContext, e.message);
        return false;
    }

    let userKey = response.userKey;
    let workerIndex = deserializeWorkerIndex(response.workerIndexStr);
    logVerbose('Worker index retrieved and validated')

    let regenCommand;
    if (!updateBuild) {
        regenCommand = `estate worker connect ${workerConfig.name} ${clientConfig.name}`;
    } else {
        [, regenCommand] = getBuildKeyAndRegenerateCommand(workerConfig.name);
    }

    const {clientPackageName, clientPackageDir, wasUpdate} =
        createClient(logContext, userKey, workerIndex, regenCommand,
            clientConfig.loadedFromDir);
    logVerbose(`Client worker package code written`);

    const fullClientPackageDir = path.join(clientConfig.loadedFromDir, clientPackageDir);
    await writeTypeDefinitions(response.compressedWorkerTypeDefinitionsStr, fullClientPackageDir);
    logVerbose('Client worker package type definitions written')

    updatePackageJson(clientConfig.configFile, workerConfig.name, clientConfig.name,
        updateBuild, clientPackageName, clientPackageDir);
    logVerbose('Client project updated');

    const between = workerConfig.name === clientConfig.name ?
        `between the worker and client named "${clientConfig.name}".` :
        `between the worker named "${workerConfig.name}" and the client named "${clientConfig.name}".`;

    const classNames: string[] = [];
    for (const clazz of workerConfig.classes)
        classNames.push(clazz.name);

    if (wasUpdate) {
        logSuccess(`An existing connection was updated ${between}`);
    } else {
        if (install) {
            const cpm = guessClientPackageManager(clientConfig.loadedFromDir);
            console.log(chalk.yellowBright(`Module dependencies must be installed using an appropriate package manager.`));
            const packageInstaller = await promptClientPackageInstallAsync(clientConfig.name, cpm);
            if (packageInstaller) {
                spawnClientPackageInstaller(packageInstaller, clientConfig.loadedFromDir);
                logSuccess(`A new connection was established ${between}`)
            } else {
                logSuccess(`A new connection was established ${between}`)
                logWarning("You must install package dependencies manually before trying to use the worker connection code.");
            }
        } else {
            logSuccess(`A new connection was established ${between}`)
            logWarning("You must install package dependencies manually before trying to use the worker connection code.");
        }
    }

    if (syntax)
        outputImportSyntax(classNames, clientPackageName);

    return true;
}

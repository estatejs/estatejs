import {Command} from "commander";
import fs from "fs";
import path from 'path';
import {logDetails, logLocalError, logSuccess} from "../util/logging";
import {maybeExecuteLoginAsync} from "./account/account-login";
import {projectAndWorkerNameValidator, projectNameOrWorkerNameRules, validateProjectNameOrError} from "../util/validators";
import inquirer from "inquirer";
import {CLIENT_DIR_NAME, EstateProject, WORKER_DIR_NAME} from "../model/estate-project";
import {copyTemplate} from "../util/template";
import {EstateConfig} from "../model/estate-config";
import {addNoLoginOption} from "./worker/common-options";
import {showLogo} from "../util/logo";
import {GlobalOptions} from "../util/global-options";
import jszip from "jszip";

async function promptProjectNameAsync(): Promise<string> {
    console.log("Give your new project a name.");
    logDetails(projectNameOrWorkerNameRules);
    const answer = await inquirer.prompt([{
        name: 'projectName',
        message: 'Project name:',
        validate: projectAndWorkerNameValidator
    }]);
    return answer.projectName;
}

export function createEstateInitCommand(before: any): Command {
    const cmd = new Command('init')
        .description('Initialize <directory> as a new Estate project. Directory must be empty or not exist. Empty git repositories are OK.')
        .arguments('<directory>')
        .option("-p,--project <value>", "The project name. Defaults to the directory name. See estate.json to change it after creation.")
        .option("--empty", "Create an empty project.")
        .option("--tutorial", "Create a Estate tutorial project used to teach how to connect a client to a worker.")
        .action(async (dir: string, opts: any) => {
            if (before)
                before();

            if (GlobalOptions.showLogo)
                showLogo();

            if (await executeEstateInitAsync(opts.project, dir,
                opts.empty ?? false, opts.login, false, false, opts.tutorial)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });

    addNoLoginOption(cmd);
    return cmd;
}

function replaceExampleWorkerName(file: string, workerName: string) {
    if (!fs.existsSync(file))
        throw new Error(logLocalError(`${file} does not exist`));
    let code = fs.readFileSync(file, {encoding: "utf8", flag: "r"});
    code = code.split("MY-WORKER-NAME").join(workerName)
    fs.writeFileSync(file, code, {encoding: "utf-8"});
}

async function extractExampleAsync(projectDir: string, exampleZipFile: string) {
    const exampleZip = path.resolve(__dirname, "..", exampleZipFile);
    const content = fs.readFileSync(exampleZip);
    const zip = new jszip();
    const result = await zip.loadAsync(content);
    for(const key of Object.keys(result.files)) {
        const item = result.files[key];
        if(item.dir) {
            const dir = path.join(projectDir, item.name);
            fs.mkdirSync(dir, {recursive: true});
        } else {
            const file = path.join(projectDir, item.name);
            fs.writeFileSync(file, Buffer.from(await item.async('arraybuffer')));
        }
    }
}

async function createEstateProjectAsync(projectName: string, dir: string,
                                      empty: boolean, bare: boolean, tutorial: boolean): Promise<boolean> {
    const config = new EstateConfig(dir, projectName);

    if (!bare) {
        const workerDir = path.resolve(dir, WORKER_DIR_NAME);
        const clientDir = path.resolve(dir, CLIENT_DIR_NAME);

        if (empty) {
            fs.mkdirSync(workerDir, {recursive: true});
            fs.mkdirSync(clientDir, {recursive: true});
        } else {
            if(tutorial) {
                await extractExampleAsync(dir, "tutorial.zip");
            } else {
                await extractExampleAsync(dir, "example.zip");
                
                // update the worker imports
                const componentsDir = path.join(clientDir, "src", "components");
                replaceExampleWorkerName(path.join(componentsDir, "create-exercise.component.js"), projectName);
                replaceExampleWorkerName(path.join(componentsDir, "create-user.component.js"), projectName);
                replaceExampleWorkerName(path.join(componentsDir, "edit-exercise.component.js"), projectName);
                replaceExampleWorkerName(path.join(componentsDir, "exercises-list.component.js"), projectName);
            }
        }

        copyTemplate("worker.gitignore", path.join(workerDir, ".gitignore"));
        const cheatSheet = "cheat-sheet.txt";
        copyTemplate(cheatSheet, path.join(workerDir, cheatSheet));
        copyTemplate("client.gitignore", path.join(clientDir, ".gitignore"));
    }

    config.writeToDir(dir);

    // verify the project can be loaded
    return !!EstateProject.tryOpenFromDir(dir);
}

export async function executeEstateInitAsync(projectName: string, dir: string, empty: boolean, login: boolean,
                                                existsOk: boolean, bare: boolean, tutorial: boolean): Promise<boolean> {
    if (!await maybeExecuteLoginAsync(login)) {
        return false;
    }

    if (dir) {
        dir = path.resolve(dir);
    } else {
        dir = process.cwd();
    }

    if (!projectName) {
        projectName = path.basename(dir);
    }

    if (EstateConfig.fileExists(dir)) {
        logLocalError(`${dir} is already a Estate project.`);
        return false;
    }

    if (!existsOk && fs.existsSync(dir)) {
        const files = fs.readdirSync(dir);
        for (const file of files) {
            let stat = fs.statSync(path.join(dir, file));
            if (!(stat.isDirectory() && file === ".git")) {
                logLocalError(`${dir} isn't empty.`);
                return false;
            }
        }
    }

    if (!projectName) {
        projectName = await promptProjectNameAsync();
    } else {
        if (!validateProjectNameOrError(projectName)) {
            return false;
        }
    }

    if (await createEstateProjectAsync(projectName, dir, empty, bare, tutorial)) {
        logSuccess(`Created the Estate project "${projectName}" in the directory "${dir}".`);
        return true;
    }

    return false;
}
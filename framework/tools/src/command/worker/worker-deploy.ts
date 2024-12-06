import {Command} from "commander";
import {logRemoteError, logLocalError,
    logSuccess, ErrorAlreadyLogged, logIfNotAlready} from "../../util/logging";
import {
    IdentityClassMapping,
    WorkerClassMapping,
    WorkerSrcClassTagMapping,
    WorkerIdentity, WorkerConfig
} from "../../model/worker-config";
import fs from "fs";
import path from "path";
import crc32 from "crc-32";
import {JayneSingleton, WorkerFileContent} from "../../service/jayne";
import {uuidv4} from "../../util/util";
import {createLogContext} from "../../util/log-context";
import {EstateProject} from "../../model/estate-project";
import chalk from "chalk";
import {compileAsync} from "../../util/compiler";
import JSZip from "jszip";
import {recreateDir} from "../../util/loud-fs";
import cliProgress, {SingleBar} from "cli-progress";

function compare(l: WorkerFileContent, r: WorkerFileContent) {
    let comparison = 0;
    if (l.name > r.name) {
        comparison = 1;
    } else if (l.name < r.name) {
        comparison = -1;
    }
    return comparison;
}

function getWorkerFileContent(baseDir: string, dir: string, ext: string[]): WorkerFileContent[] {
    let results: WorkerFileContent[] = [];
    let list = fs.readdirSync(dir);
    const baseFileNames = new Set();
    list.forEach(function (fileName) {
        const pathName = path.join(dir, fileName);
        let stat = fs.statSync(pathName);
        if (stat && stat.isDirectory()) {
            if (fileName !== "worker-runtime")
                results = results.concat(getWorkerFileContent(baseDir, pathName, ext));
        } else {
            if (!ext || ext.includes(path.extname(fileName))) {
                const name = path.relative(baseDir, pathName);
                const baseFileName = path.join(path.parse(name).dir, path.parse(name).name);
                if (baseFileNames.has(baseFileName))
                    throw new Error(`Duplicate file name: ${pathName}. All worker code must have unique file base names (I.e. cannot have a file named foo.js and a file named foo.mjs in the save directory.)`);
                baseFileNames.add(baseFileName);
                const code = fs.readFileSync(pathName, {encoding: "utf8", flag: "r"});
                if (!code || code.trim().length == 0)
                    throw new Error(`Unable to push empty code file: ${pathName}`);
                results.push({
                    name: name,
                    code: code
                });
            }
        }
    });
    return results.sort(compare);
}

function getChecksum(workerName: string, workerDir: string, workerFiles: WorkerFileContent[]): number {
    //note: don't need to know which one changed, just that any of them changed.
    let value = 0;

    value = crc32.bstr(workerName, value);

    workerFiles.forEach(workerFile => {
        //include the filename in the crc to detect filename changes
        value = crc32.bstr(workerFile.name, value); //note: file is already relative to workerDir
        //add the file's contents to the crc to detect file content changes
        value = crc32.bstr(workerFile.code, value);
    });

    return value;
}

export function createWorkerDeployCommand(before: any): Command {
    return new Command('deploy')
        .argument(`[worker-name]`, "The name of the worker to deploy. Need not be specified in single worker projects. Must be specified in multiple-worker projects.")
        .option('--no-progress', "Hide the progress bar")
        .option('--debug', "Don't delete the .build directory after deploy failure.")
        .description('Deploy a worker, creating a new version. Worker clients must run worker connect to use the new version.')
        .action(async (workerName: string | null, opts: any) => {
            if (before)
                before();
            if (await executeWorkerDeployAsync(workerName ?? null, null, opts.progress ?? false, opts.debug ?? false)) {
                process.exitCode = 0;
            } else {
                process.exitCode = 1;
            }
        });
}

// Calculates whether there were any class mapping changes
// note: Class mappings are stored in worker.json and worker.json changes don't change the checksum.
function hasClassMappingChanges(workerConfig: WorkerConfig, classMappings: WorkerClassMapping[]) {
    const prevClassNames = new Set<string>();

    // @ts-ignore
    for (const identityClassMapping of workerConfig.identity.identityClassMappings) {
        prevClassNames.add(identityClassMapping.className);
    }

    let hasDiff = classMappings.length !== workerConfig.identity.identityClassMappings?.length;
    if (!hasDiff) {
        for (const classMapping of classMappings) {
            if (!prevClassNames.has(classMapping.className)) {
                hasDiff = true;
                break;
            }
        }
    }
    return hasDiff;
}

function _zipTypeDefinitionFiles(zip: JSZip, baseDir: string, dir: string) {
    fs.readdirSync(dir).forEach(function (fileName) {
        const pathName = path.join(dir, fileName);
        const relPath = path.relative(baseDir, pathName);
        if (fs.statSync(pathName)?.isDirectory()) {
            _zipTypeDefinitionFiles(zip.folder(relPath)!, baseDir, pathName);
        } else if (fileName.endsWith('.d.ts')) {
            const content = fs.readFileSync(pathName, {encoding: "utf8"});
            zip.file(relPath, content);
        }
    });
}

async function compressTypeDefinitionFilesAsync(rootDir: string): Promise<string | null> {
    const zip = new JSZip();
    _zipTypeDefinitionFiles(zip, rootDir, rootDir);
    return await zip.generateAsync({
        compression: "DEFLATE",
        compressionOptions: {level: 9},
        type: "base64"
    });
}

export async function executeWorkerDeployAsync(workerName: string | null,
                                           project: EstateProject | null,
                                           showProgress: boolean,
                                           debug: boolean): Promise<boolean> {

    //note: does not require login because the admin key is already in worker.json
    let buildDir: string | null = null;
    let complete = false;
    try {
        if (!project) {
            project = EstateProject.tryOpenFromCurrentWorkingDirectory('estate worker deploy');
            if (!project) {
                //already logged
                return false;
            }
        }

        const workerConfig = project.tryOpenWorker(workerName, true);
        if (!workerConfig) {
            //already logged
            return false;
        }

        buildDir = path.join(workerConfig.loadedFromDir, '.build');

        let progress = null;

        if(showProgress) {
            progress = new SingleBar({
                format: `Compiling |` + chalk.blue('{bar}') + '| {percentage}%',
                barCompleteChar: '\u2588',
                barIncompleteChar: '\u2591',
                hideCursor: true
            });
        }

        progress?.start(4, 0);

        recreateDir(buildDir);

        const {schemaDir, declDir} = await compileAsync(progress, buildDir, workerConfig.loadedFromDir, workerConfig.compilerOptions);

        const compressedWorkerTypeDefinitionsStr = await compressTypeDefinitionFilesAsync(declDir);
        if (!compressedWorkerTypeDefinitionsStr) {
            logLocalError('No type definitions emitted from TypeScript');
            return false;
        }

        const schemaFiles = getWorkerFileContent(schemaDir, schemaDir, [".js", ".mjs"]);
        if (!schemaFiles || schemaFiles.length == 0) {
            logLocalError(`No JavaScript files found in ${schemaDir} directory.`);
            return false;
        }

        let checksum = getChecksum(workerConfig.name, schemaDir, schemaFiles);

        const classMappings: WorkerClassMapping[] = [];
        if (workerConfig.classes.length > 0) {
            for (const classTag of workerConfig.classes) {
                let found = false;
                // @ts-ignore
                for (const identityClass of workerConfig.identity.identityClassMappings) {
                    if (identityClass.tag === classTag.tag) {
                        classMappings.push(new WorkerClassMapping(classTag.name, identityClass.classId));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    logLocalError(`Invalid class mapping in ${workerConfig.fullFileName}. The class key ${classTag.name} has an unknown tag. If you're attempting to add a new class, simply delete the extra class key worker that file, write your class, and deploy like normal. The class mappings will be updated automatically.`);
                    return false;
                }
            }
        }

        progress?.stop();

        // if this isn't a new worker (checksum exists) and the checksum matches the current checksum
        if (workerConfig.identity.checksum &&
            workerConfig.identity.checksum === checksum) {
            if (!hasClassMappingChanges(workerConfig, classMappings)) {
                logSuccess("No changes found.");
                return true;
            }
        }

        const logContext = createLogContext();

        try {
            const adminKey = workerConfig.identity.adminKey;

            const response = await JayneSingleton.deployWorkerAsync(
                logContext,
                adminKey,
                workerConfig.name,
                schemaFiles,
                compressedWorkerTypeDefinitionsStr,
                classMappings,
                workerConfig.identity.workerId,
                workerConfig.identity.workerVersion,
                workerConfig.identity.lastClassId
            );

            const classes = [];
            const identityClasses = [];
            let lastClassId = workerConfig.identity.lastClassId ?? 0;
            for (const workerClassMapping of response.workerClassMappings) {
                let tag = null;
                if (workerConfig.identity.identityClassMappings) {
                    for (const prev of workerConfig.identity.identityClassMappings) {
                        if (prev.classId === workerClassMapping.classId) {
                            tag = prev.tag;
                            break;
                        }
                    }
                }
                if (!tag) {
                    tag = uuidv4(false);
                }
                classes.push(new WorkerSrcClassTagMapping(workerClassMapping.className, tag));
                identityClasses.push(new IdentityClassMapping(workerClassMapping.className, workerClassMapping.classId, tag));
                if (workerClassMapping.classId > lastClassId)
                    lastClassId = workerClassMapping.classId;
            }

            workerConfig.classes = classes;
            workerConfig.identity = new WorkerIdentity(adminKey, checksum, response.workerIdStr, response.workerVersionStr, identityClasses, lastClassId);
            workerConfig.writeToDir(workerConfig.loadedFromDir);
            progress?.stop();

            logSuccess(`The "${workerConfig.name}" worker is now ${chalk.yellowBright("live")} at version ${workerConfig.identity.workerVersion}.`);
            complete = true;
            return true;
        } catch (e) {
            // @ts-ignore
            logRemoteError(logContext, e.message);
            return false;
        }
    } catch (e) {
      logIfNotAlready(e);
      return false;
    } finally {
        if (buildDir && fs.existsSync(buildDir)) {
            if(complete || !debug)
            {
                fs.rmSync(buildDir, {recursive: true, force: true});
            }
        }
    }
}
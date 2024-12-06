import fs from "fs";
import path from "path";
import {logLocalError, logVerbose, logWarning} from "../util/logging";
import {requiresTruthy} from "../util/requires";
import {WorkerConfig} from "./worker-config";
import {GlobalOptions} from "../util/global-options";
import {expectDirIfExists, EstateConfig} from "./estate-config";
import {ClientConfig} from "./client-config";
import os from "os";
import {validateWorkerNameOrError} from "../util/validators";

export const WORKERS_DIR_NAME = "workers";
export const WORKER_DIR_NAME = "worker";
export const CLIENTS_DIR_NAME = "clients";
export const CLIENT_DIR_NAME = "client";

export class EstateProject {
    constructor(public readonly nWorkerDir: string,
                public readonly singleWorker: boolean,
                public readonly nClientDir: string,
                public readonly singleClient: boolean,
                private readonly config: EstateConfig) {
        requiresTruthy('nWorkerDir', nWorkerDir);
        requiresTruthy('nClientDir', nClientDir);
        requiresTruthy('config', config);
    }

    get name() {
        return this.config.name;
    }

    tryOpenWorker(workerName: string | null, log: boolean): WorkerConfig | null {
        if (this.singleWorker) {
            if (WorkerConfig.fileExists(this.nWorkerDir)) {
                const workerConfig = WorkerConfig.tryOpen(this.nWorkerDir);
                if (workerConfig) {
                    if (workerName && workerConfig.name !== workerName) {
                        if (log)
                            logLocalError(`Worker not found by that name. Did you mean "${workerConfig.name}"?`);
                        return null;
                    } else {
                        if (log)
                            logVerbose(`Found worker ${workerConfig.name} in ${workerConfig.loadedFromDir}`);
                        return workerConfig;
                    }
                }
            } else {
                if (log)
                    logLocalError('No worker found. Has it been initialized?');
                return null;
            }
        } else {
            /*multiple workers*/
            const folders = fs.readdirSync(this.nWorkerDir);
            if (!folders) {
                if (log)
                    logLocalError('No worker found. Has it been initialized?');
                return null;
            }

            const hasSingleWorker = folders.length === 1;
            if (!workerName && !hasSingleWorker) {
                if (log)
                    logLocalError('Worker name must be specified when there are more than a single worker.')
                return null;
            }

            for (const folder of folders) {
                const workerDir = path.join(this.nWorkerDir, folder);

                const stat = fs.statSync(workerDir);
                if (!stat.isDirectory()) {
                    logWarning(`Loose file found: ${workerDir}`)
                    continue;
                }

                if (!WorkerConfig.fileExists(workerDir)) {
                    logWarning(`Extra directory found: ${workerDir}`);
                    continue;
                }

                if (!workerName) {
                    //if there's only one worker and the worker name wasn't specified, just open it
                    //note: it won't get here if there's more than one worker and the worker name wasn't specified
                    return WorkerConfig.tryOpen(workerDir);
                }

                const compName = WorkerConfig.tryGetWorkerName(workerDir);
                if (!compName) {
                    logWarning(`Worker found without a name in ${workerDir}`);
                    continue;
                }

                if (compName !== folder && GlobalOptions.showNameWarnings) {
                    logWarning(`Worker "${compName}" name differs from its folder name "${folder}" in ${this.nWorkerDir}`);
                }

                if (compName === workerName) {
                    if (log)
                        logVerbose(`Found worker ${workerName} in ${workerDir}`);
                    return WorkerConfig.tryOpen(workerDir);
                }

                if (compName !== workerName && compName.toLowerCase() === workerName?.toLowerCase()) {
                    if (log)
                        logLocalError(`Worker not found by that name. Did you mean "${compName}"?`);
                    return null;
                }
            }
        }

        if (log)
            logLocalError('Worker not found. Has one been initialized?');
        return null;
    }

    tryOpenClient(clientName: string | null, log: boolean): ClientConfig | null {
        if (this.singleClient) {
            if (ClientConfig.fileExists(this.nClientDir)) {
                const clientConfig = ClientConfig.tryOpen(this.nClientDir);
                if (clientConfig) {
                    if (clientName && clientConfig.name !== clientName) {
                        if (log)
                            logLocalError(`Client not found by that name. Did you mean "${clientConfig.name}"?`);
                        return null;
                    } else {
                        if (log)
                            logVerbose(`Found client ${clientConfig.name} in ${clientConfig.loadedFromDir}`);
                        return clientConfig;
                    }
                }
            } else {
                if (log)
                    logLocalError('Client not found. Has it been initialized?');
                return null;
            }
        } else {
            /*multiple clients*/
            const folders = fs.readdirSync(this.nClientDir);
            if (!folders) {
                if (log)
                    logLocalError('Client not found. Has it been initialized?');
                return null;
            }

            const hasSingleClient = folders.length === 1;
            if (!clientName && !hasSingleClient) {
                if (log)
                    logLocalError('Client name must be specified when there are more than one client.')
                return null;
            }

            for (const folder of folders) {
                const clientDir = path.join(this.nClientDir, folder);

                const stat = fs.statSync(clientDir);
                if (!stat.isDirectory()) {
                    logWarning(`Loose file found: ${clientDir}`)
                    continue;
                }

                if (!ClientConfig.fileExists(clientDir)) {
                    logWarning(`Extra directory found: ${clientDir}`);
                    continue;
                }

                if (!clientName) {
                    //if there's only one client and the client name wasn't specified, just open it
                    //note: it won't get here if there's more than one client and the client name wasn't specified
                    return ClientConfig.tryOpen(clientDir);
                }

                const compName = ClientConfig.tryGetClientName(clientDir);
                if (!compName) {
                    logWarning(`Client found without a name in ${clientDir}`);
                    continue;
                }

                if (compName !== folder && GlobalOptions.showNameWarnings) {
                    logWarning(`Client "${compName}" name differs from its folder name "${folder}" in ${this.nClientDir}`);
                }

                if (compName === clientName) {
                    if (log)
                        logVerbose(`Found client ${clientName} in ${clientDir}`);
                    return ClientConfig.tryOpen(clientDir);
                }

                // Note: I realize that node.js packages can't have upper case letters but that's
                // their limitation and not something I can ensure they're enforcing. So, because the
                // above check is case-sensitive, this needs to see if the casing is (for some odd reason)
                // the only reason why the check failed. (sj)
                if (compName !== clientName && compName.toLowerCase() === clientName?.toLowerCase()) {
                    if (log)
                        logLocalError(`Client not found by that name. Did you mean "${compName}"?`);
                    return null;
                }
            }
        }

        if (log)
            logLocalError('Client not found. Has a client been initialized?');
        return null;
    }

    static tryOpenFromDir(dir: string): EstateProject | null {
        // load the config
        const config = EstateConfig.tryOpen(dir);
        if (!config) {
            return null; //already logged
        }

        const multiWorkerDir = expectDirIfExists(path.join(config.loadedFromDir, WORKERS_DIR_NAME));
        const singleWorkerDir = expectDirIfExists(path.join(config.loadedFromDir, WORKER_DIR_NAME));
        if (multiWorkerDir && singleWorkerDir) {
            logLocalError(`Invalid project: cannot contain both '${WORKERS_DIR_NAME}' and '${WORKER_DIR_NAME}' directories.`);
            return null;
        }
        if (!multiWorkerDir && !singleWorkerDir) {
            logLocalError(`Invalid project: missing '${WORKERS_DIR_NAME}' or '${WORKER_DIR_NAME}' directory.`);
            return null;
        }

        const multiClientDir = expectDirIfExists(path.join(config.loadedFromDir, CLIENTS_DIR_NAME));
        const singleClientDir = expectDirIfExists(path.join(config.loadedFromDir, CLIENT_DIR_NAME));
        if (multiClientDir && singleClientDir) {
            logLocalError(`Invalid project: cannot contain both '${CLIENTS_DIR_NAME}' and '${CLIENT_DIR_NAME} directories.`);
            return null;
        }
        if (!multiClientDir && !singleClientDir) {
            logLocalError(`Invalid project: missing '${CLIENTS_DIR_NAME}' or '${CLIENT_DIR_NAME}' directory.`);
            return null;
        }

        const nWorkerDir = multiWorkerDir ?? singleWorkerDir;
        const nClientDir = multiClientDir ?? singleClientDir;

        return new EstateProject(
            nWorkerDir!,
            !!singleWorkerDir,
            nClientDir!,
            !!singleClientDir,
            config);
    }

    static tryOpenFromCurrentWorkingDirectory(forCommand: string | null): EstateProject | null {
        // find the correct config dir
        const configDir = findEstateConfigDir(process.cwd());
        if (!configDir) {
            let forCmdMsg = forCommand ? `. Command '${forCommand}' must be executed inside a Estate project directory structure.` : '';
            logLocalError('Project not found' + forCmdMsg);
            return null;
        }
        return EstateProject.tryOpenFromDir(configDir);
    }
}

function findEstateConfigDir(dir: string): string | null {
    if (EstateConfig.fileExists(dir)) {
        return dir;
    }

    const parent = path.dirname(dir);

    if (parent === os.homedir())
        return null;

    if (parent && parent !== dir)
        return findEstateConfigDir(parent);
    return null;
}
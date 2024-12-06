import {requiresTruthy} from "../util/requires";
import fs from "fs";
import path from "path";
import {BaseConfig} from "./base-config";
import {logLocalError} from "../util/logging";
import {projectAndWorkerNameValidator} from "../util/validators";
import {decodeBase64, encodeBase64} from "../util/base64";
import {validateConfigVersion} from "../util/config-validator";
import {parseJsonFromFile} from "../util/json-parse";
import {CompilerOptions} from "../util/compiler";

export const WORKER_CONFIG_FILE_NAME = "worker.json";
const WORKER_CONFIG_VERSION = 1;

export class WorkerClassMapping {
    constructor(public className: string, public classId: number) {
    }
}

export class WorkerSrcClassTagMapping {
    constructor(public name: string, public tag: string) {
    }
}

export class IdentityClassMapping {
    constructor(public className: string, public classId: number, public tag: string) {
    }
}

export class WorkerIdentity {
    constructor(public adminKey: string,
                public checksum?: number,
                public workerId?: string,
                public workerVersion?: string,
                public identityClassMappings?: IdentityClassMapping[],
                public lastClassId?: number) {
        requiresTruthy('adminKey', adminKey);
        if(checksum || workerId || workerVersion) {
            requiresTruthy('checksum', checksum);
            requiresTruthy('workerId', workerId);
            requiresTruthy('workerVersion', workerVersion);
            requiresTruthy('identityClassMappings', identityClassMappings);
            requiresTruthy('lastClassId', lastClassId);
        }
    }
}

function getConfigFile(dir: string) : string {
    return path.resolve(dir, WORKER_CONFIG_FILE_NAME);
}

export class WorkerConfig extends BaseConfig {
    private readonly configVersion: number;
    constructor(public loadedFromDir: string,
                public name: string,
                public classes: WorkerSrcClassTagMapping[],
                public identity: WorkerIdentity,
                public compilerOptions: CompilerOptions | null) {
        super();
        requiresTruthy('loadedFromDir', loadedFromDir);
        requiresTruthy('name', name);
        requiresTruthy('identity', identity);
        this.configVersion = WORKER_CONFIG_VERSION;
    }

    public get fullFileName() {
        return path.join(this.loadedFromDir, WORKER_CONFIG_FILE_NAME);
    }

    public writeToDir(dir: string) : string {
        if (!fs.existsSync(dir))
            throw new Error(`${dir} does not exist`);
        let file = getConfigFile(dir);

        let obj = {
            identity: encodeBase64(JSON.stringify(this.identity)),
            name: this.name,
            configVersion: this.configVersion,
            classes: this.classes,
            // I want to take a snapshot of the current default compiler options when the
            // worker is first initialized and use those settings throughout their worker development.
            // Basically, I only want to apply the defaults the very first time this file is written.
            compilerOptions: this.compilerOptions ??
                new CompilerOptions(),
        }

        this.writeToFile(obj, file);

        return file;
    }

    public static fileExists(dir: string) {
        return fs.existsSync(getConfigFile(dir));
    }

    public static delete(dir: string) {
        fs.unlinkSync(getConfigFile(dir));
    }

    public static tryGetWorkerName(dir: string) : string | null {
        let fileObj = tryOpenWorkerConfigFile(getConfigFile(dir));
        return fileObj?.name || null;
    }

    public static tryOpen(dir: string): WorkerConfig | null {
        const configFile = getConfigFile(dir)

        let fileObj = tryOpenWorkerConfigFile(configFile);
        if(!fileObj)
            return null; //already logged

        if(!validateConfigVersion(fileObj, configFile, WORKER_CONFIG_VERSION))
            return null; //already logged

        if (!fileObj.hasOwnProperty("name") ||
            !fileObj.hasOwnProperty("identity") ||
            !fileObj.hasOwnProperty('classes')) {
            logLocalError(`Invalid or corrupt ${configFile}`);
            return null;
        }

        if (!fileObj.name || !projectAndWorkerNameValidator(fileObj.name)) {
            logLocalError(`Invalid worker name in ${configFile}`);
            return null;
        }

        if(!fileObj.identity) {
            logLocalError(`Invalid worker identity in ${configFile}`);
            return null;
        }

        if(!fileObj.classes) {
            logLocalError(`Invalid worker class mappings in ${configFile}`);
            return null;
        }

        return new WorkerConfig(dir,
            fileObj.name,
            fileObj.classes,
            JSON.parse(decodeBase64(fileObj.identity)),
            fileObj.compilerOptions);
    }
}

function tryOpenWorkerConfigFile(file: string) : any {
    if (!fs.existsSync(file)) {
        logLocalError(`${file} does not exist`);
        return;
    }
    return parseJsonFromFile(file);
}
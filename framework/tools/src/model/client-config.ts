import {requiresTruthy} from "../util/requires";
import fs from "fs";
import path from "path";
import {logLocalError} from "../util/logging";
import {clientNameValidator} from "../util/validators";
import {parseJsonFromFile} from "../util/json-parse";

export const CLIENT_CONFIG_FILE_NAME = "package.json";

function getConfigFile(dir: string) {
    return path.resolve(dir, CLIENT_CONFIG_FILE_NAME);
}

export class ClientConfig {
    constructor(public loadedFromDir: string,
                public name: string) {
        requiresTruthy('loadedFromDir', loadedFromDir);
        requiresTruthy('name', name);
    }

    public get configFile() {
        return getConfigFile(this.loadedFromDir);
    }

    public static fileExists(dir: string) {
        return fs.existsSync(getConfigFile(dir));
    }

    public static tryGetClientName(dir: string) : string | null {
        let fileObj = tryOpenClientConfigFile(getConfigFile(dir));
        return fileObj?.name || null;
    }

    public static tryOpen(dir: string): ClientConfig | null {
        const file = getConfigFile(dir);

        let fileObj = tryOpenClientConfigFile(file);
        if(!fileObj)
            return null; //already logged

        if (!fileObj.hasOwnProperty("name")) {
            logLocalError(`Invalid or corrupt ${file}`);
            return null;
        }

        if (!fileObj.name || !clientNameValidator(fileObj.name)) {
            logLocalError(`Invalid client name in ${file}`);
            return null;
        }

        return new ClientConfig(dir, fileObj.name);
    }
}

function tryOpenClientConfigFile(file: string) : any {
    if (!fs.existsSync(file)) {
        logLocalError(`${file} does not exist`);
        return;
    }
    return parseJsonFromFile(file);
}
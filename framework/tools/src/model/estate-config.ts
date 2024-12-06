import fs from "fs";
import {logLocalError} from "../util/logging";
import {BaseConfig} from "./base-config";
import {requiresTruthy} from "../util/requires";
import path from "path";
import {projectAndWorkerNameValidator} from "../util/validators";
import os from "os";
import {validateConfigVersion} from "../util/config-validator";
import {parseJsonFromFile} from "../util/json-parse";

const ESTATE_PROJECT_CONFIG_FILE_VERSION = 1;
const ESTATE_PROJECT_CONFIG_FILE_NAME = "estate.json";

export function expectDirIfExists(dir: string): string | null {
    if (fs.existsSync(dir)) {
        const stat = fs.statSync(dir);
        if (!stat.isDirectory())
            throw new Error(logLocalError(`Expected directory ${dir} but file found instead`));
        return dir;
    }
    return null;
}

export class EstateConfig extends BaseConfig {
    private version: number;

    constructor(
        public loadedFromDir: string,
        public name: string) {
        super();
        requiresTruthy('loadedFromDir', loadedFromDir);
        requiresTruthy('name', name);
        this.version = ESTATE_PROJECT_CONFIG_FILE_VERSION;
    }

    public writeToDir(dir: string): string {
        if (!fs.existsSync(dir))
            throw new Error(`${dir} does not exist`);

        let file = path.resolve(dir, ESTATE_PROJECT_CONFIG_FILE_NAME);

        if (!this.name) {
            throw new Error(logLocalError('Cannot store estate configuration without name.'));
        }

        this.writeToFile(
            {
                name: this.name,
                configVersion: ESTATE_PROJECT_CONFIG_FILE_VERSION
            }, file);

        return file;
    }

    public static fileExists(dir: string) {
        return fs.existsSync(path.join(dir, ESTATE_PROJECT_CONFIG_FILE_NAME));
    }

    public static delete(dir: string) {
        fs.unlinkSync(path.join(dir, ESTATE_PROJECT_CONFIG_FILE_NAME));
    }

    public static tryOpen(dir: string): EstateConfig | null {
        requiresTruthy('dir', dir);
        dir = path.normalize(dir);

        let file = path.resolve(dir, ESTATE_PROJECT_CONFIG_FILE_NAME);
        if (!fs.existsSync(file)) {
            logLocalError(`${ESTATE_PROJECT_CONFIG_FILE_NAME} does not exist in ${dir}`);
            return null;
        }

        let fileObj = parseJsonFromFile(file);
        if(!fileObj)
            return null; //already logged

        if(!validateConfigVersion(fileObj, file,
            ESTATE_PROJECT_CONFIG_FILE_VERSION))
            return null; //already logged

        if (!fileObj.hasOwnProperty("name")) {
            logLocalError(`Invalid or corrupt ${file}`);
            return null;
        }

        if (!fileObj.name || !projectAndWorkerNameValidator(fileObj.name)) {
            logLocalError(`Invalid name value in ${file}`);
            return null;
        }

        return new EstateConfig(dir, fileObj.name);
    }
}
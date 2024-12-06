import {logLocalError} from "./logging";

export function validateConfigVersion(fileObj: any, configFileName: string, currentVersion: number) : boolean {
    if (!fileObj || !fileObj.hasOwnProperty("configVersion")) {
        logLocalError(`Invalid or corrupt ${configFileName}`);
        return false;
    }

    if(!fileObj.configVersion) {
        logLocalError(`Invalid configVersion value in ${configFileName}`);
        return false;
    }

    if(fileObj.configVersion !== currentVersion) {
        logLocalError(`Incompatible project configVersion ${fileObj.version}.`);
        return false;
    }

    return true;
}
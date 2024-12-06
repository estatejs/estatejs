import {logDetails, logLocalError} from "./logging";

export const emailValidator = async (input: any) => {
    const re = /^(([^<>()\[\]\\.,;:\s@"]+(\.[^<>()\[\]\\.,;:\s@"]+)*)|(".+"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;
    return re.test(String(input).toLowerCase());
};

export const clientNameValidator = (input: any) => {
    const re = /^(@[a-z0-9-~][a-z0-9-._~]*\/)?[a-z0-9-~][a-z0-9-._~]*$/;
    return re.test(String(input));
}

// NOTE: project and worker name have the same constraints.
// project and worker names are a strict subset of the npm package name constraints.
// This means that all workers/project names will work as npm package names but not the other way around.

export const projectAndWorkerNameValidator = (input: any) => {
    const re = /^[a-z][a-z0-9\-_]{2,49}$/;
    return re.test(String(input));
}

export const projectNameOrWorkerNameRules = "\nThe name must consist of 3 to 50 characters and be entirely lowercase. It must begin with a letter. After the first letter, it must only contain letters, numbers, and the characters - (hyphen), and _ (underscore).\n" +
    "\nGood:\tmy-cool-project\n\tjanes-grill\n" +
    "\nBad:\t1my-project (first letter)\n\tPlanning@Project (upper-case letters, @ symbol)\n\tet (too short)";

export function projectNameOrWorkerNameErrorMessage(isProject: boolean, name: string) : string {
    return `The ${isProject ? 'project' : 'worker'} name "${name}" is invalid.`;
}

export function validateProjectNameOrError(projectName: string) : boolean {
    if (!projectAndWorkerNameValidator(projectName)) {
        logLocalError(projectNameOrWorkerNameErrorMessage(true, projectName));
        logDetails(projectNameOrWorkerNameRules, true);
        return false;
    }
    return true;
}

export function validateWorkerNameOrError(workerName: string | null) : boolean {
    if (!projectAndWorkerNameValidator(workerName)) {
        logLocalError(projectNameOrWorkerNameErrorMessage(false, workerName!));
        logDetails(projectNameOrWorkerNameRules, true);
        return false;
    }
    return true;
}

export const passwordValidator = (input: any) => {
    if (!input)
        return false;

    if (typeof input !== 'string')
        return false;

    return input.length >= 8;
};
export class PlatformError extends Error {
    constructor(message: string, public code: string) {
        super(message);
        this.code = code;
    }
}

export class ScriptError extends Error {
    constructor(message: string, public workerStack: string) {
        super(message);
        this.workerStack = workerStack;
    }
}
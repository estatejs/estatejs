export class WorkerOptions {
    constructor(public enableVerboseLogging: boolean = true,
                public enableMessageTracing: boolean = true,
                public debugLogHeader: string = "[estate]") {
    }
}
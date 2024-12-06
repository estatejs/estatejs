import {Command} from "commander";
import {createWorkerDeployCommand} from "./worker-deploy";
import {createWorkerConnectCommand} from "./worker-connect";
import {createWorkerDeleteCommand} from "./worker-delete";
import {createWorkerInitCommand} from "./worker-init";
import {createWorkerListCommand} from "./worker-list";

export function createWorkerCommand(start: any) : Command {
    return new Command('worker')
        .storeOptionsAsProperties(false)
        .description("Worker Management")
        .addCommand(createWorkerDeployCommand(start))
        .addCommand(createWorkerConnectCommand(start))
        .addCommand(createWorkerDeleteCommand(start))
        .addCommand(createWorkerInitCommand(start))
        .addCommand(createWorkerListCommand(start))
}

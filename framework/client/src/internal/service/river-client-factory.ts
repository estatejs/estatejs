import {sysLogError} from "../util/logging.js";
import {openRiverClientAsync, RiverClient} from "./river-client.js";
import {WorkerRegistry} from "../util/registry.js";
import {
    handleMessageMessageWithTrackerAsync,
    handleDataUpdateMessageWithTrackerAsync
} from "../util/update-handler.js";
import {WorkerKey} from "../internal-model-types.js";

export async function getRiverClientAsync(workerKey: WorkerKey): Promise<RiverClient> {
    let riverClient = WorkerRegistry.instance.tryGetRiverClient(workerKey);
    if (riverClient)
        return riverClient;

    let userKey = WorkerRegistry.instance.getUserKey(workerKey);

    try {
        riverClient = await openRiverClientAsync(workerKey, userKey, handleDataUpdateMessageWithTrackerAsync, handleMessageMessageWithTrackerAsync);
    } catch (reason) {
        throw new Error(sysLogError(`Unable to open connection to Estate. Reason: ${JSON.stringify(reason)}`));
    }

    WorkerRegistry.instance.setRiverClient(workerKey, riverClient);
    return riverClient;
}
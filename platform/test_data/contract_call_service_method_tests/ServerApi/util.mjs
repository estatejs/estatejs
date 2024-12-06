import {createUuid} from "./worker-runtime";

export function getUniqueId() {
    return createUuid(true);
}

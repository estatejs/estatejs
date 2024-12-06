const crypto = require("crypto");

const ESTATE_LOG_CONTEXT_LENGTH = 10;

export function createLogContext() {
    return crypto.randomBytes(ESTATE_LOG_CONTEXT_LENGTH / 2).toString('hex');
}
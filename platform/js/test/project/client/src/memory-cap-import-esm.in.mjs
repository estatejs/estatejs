import {MemoryCapService} from "memory-cap-worker";
import {worker, WorkerOptions, PlatformError} from "warp-client";
import * as t from "./single-runner.js";
const {test, run, setLogPrefix, getLogPrefix} = t;
import * as a from "./assert.js";
const {assert} = a;

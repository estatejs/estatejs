import {Metadata, TestService, OtherService} from "method-and-serialization-worker";
import {worker, WorkerOptions} from "warp-client";
import * as t from "./single-runner.js";
const {test, run, setLogPrefix, getLogPrefix} = t;
import * as a from "./assert.js";
const {assert} = a;

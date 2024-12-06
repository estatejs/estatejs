const {Metadata, TestService, OtherService} = require("method-and-serialization-worker");
const {worker, WorkerOptions} = require("warp-client");
const t = require("./single-runner.js");
const {test, run, setLogPrefix, getLogPrefix} = t;
const a = require("./assert.js");
const {assert} = a;

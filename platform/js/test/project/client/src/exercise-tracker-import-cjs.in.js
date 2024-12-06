const {Exercise, ExerciseAdded, ExerciseTrackerService, User} = require("exercise-tracker-worker");
const {worker, WorkerOptions, createUuid} = require("warp-client");
const {test, run, barrierWaitAsync, setLogPrefix, getLogPrefix} = require("./shared-runner.js");
const {assert} = require("./assert.js");
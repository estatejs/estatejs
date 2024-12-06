import {Exercise, ExerciseAdded, ExerciseTrackerService, User} from "exercise-tracker-worker";
import {worker, WorkerOptions, createUuid} from "warp-client";
import * as t from "./shared-runner.js";
const {test, run, barrierWaitAsync, setLogPrefix, getLogPrefix} = t;
import * as a from "./assert.js";
const {assert} = a;
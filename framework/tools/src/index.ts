#!/usr/bin/env node
import {executeEstate} from "./command/estate";
import {logIfNotAlready} from "./util/logging";

function main() {
    try{
        executeEstate();
    }
    catch(e) {
        logIfNotAlready(e);
    }
}

main();

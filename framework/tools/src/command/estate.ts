import {GlobalOptions} from "../util/global-options";
import {showLogo} from "../util/logo";
import {showServiceWarning} from "../util/service-warning";
import {createAccountCommand} from "./account/account";
import {createEstateInitCommand} from "./estate-init";
import {createWorkerCommand} from "./worker/worker";
import {program} from "commander";

const {version} = require("../package.json");

function createEstateOptions() {
    program.option('--no-logo', "Don't show the Estate logo on start");
    program.on('option:no-logo', function() {
        GlobalOptions.showLogo = false;
    });

    program.option('--version', "Output the version of estate-tools and exit.")
    program.on('option:version', function () {
        console.log(version);
        process.exit(0);
    });

    program.option('-v, --verbose', "Show verbose messages");
    program.on('option:verbose', function () {
        GlobalOptions.verbose = true;
    });

    program.option('--quiet', "Don't show any success/ok messages (implies --no-logo).");
    program.on('option:quiet', function () {
        GlobalOptions.showLogo = false;
        GlobalOptions.quiet = true;
        GlobalOptions.showServiceWarning = false;
    });
}

export function executeEstate() {
    createEstateOptions();

    let start = (showLogo: boolean = false) => {
        if(GlobalOptions.showServiceWarning)
            showServiceWarning();
    }

    //estate account <subcommand>
    program.addCommand(createAccountCommand(start));

    //estate worker <subcommand>
    program.addCommand(createWorkerCommand(start));

    //estate init
    program.addCommand(createEstateInitCommand(start));

    program.parse(process.argv);
}
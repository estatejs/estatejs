import {Command} from "commander";

export function addNoLoginOption(command: Command) : Command {
    return command.option("--no-login", "Fail when not logged in instead of prompting");
}
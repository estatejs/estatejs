import {Command} from "commander";
import {createAccountDeleteCommand} from "./account-delete";
import {createAccountLoginCommand} from "./account-login";

export function createAccountCommand(start: any) : Command {
    return new Command('account')
        .storeOptionsAsProperties(false)
        .description("Developer Account Management")
        .addCommand(createAccountDeleteCommand(start))
        .addCommand(createAccountLoginCommand(start))
}

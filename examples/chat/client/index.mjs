import inquirer from 'inquirer';
import chalk from "chalk";

import { worker, WorkerOptions } from "warp-client";
import { ChatServer, ChatTextSaid } from "chat-worker";

//hide the normally verbose logging
worker.setOptions(new WorkerOptions(false, false));

const chatServer = worker.getService(ChatServer, "Scott's No-kill Animal Shelter");

console.log(`Welcome to ${chalk.blueBright(chatServer.primaryKey)}!`)
console.log(chalk.grey(`The current server time is ${await chatServer.getTimeAsync()}.`));

const args = process.argv.slice(2);
if (args[0] === "--console") {
    console.log("Press Ctrl+C to exit.")
    const history = await chatServer.getHistoryAsync();
    if (history && history.length) {
        for (const chatText of history)
            outputChatText(chatText);
    }
    await worker.subscribeMessageAsync(chatServer, ChatTextSaid, (chatTextSaid) => {
        outputChatText(chatTextSaid.chatText);
    });
} else {
    await inquirer.prompt([{
        type: 'input',
        name: 'userName',
        message: "Login: "
    }]).then(async ({userName}) => {
        console.log("Use /quit to exit chat.");
        while (true) {
            await inquirer.prompt([{
                type: 'input',
                name: 'text',
                message: `${userName}: `
            }]).then(async ({text}) => {
                if (text === "/quit") {
                    process.exit(0);
                    return;
                }
                await chatServer.sayAsync(userName, text);
            })
        }
    })
}

function outputChatText(chatText) {
    const timestamp = chalk.grey(`[${chatText.timestamp.toLocaleTimeString()}]`);
    const userName = chalk.whiteBright(`[${chatText.userName}]:`);
    const text = chalk.greenBright(chatText.text);
    console.log(`${timestamp} ${userName} ${text}`);
}
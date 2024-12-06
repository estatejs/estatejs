import chalk from "chalk";
import * as figlet from "figlet";
export {Fonts} from "figlet";

export const DefaultFont: figlet.Fonts = "Doom";

export function showLogo(font: figlet.Fonts = DefaultFont) {
    //display the header
    console.log(chalk.blueBright(
        figlet.textSync('Estate', font)
    ));
    console.log(chalk.gray("Copyright (c) 2023 Warpdrive Technologies, Inc. All Rights Reserved."));
    console.log()
}
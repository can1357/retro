/**
 * Check configs.js
 * =====================
 * Check if configs/config.js exist, if don't exist rename .tpl
 *
 * @contributors: Retro
 *
 * @license: MIT License
 *
 */
import * as fs from "fs";
import * as shell from "shelljs";

declare const __dirname: string;

const path = `${__dirname}/../app/configs/config.js`;

if (!fs.existsSync(path)) {
	shell.cp("-Rf", `${__dirname}/../app/configs/config.js.tpl`, path);
}

/**
 * Delete dist folder
 * =====================
 *
 * @contributors: Retro
 *
 * @license: MIT License
 *
 */
import * as shell from "shelljs";
declare const __dirname: string;

const path_dist = `${__dirname}/../dist`;
const path_build = `${__dirname}/../build`;

shell.rm("-Rf", path_dist);
shell.rm("-Rf", path_build);

/**
 * Routes
 * =====================
 * All app routes
 *
 * @contributors: Retro
 *
 * @license: MIT License
 *
 */
import Home from "@app/pages/home/home.svelte";
import Wild from "@app/pages/wild/wild.svelte";
import NotFound from "@app/pages/404/404.svelte";

export default {
	"/": Home,
	"/wild": Wild,
	"/wild/*": Wild,
	"*": NotFound,
};

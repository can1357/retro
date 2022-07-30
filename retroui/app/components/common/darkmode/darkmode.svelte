<script lang="ts">
	/**
	 * Dark Mode HTML
	 * =====================
	 *
	 * @contributors: Retro
	 *
	 * @license: MIT License
	 *
	 */
	import { onMount } from "svelte";
	import { darkModeToggle, darkModeDetect, darkmode } from "./darkmode";

	let checked = darkmode;

	onMount(async () => {
		checked = darkModeDetect();
	});
</script>

<div class="field">
	<input id="dark-mode" type="checkbox" name="dark-mode" class="switch" on:click={() => darkModeToggle()} {checked} />
	<label for="dark-mode" />
</div>

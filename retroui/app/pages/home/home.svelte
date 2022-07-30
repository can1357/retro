<script>
	import "carbon-components-svelte/css/all.css";
	let open = false;

	let theme = "g90"; // "white" | "g10" | "g80" | "g90" | "g100"
	$: document.documentElement.setAttribute("theme", theme);

	import { Button, Modal, ButtonSet, InlineLoading } from "carbon-components-svelte";
	import { onDestroy } from "svelte";

	const descriptionMap = {
		active: "Submitting...",
		finished: "Success",
		inactive: "Cancelling...",
	};

	const stateMap = {
		active: "finished",
		inactive: "dormant",
		finished: "dormant",
	};

	let timeout = undefined;
	let state = "dormant"; // "dormant" | "active" | "finished" | "inactive"

	function reset(incomingState) {
		if (typeof timeout === "number") {
			clearTimeout(timeout);
		}

		if (incomingState) {
			timeout = setTimeout(() => {
				state = incomingState;
			}, 2000);
		}
	}

	onDestroy(reset);

	$: reset(stateMap[state]);
</script>

<Button on:click={() => (open = true)}>Change to light theme</Button>

<Modal
	bind:open
	modalHeading="Change to light theme"
	primaryButtonText="Confirm"
	secondaryButtonText="Cancel"
	on:click:button--secondary={() => (open = false)}
	on:click:button--primary={() => {
		theme = "white";
		open = false;
	}}
	on:open
	on:close
	on:submit
>
	<p>Change to light theme.</p>
</Modal>

<ButtonSet>
	<Button kind="ghost" disabled={state === "dormant" || state === "finished"} on:click={() => (state = "inactive")}>
		Cancel
	</Button>
	{#if state !== "dormant"}
		<InlineLoading status={state} description={descriptionMap[state]} />
	{:else}
		<Button on:click={() => (state = "active")}>Submit</Button>
	{/if}
</ButtonSet>

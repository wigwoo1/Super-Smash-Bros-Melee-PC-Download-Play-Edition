# Super Smash Bros. Melee PC / Download Play Edition

An unfinished native Windows PC port research project based on [doldecomp/melee](https://github.com/doldecomp/melee) and [Aurora](https://github.com/AxioDL/aurora). This is native game code with a replacement host runtime. It is not a finished playable release.

**Current estimate: approximately 30% of the planned project.** This is a rough engineering estimate, not measured feature coverage. The separate browser/Download Play build and production multiplayer are planned, not available.

## Current diagnostic checkpoint

- November 6 private checks verified a keyboard-triggered P2 jab and original opponent damage increasing from 0% to 2% on muted D3D12 and Vulkan, after automated positioning and native-API binding setup. The helper released P2 test input before the OS keyboard F press; P1 remained neutral. This is scoped keyboard-to-collision acceptance, not unassisted full-flow or release acceptance. See [P2 keyboard/contact verification and limits](verification/P2_KEYBOARD_CONTACT_2026-11-06.md). No new public binary is included.
- November 4 source-matched private WIP checks verified native Versus stage/fighter/HUD rendering, ordinary keyboard movement/jump/jab response, and a normal keyboard jab raising opponent damage from 0% to 3% on both D3D12 and Vulkan. Native Controls remapping from J to F worked in a D3D12 match and reset to J on restart, with no settings files written. A separate staged-controller run reached original timed Results and returned to character selection. These are scoped checks, not full roster, physical-controller, performance or release acceptance. See [November 4 verification and limits](verification/NATIVE_VERSUS_2026-11-04.md). No new public binary was released.
- November 3 Adventure diagnostics verified three original Link defeats and maze returns on the same private candidate on muted D3D12 and Vulkan, including the west gap, shelf, descent and stairs. Earlier Vulkan runs stopped before third combat; the later accepted repeat exercised the actual fight and original return. Full maze and Adventure acceptance remain open. See the [scoped November 3 verification](verification/ADVENTURE_MAZE_2026-11-03.md).
- Current source adds opt-in controller routes and a guard that refuses scripted character selection outside the actual Adventure campaign. These tests use ordinary controller inputs and preserve original stage, physics, stocks, damage and randomized exits. They do not establish physical-controller or full-release acceptance.
- New optional lower-chute routing compiles, but its two muted D3D12 attempts stopped earlier in the campaign. The descent and follow-up arrival-window correction remain unexercised; no new build is delivered for that milestone. See the [lower-chute limits and test results](verification/LOWER_CHUTE_2026-11-03.md).
- The cached-source rebuild now respects the driver's explicit Dawn dependency-fetch OFF setting. Focused configuration regressions, a fresh cached-source runtime configuration and a small fresh x86 library build passed. Full cold-download acceptance remains open. See the [build-system verification](verification/DAWN_FETCH_OPTION_2026-11-03.md).
- The full cached-source runtime build passed all 1,402 steps. Its x86 artifacts passed inspection and linked with zero unresolved symbols. The November 4 source-matched private checks above now exercise the rebuilt runtime in Versus; the earlier [full cached-runtime build record](verification/FULL_CACHED_RUNTIME_2026-11-03.md) describes the build-only checkpoint. Cold dependency downloads remain unverified.
- Reviewed October 28 source includes the native Data/Special metadata repair, scoped Archives playback checks, and original Records-page checks. The full Special Movie finished naturally on muted D3D12 at its original schedule; How to Play finished on D3D12 and Vulkan. Full Special Movie completion on Vulkan remains unverified.
- Session-only statistics passed an original short Time match, Sudden Death, Results and return to Misc. Records on muted D3D12 and Vulkan: the actual screen displayed VS./Time match totals of 1, without save storage or injected scores. Other statistics and populated bonus rows remain unverified.
- Ordinary Windows Enter/S/J/K menu input passed a D3D12 control with the title timeout held. Ordinary unheld startup and physical keyboard/controller acceptance remain open; this exception is not claimed as ordinary startup verification.
- Newer source includes focused Kirby Ice Shot collision tracing: a muted 1,500-frame native test followed one projectile through a Popo hit and verified damage increasing from 8% to 13%. Scripted pad input was used; no damage was injected. This does not establish all-match or physical-controller acceptance.
- Kirby's Game & Watch hat visibility now uses the hat's own model table. Focused renderer checks passed; broader stability remains unaccepted.
- Cached uniform diagnostic flags passed over 540,000 live byte-equivalence comparisons in one workload. Timing comparisons favored the candidate but varied substantially; no stable percentage speedup is claimed.
- Bounded muted D3D12, Vulkan and D3D11 checks passed on the test machine. The latest D3D11 timing check kept simulation near 60 Hz; older hardware is not verified.
- Full roster/stage fidelity, physical controls, performance, multiplayer and release readiness remain unfinished. Overall estimate remains approximately 30%.

These results concern newer private diagnostic candidates and current source. The October 18 prerelease remains pinned to its original source and does not contain these newer changes. See [progress notes](docs/PORT_PROGRESS.txt) for scoped evidence; older verification files describe historical candidates.

## Downloads and private setup

Development releases contain an **asset-free executable template**, matching source, dependency notices, and hashes. The template cannot run directly. Original game fonts were removed from it, and original disc images, textures, audio, movies, caches, and saves are not included.

1. Download and extract the development runtime template from this repository's Releases page.
2. Supply your own supported USA revision 2 disc image. The setup checks the disc executable's SHA-1 before reading the two font regions.
3. With Python installed, run from the extracted runtime folder:

   ```powershell
   py prepare-local.py --disc "D:\path\to\your-disc.nkit.iso" --output "private\melee.exe"
   ```

4. Build the dependencies using [RUNTIME_REBUILD.txt](docs/RUNTIME_REBUILD.txt), then copy its x86 `SDL3.dll`, `webgpu_dawn.dll`, and `nod.dll` beside that private executable. Third-party DLLs are not bundled in this public development archive. Install the Microsoft Visual C++ x86 runtime if missing; Microsoft runtime DLLs are not bundled in the public package.
5. Place your own disc copy at `private\data\melee.nkit.iso`. Run the private executable with audio muted:

   ```powershell
   $env:AURORA_DIAGNOSTIC_MUTE_AUDIO="1"
   .\private\melee.exe
   ```

The locally restored executable contains fonts from your disc. Keep it and your assets private. Setup never overwrites an existing output, modifies the disc, or extracts loose original assets. Optional HD packs and ACE content are not bundled; their integration and distribution require their own authorization.

## Source, rebuilds, and mods

- `source/`: current Melee source, including all native PC conversion and gameplay changes.
- `dependencies/aurora/`: current host/renderer source and dependency version declarations.
- [PUBLIC_REBUILD.md](docs/PUBLIC_REBUILD.md): rebuild commands for this repository's `source/` and `dependencies/aurora/` layout, private font generation and local testing. [SOURCE_REBUILD.txt](docs/SOURCE_REBUILD.txt) preserves the historical Desktop build's verification scope.
- [MODDING.txt](docs/MODDING.txt): current mod installation and rebuild guidance. Easy custom character/stage installation remains planned.
- `publication/source-provenance.json`: original upstream commits and exact source file hashes.
- `publication/build-template.json`: template hashes, removed font regions, and source-path normalization record.

Each release pins its corresponding source commit; repository main may later be ahead. This first template is derived from the candidate compiled from the initial source snapshot. Only the original font regions and fixed-length diagnostic source-path strings are changed. Preparing it locally restores the fonts and verifies the expected final hash. A complete cold dependency rebuild on another machine remains unverified.

## Roadmap

Native Melee-styled settings include the planned aspect ratios, protected menu/FMV framing, resolution and texture quality, shadow and lighting controls, LOD, renderer selection, audio settings, and higher presentation rates without changing 60 Hz gameplay. Standard content is unlocked by default; preferences/statistics are session-only, with no memory-card user flow.

Later milestones include native mods, controller/keyboard customization, four-player P2P and LAN lobbies, and a separate browser launcher. Download Play is intended to transfer assets the host is authorized to distribute, show each client's progress, and allow control/graphics configuration during download. Temporary storage does not grant redistribution rights.

## Notices and updates

Preserve all upstream notices. [Third-party notices](docs/THIRD_PARTY_NOTICES.txt) describe the currently known source/dependency notices; there is no new blanket license assigned to the Melee source. Aurora retains its MIT license. The release's `notices/` directory preserves available dependency license texts; third-party DLLs are obtained separately through the runtime rebuild instructions.

A daily local Codex publishing check is scheduled. It publishes reviewed changes when available; it does not generate empty daily commits, and requires the local environment and GitHub authorization to remain available.

> Main has newer source than the pinned development binary. Read each release at its own source commit; the old binary is not claimed to include these newer changes.

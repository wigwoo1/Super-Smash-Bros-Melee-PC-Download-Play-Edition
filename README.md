# Super Smash Bros. Melee PC / Download Play Edition

An unfinished native Windows PC port research project based on [doldecomp/melee](https://github.com/doldecomp/melee) and [Aurora](https://github.com/AxioDL/aurora). This is native game code with a replacement host runtime. It is not a finished playable release.

**Current estimate: approximately 28% of the planned project.** This is a rough engineering estimate, not measured feature coverage. The separate browser/Download Play build and production multiplayer are planned, not available.

## Current diagnostic checkpoint

- Native menus and bounded matches have run with D3D12 and Vulkan in earlier checks; D3D11 remains less thoroughly verified.
- Latest candidate: Kirby can inhale Ice Climbers, acquire their ability, and create Ice Shot. Muted 1,500-frame native tests exited cleanly.
- Ice Shot launched at its loaded speed of 1.5 units/frame and traveled forward. A 5-damage callback occurred, but the actual target was not identified and Popo's damage did not increase. **Opponent-hit verification is incomplete.**
- An intermittent visibility-related memory crash is unresolved. Full roster/stage fidelity, physical controllers, performance, online play, and release readiness are not accepted.

See [verification](verification/), [the implementation plan](docs/IMPLEMENTATION_PLAN.txt), and [progress notes](docs/PORT_PROGRESS.txt). Historical notes describe experiments; they are not a feature checklist of completed work.

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
- [SOURCE_REBUILD.txt](docs/SOURCE_REBUILD.txt): native rebuild steps and known limits. For this repository, the documents are in `docs/`; run the Python build drivers from the repository root.
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

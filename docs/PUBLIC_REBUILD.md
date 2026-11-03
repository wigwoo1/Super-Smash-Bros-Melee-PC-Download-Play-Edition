# Rebuilding the public repository on Windows

This guide uses the repository layout, with `source/`, `dependencies/aurora/` and the Python build drivers at its root. The Desktop package instructions in `SOURCE_REBUILD.txt` describe a different installation layout. Neither a successful library build nor an executable link establishes a finished playable port.

## Prerequisites

Use an **x86 Visual Studio Developer Command Prompt** with the Windows SDK, Clang/Clang++, CMake 3.25 or newer, Ninja, Python 3 and Rust available. Rust needs the `i686-pc-windows-msvc` target. Run the commands below in that command prompt, from the repository root. They use Command Prompt syntax, not PowerShell syntax.

Check the dependency declarations in `dependencies/aurora/cmake/AuroraDependencyVersions.cmake`, the renderer CMake files and [third-party notices](THIRD_PARTY_NOTICES.txt). Keep all notices when modifying or redistributing code or dependencies. No new blanket license is assigned to the Melee source.

Choose a **new, short build directory outside the repository**. Keep original game assets, generated fonts, private executable outputs and local logs out of the public source tree. Do not overwrite a working installation with an untested build.

```bat
set "MELEE_BUILD_ROOT=%USERPROFILE%\Documents\melee-build"
set "MELEE_LOCAL_DISC=D:\path\to\your-disc.nkit.iso"
```

The disc must be your own supported USA revision 2 image. The font extractor checks the original disc executable's SHA-1 before reading font data. It reads the disc without modifying it; generated font includes belong only in your private build directory.

## Build and link

Generate the private font inputs:

```bat
python source/tools/extract_pc_fonts.py "%MELEE_LOCAL_DISC%" "%MELEE_BUILD_ROOT%\fonts"
```

Build the renderer/runtime:

```bat
python rebuild-native-runtime.py --build-dir "%MELEE_BUILD_ROOT%\runtime"
```

The runtime driver selects `dependencies/aurora/` automatically. Keep `execute-target-for-cmake.py` beside it. Without a dependency source map, it needs upstream downloads; the complete cold-download path remains unverified. For an existing, complete set of dependency sources, see [RUNTIME_REBUILD.txt](RUNTIME_REBUILD.txt) for `--dependency-source-map` and tool overrides. That mode reuses source folders and must not be described as a cold download.

Current source fixes the Dawn provider overwriting the source-map driver's explicit `DAWN_FETCH_DEPENDENCIES=OFF`. A fresh cached-source configuration now retains OFF; the normal default without that override remains ON. See the [configuration repair and scoped verification](../verification/DAWN_FETCH_OPTION_2026-11-03.md). A complete cached-source set is still required, and the full cold build remains unverified.

Build the game library in **Release**, matching the runtime configuration:

```bat
python rebuild-native-library.py --build-type Release --generated-includes "%MELEE_BUILD_ROOT%\fonts" --build-dir "%MELEE_BUILD_ROOT%\game"
```

The library driver selects `source/` and `dependencies/aurora/` automatically. It enables session-only no-save operation and unlocks standard content by default. The library alone cannot launch the game.

Link into a separate private output directory:

```bat
python link-game-audit.py --runtime-build "%MELEE_BUILD_ROOT%\runtime" --game-library "%MELEE_BUILD_ROOT%\game\melee.lib" --output "%MELEE_BUILD_ROOT%\output\melee.exe" --no-saves
```

Inspect the generated build records and link report. A zero-unresolved-symbol link is a compilation result, not gameplay verification. The no-save link must exclude the memory-card backend.

## Assemble a private test installation

Copy the matching x86 runtime DLLs beside the new executable: `SDL3.dll` from `runtime/examples`, `webgpu_dawn.dll` from `runtime/_deps/dawn-build` and `nod.dll` from `runtime/_deps/aurora_nod-build`. Install the required Microsoft Visual C++ x86 runtime separately. Do not mix DLLs from unrelated configurations.

Put your own disc copy at `output/data/melee.nkit.iso`, relative to the build directory above. Keep your original disc unchanged. Launch your private executable muted:

```bat
set "AURORA_DIAGNOSTIC_MUTE_AUDIO=1"
"%MELEE_BUILD_ROOT%\output\melee.exe"
```

The generated executable contains fonts from your disc. Keep it and those assets private. This source repository does not supply original game assets or grant permission to redistribute them. Follow [MODDING.txt](MODDING.txt) for current modification limits; easy character/stage package installation and browser Download Play remain roadmap work.

## Verification limits

On November 3, 2026, the game source from public commit `54944c0163512a3aa3ead443b07e9cb1b5fd3c01` was compiled into a fresh private output directory using this repository layout. All 1,011 build steps passed; all 1,010 archive object members were x86. Linking that library against the existing configured Release runtime passed with zero unresolved symbols, the memory-card library excluded and an x86 large-address-aware executable produced. Local font extraction passed and the original disc's size and modification time were unchanged. That new executable was **not launched**. This verifies the public game-source build and link on the development PC, not a fresh runtime/dependency build or gameplay acceptance. See the [sanitized verification record](../verification/PUBLIC_GAME_REBUILD_2026-11-03.json).

Historical cached-source runtime builds and bounded muted native tests are described in [SOURCE_REBUILD.txt](SOURCE_REBUILD.txt). They were performed on the development PC. They do not prove a cold dependency download, another computer's compatibility, full roster/stage correctness or release readiness. Read each public release at its pinned source commit: current `main` can contain newer source than an older downloadable binary.

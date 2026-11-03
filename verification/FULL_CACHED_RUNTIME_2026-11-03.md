# Full cached-source runtime build

Status: verified for compilation, artifact architecture and executable linking. Overall project estimate remains approximately 30%; this is not playable-release acceptance.

On November 3, 2026, the fresh x86 Release runtime configuration with Dawn dependency fetching disabled completed its full default build: all 1,402 scheduled steps passed, exit code 0, in 732.8 seconds. This follows the earlier six-object matrix-library probe. Existing dependency sources, compiler/SDK tools and Rust registry inputs were reused; Cargo was set to offline mode. Compiler warnings remain in the private build record.

The artifact audit checked all members of 12 Aurora archives as x86 COFF. Dawn, nod and SDL3 DLLs and the standalone sample executable were checked as x86 PE files. No memory-card archive was produced. The new runtime then linked successfully with the existing Release game library, with zero unresolved symbols and the card backend excluded.

No game or sample executable was launched. The existing Desktop delivery and installed game were not replaced, and no binary was publicly released. Original discs, saves and HD art were preserved.

This does not prove cold dependency downloads, a fresh compilation of the game library in this step, gameplay with these newly built DLLs, another-machine compatibility or performance. The earlier lower-chute gameplay retry gate remains separate. Next: stage the newly linked executable with these exact DLLs and matching source, then obtain appropriate muted integration evidence before promoting a build. Audit nested Dawn and Rust inputs before claiming an empty-cache rebuild.

See [the artifact record](FULL_CACHED_RUNTIME_2026-11-03.json) and the preceding [Dawn fetch-option repair](DAWN_FETCH_OPTION_2026-11-03.md).

## Static integration staging follow-up

The newly linked executable was staged privately with its exact runtime DLLs and matching source at source commit `1dcb13ebcef9b013e914f28eb0cb9eb779c7ee8c`. All seven PE files are x86; normal imports resolve to bundled files or this PC's Windows x86 system libraries. API-set imports are delegated to Windows; delay loads and dynamic library loading remain unverified. The source ZIP contains 3,777 hash-verified entries, including 34 compiled game inputs and all 3,665 current native source files. Rebuild/mod instructions and dependency notices are included. No executable was launched, no Desktop build was replaced, and no binary was published. See the [static staging record](CACHED_RUNTIME_STAGING_2026-11-03.json). This is preparation for muted integration testing, not runtime acceptance.

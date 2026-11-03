# Cached-source Dawn configuration repair

Status: verified for the configuration behavior and small library build below. The entire port remains unfinished, with an approximate engineering estimate of 30%.

## Problem and change

The runtime rebuild driver passes `DAWN_FETCH_DEPENDENCIES=OFF` when a dependency source map is supplied. Aurora's Dawn provider used an INTERNAL cache assignment that overwrote this with ON. The existing configured development tree contained ON, and a regression fixture reproduced the mismatch against the actual provider.

The provider now uses a normal BOOL cache default. An explicit OFF is preserved; the normal default remains ON. The change affects dependency setup, not rendering or game algorithms. The complete cached-source directory set is still required.

## Checks actually completed

- Before the change, the real-provider configuration fixture passed default ON but failed explicit OFF with observed ON. FetchContent population was intercepted, so this test did not download dependencies.
- After the change, seven configuration cases passed twice: default ON, explicit OFF, explicit ON, OFF/ON/OFF changes in a reused cache, and legacy INTERNAL OFF. The test is included in Aurora's CTest registration as `dawn_dependency_fetch_option`.
- A separate fresh Windows x86 Release runtime configuration passed with 15 source-map keys referencing 14 existing dependency folders and Cargo set to offline mode. Its generated cache retained `DAWN_FETCH_DEPENDENCIES=OFF`, `FETCHCONTENT_FULLY_DISCONNECTED=ON` and `AURORA_ENABLE_CARD=OFF`.
- The new configured tree then compiled the `aurora_mtx` target from fresh objects. All seven build steps passed and its six archive object members were verified as x86 COFF.

No game or audio device was launched. Existing executables, DLLs, Desktop deliveries, discs and saves were preserved. There is no new public binary.

## Reproduce the focused configuration test

From the repository root, with CMake and Ninja available, choose a separate test build directory:

```bat
cmake -DTEST_ROOT="D:/path/to/new-test-build" -DTEST_GENERATOR=Ninja -P dependencies/aurora/tests/cmake/dawn_fetch_option/run.cmake
```

Supply `-DTEST_MAKE_PROGRAM="D:/path/to/ninja.exe"` if Ninja is not on PATH. This fixture includes the actual provider but substitutes dependency population with imported test targets. It does not compile Dawn or prove that the cached sources are complete. It writes logs and configuration outputs only under the chosen build directory.

## Remaining scope

The real configuration reused existing dependency sources, SDK/compiler tools and Rust registry inputs. It was **not a cold dependency download**, a complete new Dawn/runtime build, another-machine rebuild or gameplay test. The previous lower-chute gameplay retries remain stopped under the user's two-attempt rule. Next build milestone: compile the complete cached-source runtime from this new tree; audit nested Dawn and Rust inputs before claiming an empty-cache rebuild.

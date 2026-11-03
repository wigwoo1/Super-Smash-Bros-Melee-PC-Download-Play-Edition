# Adventure maze: scoped November 3 source checkpoint

The project remains unfinished, with a rough engineering estimate of approximately 30%. This update publishes source and documentation only. It does not publish a new binary or change the immutable October 18 prerelease.

## Verified private candidates

| Private executable SHA-256 | Renderer | Accepted scope |
| --- | --- | --- |
| `734df1e31b2cf9edf9b98dc905caf62977a3264efa1afc83782265a2fd452e74` | D3D12 and Vulkan, muted | Two original Link defeats and maze returns, west gap, upper shelf, lower-platform descent, small west lip, descending stairs and original third encounter at sword 194. |
| `863ffb8e2675ca29013beae1025582c169300673607367199015b073025b492b` | D3D12, muted | Three original Link defeats and returns. Third combat sampled opponent damage up to 49; the original encounter progressed through states 1/2/3/4/5/0 and returned with cleared mask 35, three player stocks and 54% player damage. The restored maze scene was visually observed. |

The second candidate's Vulkan checks are **unexercised for third combat**, not passes or demonstrated combat failures. One ended at the earlier upper-passage route deadline. The other reached sword 194, which was the original randomized exit in that run, and displayed Stage Clear without a third fight. Both processes exited cleanly with audio muted and without session storage.

## What the source changes do

- `source/src/pc/adventure_koopa_diagnostic.h` adds optional normal-controller routes across the west platforms and stairs. Third-fight input reuses the existing Link combat policy with live marker-centered inward recovery, a bounded deadline and neutral input after the original maze return.
- `source/src/melee/mn/mncharsel.c` refuses scripted character selection when the opt-in test expects Adventure but the actual campaign differs. It leaves the original character-selection callback running.
- `source/src/melee/mn/mnmain.c` adds optional Regular Match selection/input logging.

These diagnostics do not assign stage geometry, fighter physics, positions, stocks, damage, RNG, exit choice or encounter outcomes. Original proximity checks, combat and stage callbacks decide the results. The third-fight candidate's other 32 tracked build inputs and original stage source remained unchanged during this adaptation. Manual private launchers leave scripted route/combat inputs disabled.

Native incremental builds and no-card links passed with zero undefined symbols; compiler warnings remain. Source, executable and frozen runtime DLL hashes were checked before each accepted run. The separately delivered private candidates include matching source, dependency notices and rebuild/mod instructions. They contain original font data and are not public downloads.

## Remaining limits

- Exact third-fight candidate Vulkan defeat/return verification remains pending.
- Full maze navigation, Adventure completion, physical controls, cold rebuilds and the complete port remain unaccepted.
- No renderer performance comparison, shortcut, production multiplayer, browser build or original-asset redistribution permission is established by these tests.
- Older verification records and the October 18 binary describe their own source/build snapshots. The current source is newer; it must not be represented as code included in that old binary.

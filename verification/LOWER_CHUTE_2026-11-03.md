# Lower western maze chute: compiled, runtime unverified

Overall project estimate remains approximately 30%. This is a source-only update. The previously verified three-Link checkpoint and immutable October 18 prerelease retain their own exact source/build scope.

## Source change

`source/src/pc/maze_lower_chute_diagnostic.h` adds an optional normal-controller descent after the original third Link return. It walks west across the connected upper floors, steers down the authored western chute and emits down-stick edges on one-way platforms. Capture escape and get-up use ordinary buttons. The controller stops on stock loss, a 1,200-tick deadline, bounds violation or the target floor 46 landing. It does not implement the fourth fight.

The existing controller calls it only when `MELEE_DIAGNOSTIC_ADVENTURE_MAZE_LOWER_CHUTE` is enabled and the original encounter has returned to its maze state. Without that flag, the previously accepted third-return stop remains. No stage geometry, physics, position, damage, stocks, RNG, exit choice or encounter outcomes are assigned by the new diagnostic.

A follow-up change gives this opt-in test a bounded arrival window on the earlier west approach: after tick 500, it may continue only while grounded on floor 128 or 30, within x=330..370, and never beyond tick 620. Outside that near-landing region the original 500-tick limit remains. Stock and height guards remain in place. This correction has compiled but was not exercised by the second run.

## Actual checks

| Candidate SHA-256 | Result |
| --- | --- |
| `e069b2daabc07e6808f64d0b6c84697d4a428f9a5e8a1e652943befba9b40084` | Incremental native build and no-card link passed, zero unresolved symbols. Muted D3D12 verified two original Link defeats/returns, then the earlier west approach deadline stopped the controller at frame 4671, x=347.049, floor 128, two stocks and 58% damage. The new chute was unexercised. Clean exit 0, no timeout or session storage, 911.7 seconds. |
| `86da401e1b2b7e5c8e054a21c17bcd8b8cd8405d9ffa3dd2e63ff548cff1ff00` | Follow-up arrival-window build and no-card link passed, zero unresolved symbols. The muted D3D12 run lost its last stock in the earlier Yoshi fight while the test controller chased an opponent beyond the platform edge. The original Continue screen was inspected. Neither the arrival-window correction nor the new chute was exercised. Clean exit 0, no timeout or session storage, 484.5 seconds. |

Both candidates passed D3D12/Vulkan launch-plan hash guards. Those plan checks did not launch Vulkan gameplay. All 34 current tracked build inputs were checked, the six runtime DLLs were unchanged, and the original maze stage source remained unchanged. Compiler warnings remain (244 for the first candidate, 245 for the follow-up).

There is **no accepted lower-chute landing**, no new Desktop delivery and no new public binary. The two focused attempts are stopped pending continuation. Next investigation: keep the normal-input Yoshi test controller from chasing beyond the actual stage edge, then exercise the lower route without forcing campaign progress or randomized exits. Full maze, Adventure, physical-controller and playable-release acceptance remain unfinished.

# November 4 native Versus and Controls verification

Overall project estimate: approximately 30%. The complete port is unfinished.

These results concern a source-matched **private WIP**, not the immutable
October 18 public development template. No new executable is published here.
The private executable contains original fonts and is excluded from the public
source repository. The source archive excludes original assets and fonts.

Private tested executable SHA-256:
`60ea2bcc1fbc1bef42a99ea26d6fd690b3d05940dfd9079daa10b14d0c9a2b72`

Matching private source archive SHA-256:
`31cf60dc7a6d32a6397602a15a084236902641c6667b3141443eb895bffb98eb`

## Verified scope

All runtime checks were muted on the test machine.

| Check | Evidence and limits |
| --- | --- |
| Reviewed source build | Full 1,011-step Release x86 game build, followed by a two-step settings-only rebuild from the updated exact source snapshot; native link had zero unresolved symbols. Cached Aurora reuse was checked against source and artifact hashes. Cold downloads were not tested. |
| Settings layout | Six visible rows per page, larger native SIS text, separated columns, complete captions and page labels; both Graphics and Controls pages visually checked on D3D12. |
| Native Versus | Yoshi's Story, Kirby, Mr. Game & Watch and HUD rendered; ordinary keyboard movement, jump and original jab action were checked on D3D12 and Vulkan. Setup automation was limited to menus/character/stage selection. Diagnostic key bridging and synthetic gameplay PAD were absent. |
| Ordinary keyboard damage | On Final Destination, normal D approach then J jab changed the opponent from 0% to 3% on each renderer. Original attack tracing recorded idle motion 14 becoming jab 44. No damage was injected. |
| Native keyboard remapping | With diagnostic Controls input disabled, the native menu captured A from J to F. F confirmed subsequent menus and triggered Kirby's original jab in D3D12 Versus; J produced no attack event. Remapped-key contact damage and Vulkan remap UI were not tested. |
| Session lifecycle | A fresh D3D12 launch using the previous session path showed A=J again. J confirmed menus. Runtime reported session-only preferences with no user path; no session files were written. |
| Timed results | A separate D3D12 run used existing staged walking/falling inputs and the normal two-minute timer. Original Time Battle results appeared; staged Start exited results and returned to character selection. Retained setup automation then began another match. This was not a normal-keyboard full-match test. |
| Shutdown/integrity | Runs closed cleanly; source archive, executable and seven runtime-file hashes passed the private package verifier. Disc and existing saves were preserved. |

Stage-label correction: stage 8 / `GrSt.dat` is Yoshi's Story. Some historical
reports called it Dream Land; those names were incorrect.

## Remaining work

Normal Player 2 keyboard joining remains unverified after two earlier attempts.
The existing configurable-port path requires observation through PAD sampling,
the ordered input queue and the character-select consumer before a behavioral
fix is justified. Automated two-player setup is separate evidence.

Physical Xbox/controller input, complete standard roster/stage fidelity, long
sessions, stable high-FPS performance, full audio/device acceptance, native
custom character/stage registration, production P2P/LAN and the separate genuine
browser runtime/authorized asset transfer remain incomplete or unverified.
The current source changes do not update the October 18 binary or its claims.

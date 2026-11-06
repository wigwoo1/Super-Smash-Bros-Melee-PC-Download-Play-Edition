# Scoped P2 keyboard/contact verification — November 6, 2026

The original keyboard-to-gameplay path produced a Player 2 jab, an accepted
Player 2-to-Player 1 fighter collision, and a subsequent damage increase from
0% to 2% on muted D3D12 and Vulkan. Both captures were reviewed privately;
original game pixels and private executables are not published here.

The test first used original controller inputs to select Pichu/Pikachu and
Final Destination, then walked P2 into range. It configured the session keyboard
preset and F binding through the same native APIs used by Controls. At frame 478
it cleared P2 virtual status and stopped staging P2 input. Computer Use then
pressed F through the Windows game window. P1 remained staged neutral so shared
keyboard mappings could not cause Pichu self-damage.

| Renderer | P2 jab accepted | Original collision | P1 damage 0 → 2 |
| --- | ---: | ---: | ---: |
| D3D12 | 4077 | 4078 | 4079 |
| Vulkan | 4577 | 4578 | 4579 |

Each trace recorded a focused, accepted F key event, its mapped PAD edge,
Pikachu's original idle 14 → jab 44 transition, the original collision with
Pichu, and the later original damage update. No scripted P2 A press occurred
after handoff. Both runs exited cleanly and the disc hash remained unchanged.

An earlier controller-only version produced the same 2% hit on both renderers.
Separately, the fully rebuilt private candidate passed scripted D3D12 match
rendering/movement/attack samples and normal Controls/P2 joining on Vulkan.
These are different executable checkpoints; they are not one unassisted run.

## Source and reproduction scope

The opt-in helper is `source/src/pc/p2_contact_diagnostic.h`, called by the
existing `MELEE_DIAGNOSTIC_MATCH_INPUT` harness. `MELEE_DIAGNOSTIC_P2_CONTACT`
enables the approach/jab probe; `MELEE_DIAGNOSTIC_P2_CONTACT_KEYBOARD` instead
hands P2 back to normal keyboard input when grounded and facing the opponent.
It refuses stages other than standard Final Destination and requires human
ports 0 and 1. Fighter positions, damage, hitboxes, stocks and outcomes are
read-only. Default gameplay is unaffected when the diagnostic flags are absent.

Use the normal source rebuild instructions and an authorized local disc.
The fixture uses `MELEE_DIAGNOSTIC_CSS_TARGET0=24` (Pichu),
`MELEE_DIAGNOSTIC_CSS_TARGET1=13` (Pikachu), and stage row `25` (Final Destination)
with the existing title/menu/CSS/stage diagnostic route. Enable
`MELEE_DIAGNOSTIC_PLAYER2_DAMAGE`, `MELEE_DIAGNOSTIC_ATTACK_STATE`,
`MELEE_DIAGNOSTIC_KEYBOARD_PAD` and muted audio to correlate the input and hit.
After `NATIVE_P2_CONTACT_KEYBOARD_READY`, focus the game window and press F.
This describes the scoped probe, not an end-user gameplay requirement.

The [structured verification](P2_KEYBOARD_CONTACT_2026-11-06.json) records source
hashes and the private diagnostic executable hash. No new public binary or
release tag accompanies this source update. Existing releases remain immutable
and keep their original limitations, including their historical opponent-hit
status. The full native port, independent movement-profile acceptance, physical
controller tests, long-session fidelity, networking and browser runtime remain
unfinished. The overall project estimate remains approximately 30%.

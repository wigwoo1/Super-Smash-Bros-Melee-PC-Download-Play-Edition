# November 6 renderer and session-option verification

Current reviewed source adds failed GPU-startup cleanup, owned request callback
state, normal-start renderer fallback and bounded same-build renderer replacement
session transfer. It also distinguishes D-pad, main stick, C-stick and analog
trigger destinations in Controls.

The private candidate identified in the [structured record](RENDERER_SESSION_2026-11-06.json)
compiled and linked as native x86 Release with no unresolved symbols or card
backend. Its focused options tests passed1,613 checks and the process protocol
passed11 checks. Muted native Graphics-menu D3D12-to-Vulkan and reverse handoffs
retained original GamePrefs after the final boot default reset. These options
include exact random-stage and item masks, frequency, rumble, sound balance,
deflicker and saved language. A test-only fixture supplied the nondefault parent
values. The Vulkan Sound menu visibly showed balance -37; replacement menus
responded to ordinary keyboard input. Both parents and children exited cleanly.
No preference/save file is used. A fresh process launch starts a fresh session.

A normal-start scripted original-menu/PAD Versus regression ran600match frames,
rendered Kirby, Mr.Game&Watch and Yoshi Story with both HUDs, consumed movement
and recorded attack inputs, then exited cleanly. This does not prove a full
unassisted match or all content. The later ordinary-menu-only options/handoff
repeat was interrupted and is not accepted. Development is user-paused;
publication upkeep does not resume gameplay testing.

An earlier, separately pinned fallback candidate passed six real host/API fault
cases, game fallback in both directions after controlled device failures, and
rejection of unmet explicit switch children before disc mounting/readiness. Those
checks concern this test machine, not old hardware or all failure conditions.

GameRules/match settings, statistics and physical-controller mappings/assignments
are not yet transferred. Physical Down-arrow, full roster/stages, long sessions,
performance/high refresh, FSR3, mods, P2P/LAN and browser/authorized asset transfer
remain unfinished. Overall project estimate remains approximately30%, not measured
feature coverage or a finished playable release.

No new binary, original assets/fonts, private logs or paths accompany this source
publication. The October18 public template stays pinned to its original source.

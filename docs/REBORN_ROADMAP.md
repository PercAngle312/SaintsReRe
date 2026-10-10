# Saints Reborn — Reborn Roadmap

This roadmap tracks a quality-first plan for improving Saints Reborn while keeping the original Saints Row experience intact. Enhancements should be optional when they change presentation or gameplay.

## Engineering principles

- Preserve original gameplay and save compatibility by default.
- Prefer small, reversible changes over broad renderer rewrites.
- Do not distribute game assets, executable dumps, generated game code, or memory dumps.
- Keep diagnostics opt-in and avoid logging personal data or game content.
- A change is not considered fixed until it builds and has a reproducible test result.

## Phase 1 — Graphics stability

### Targets

- Investigate intermittent world-texture corruption and pop-in.
- Investigate black minimap/render-target reports.
- Reproduce the 21:9 black-screen report and distinguish presentation/aspect handling from scene-rendering failures.
- Check whether existing GPU troubleshooting switches isolate any of these failures before changing defaults.

### Current evidence

- Upstream [issue #28](https://github.com/whompay/SaintsReborn/issues/28) reports intermittent world-texture corruption/pop-in. A commenter reports improvement with three opt-out files; the workaround is captured, clearly labeled unverified, in [Graphics troubleshooting](GRAPHICS_TROUBLESHOOTING.md).
- Upstream [issue #22](https://github.com/whompay/SaintsReborn/issues/22) reports an intermittently black minimap. No verified workaround is documented yet.

### Baseline capture

For every reproduction, record:

- GPU model and driver version
- Display resolution, aspect ratio, window/fullscreen mode
- Internal render scale and frame-rate cap
- Location/mission and steps to reproduce
- Whether restarting, changing render scale, or disabling one existing optimization changes the symptom
- Relevant `build/setup.log` and opt-in GPU timing logs, with personal paths redacted

Do not upload game files, generated recompiled source, executables, or memory dumps.

### Regression matrix

| Scenario | Required check |
|---|---|
| 16:9, windowed and fullscreen | Boot, enter gameplay, pause/resume, change areas |
| 21:9 ultrawide | 3D scene and 2D HUD both render; no persistent black scene |
| Internal scale 1x / 2x / 3x | Scene, HUD, minimap, and map remain visible |
| Interiors and shops | No new corruption, freeze, or missing scene geometry |
| Rooftops and street level | Visibility behavior remains intentional |
| Minimap over extended play | No intermittent black texture |
| Exit and relaunch | No regressions to startup or save loading |

## Phase 2 — Native PC usability

- Audit display and input settings for discoverability and persistence.
- Verify keyboard/mouse and controller switching across gameplay, menus, map, shops, and character creator.
- Make accessibility and quality-of-life options configurable rather than silently changing original behavior.

## Phase 3 — Mod ecosystem

- Document stable mod entry points and compatibility expectations.
- Provide clear examples and diagnostic guidance for mod authors.
- Keep experimental gameplay changes opt-in and single-player-only when multiplayer safety is not established.

## Phase 4 — Release quality

A release candidate should have a clean build, a documented test matrix, known limitations, and a rollback path. Never label an issue fixed solely because a code path looks plausible; record the actual reproduction result.

## Immediate next task

Establish a reproducible graphics baseline before modifying rendering behavior. Start with the existing renderer switches and logs, then make the smallest targeted fix supported by evidence.

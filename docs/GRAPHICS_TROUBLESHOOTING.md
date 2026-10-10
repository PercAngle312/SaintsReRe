# Graphics troubleshooting

This guide collects reversible diagnostics for intermittent rendering problems. These are troubleshooting steps, not guarantees that a defect is fixed.

## Before changing anything

1. Exit the game.
2. Open the folder containing `saintsrow.exe` (not the source-code checkout).
3. Note the current display mode, internal render scale, frame-rate cap, GPU, and driver.
4. Change one setting at a time and revisit the same location so results are comparable.

Keep a copy of any existing option files so you can restore the previous behavior.

## Intermittent world-texture corruption or pop-in

The upstream Saints Reborn issue [#28](https://github.com/whompay/SaintsReborn/issues/28) describes intermittent texture corruption in several city locations. A commenter reported that creating these three **empty, extensionless files** beside `saintsrow.exe` stopped the symptom for them:

- `far_clip_original`
- `prepass_full`
- `pvs_original`

**Status: community-reported workaround; not independently reproduced by this project.** These switches alter rendering behavior and may have visual or performance trade-offs. Do not assume all three are necessary or that the result generalizes to every GPU. For a useful diagnosis, test the files individually, restarting the game between changes, and record which exact combination affects the issue. Remove the files to restore the normal defaults.

## Black 3D scene at 21:9 ultrawide

Upstream [issue #24](https://github.com/whompay/SaintsReborn/issues/24) reports that at 21:9, the 3D scene is black while 2D elements such as text and the minimap still render. That distinction is useful: the application is running and drawing some UI, so record the scene and UI separately rather than reporting only “black screen.”

Use this controlled comparison before changing renderer defaults:

1. Set **Display > Aspect Ratio** to 16:9, restart, and confirm whether the 3D scene renders.
2. Set it to 21:9, restart, and reproduce the same scene. Aspect ratio is read at startup.
3. If 21:9 fails, test internal render scale at 1x, then 2x, restarting between tests. Restore the preferred scale afterward.
4. Check whether `frame_size.txt` is present. If it is, record its value; do not combine a low-resolution frame override with the first baseline test.
5. Save the relevant startup log lines beginning with `Aspect ratio:` (frame dimensions and FOV). Redact personal paths before sharing.

Record whether the 3D world, HUD text, minimap, and pause map each render. This comparison helps distinguish aspect/framebuffer setup from a general startup failure; it does not by itself identify the root cause.

## Black minimap

Upstream issue [#22](https://github.com/whompay/SaintsReborn/issues/22) reports that the minimap can become completely black during play. No verified workaround is documented here yet.

If it occurs, record whether the world scene still renders, whether opening/closing the pause map changes it, the internal render scale, and whether the minimap recovers after changing areas or restarting. Avoid changing several rendering switches at once; that makes the cause harder to identify.

## GPU diagnostics

The upstream README documents an opt-in `gpu_timing` file that logs GPU work categories periodically, plus troubleshooting switches that disable individual optimizations. Use those only when investigating a reproducible symptom, and consult the current upstream README before changing them.

Do not share game files, generated recompiled game code, executables, or memory dumps in bug reports. Redact personal paths from logs before posting them.

## Reporting a useful result

Include:

- GPU model and driver version
- Resolution, aspect ratio, window/fullscreen mode, render scale, and frame cap
- Exact location and steps to reproduce
- Whether the 3D scene, HUD text, minimap, and pause map render
- The one option changed and whether it improved, worsened, or did not affect the symptom
- Relevant log lines and whether the symptom survives a restart

A workaround should not be called a fix until the original failure has been reproduced and the result verified.

# Streamer diagnostic visibility — mission brief

> **Written:** 2026-08-24. Stable mission only; active evidence is in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** the policy ships and the permanent CkCore / module documentation is updated.

## Goal

When `-CkStreamerMode` is present or `ck.Debug.StreamerMode 1` is set, every Ck runtime diagnostic surface is absent from a marketing capture. Disabling the CVar restores each surface's own local diagnostic configuration without mutating it.

## Success criteria

1. CkCore owns the launch-option spelling, CVar spelling, and typed runtime diagnostic-visibility policy.
2. Immediate debug draw, retained debug geometry, Slate/UMG diagnostic overlays, and runtime debugger UI consume that policy at their final visibility seam.
3. A policy change hides visuals that existed before the CVar changed and restores local visibility when it is cleared.
4. Focused automation tests cover policy precedence and each retained/UI final seam; the host editor rebuilds successfully.
5. A human can verify that a capture launched with `-CkStreamerMode` contains no Ck diagnostic output.

## Locked decisions

| Decision | Choice | Why |
|---|---|---|
| Policy owner | Public standalone CkCore diagnostic-visibility contract | The policy governs more than debug primitives. |
| Inputs | Keep `-CkStreamerMode` and `ck.Debug.StreamerMode` | Preserve the marketing-facing interface already published. |
| Existing debug API | Transitional forwarding compatibility only | Avoid a flag-day migration while removing draw-helper ownership. |
| Existing local settings | Preserve and restore them | Streamer mode is an overlay policy, not a configuration mutation. |

## Non-goals

- Hide ordinary authored gameplay HUD/UI: those are not diagnostics.
- Rename or remove current developer-specific debug settings: they remain useful outside streamer mode.
- Claim third-party engine/debugger overlays are suppressed: this contract owns Ck runtime diagnostics.

## Reading list

- `Source/CkCore/Public/CkCore/Debug/CkDebugDraw_Utils.{h,cpp}` — existing input predicate and draw wrappers.
- `Source/CkPmg/Public/CkPmg/CkPmg_Processor_DebugShapes.cpp` — retained debug-shape final seam.
- `Source/CkDebugScene/Public/CkDebugScene/CkDebugScene_Target.cpp` — retained scene-target final seam.
- `Source/CkWatermark/Public/CkWatermark/CkWatermark_Panel_Widget.cpp` — watermark viewport root.
- `../CkGameplayDebugger/Source/CkGameplayDebugger/Public/CkGameplayDebugger/Bridge/CkDebugger_Bridge.cpp` — profile HUD viewport root.

## Things ruled out — do not re-investigate

| Ruled out | Why | Evidence |
|---|---|---|
| Screenshot label is ordinary gameplay UI | It is `UNREAL NAV: Pending`, emitted by CkCrowd's navigation diagnostic processor. | `CkCrowdAgent_DrawNavStatus_Processor.cpp` |
| Reusing ensure display policy | It only controls ensure presentation, not all runtime diagnostic surfaces. | `CkCore_Settings.h` |

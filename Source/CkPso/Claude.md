# CkPso

**Purpose:** PSO preparation for loading screens. A GameInstance subsystem holds the CkLoadingScreen
screen until pending PSO work drains - the bundled pipeline cache plus runtime PSO precache requests, the
one number Epic recommends gating on (`FShaderPipelineCache::NumPrecompilesRemaining()`) - reports
progress for a UMG bar, and measures the runtime PSO hitches that got past it during gameplay.

**Depends on:** `CkCore`, `CkLog`, `CkSettings`, `CkLoadingScreen` (+ engine: Engine, RenderCore, RHI,
DeveloperSettings).
**Used by:** any project that uses CkLoadingScreen - the gate is default-on and inert where there is nothing
to wait on.

Subsystem-shaped like CkLoadingScreen, not an ECS quartet: the PSO cache is process-global, not entity state.

---

## Key API

- `UCk_Pso_Subsystem_UE` (`Subsystem/`) - the gate. Implements `ICk_LoadingProcess` and registers itself
  through `UCk_LoadingScreen_Subsystem_UE::Register_LoadingProcessor`; CkLoadingScreen's Lyra-verbatim
  decision function is not touched. Not created on dedicated servers.
  - `Get_DrainProgress()` -> `FCk_Pso_DrainProgress` (state, timeout reason, pending, peak, 0..1 ratio, elapsed).
  - `Get_IsDraining()` - Draining or Settling (i.e. holding, when the gate is enabled).
  - `Get_IsPrecachingSupported()` - `PipelineStateCache::IsPSOPrecachingEnabled()`; always false in the editor.
  - `Get_HitchReport()` -> `FCk_Pso_HitchReport` for the current gameplay window.
  - `Get_DebugReport()` - the text `ck.Pso.DumpReport` prints.
  - `Request_BeginDrainWindow()` - arm a window for work no map load or loading screen announces.
  - `BindTo_OnDrainProgressChanged` / `UnbindFrom_OnDrainProgressChanged` - fires when the snapshot changes.
    Elapsed time alone is not a change; poll `Get_DrainProgress()` for a clock.
- `FCk_Pso_DrainTracker` (`Drain/`) - pure, engine-free state machine fed `(pending, DeltaT)`:
  `Idle -> Draining -> Settling -> Complete | TimedOut`. Unit-tested on its own (`Ck.Pso.DrainTracker.*`).
- `UCk_Pso_ProjectSettings_UE` / `UCk_Utils_Pso_Settings_UE` (`Settings/`) - Project Settings > CkFoundation > PSO.

### When a window arms

When the loading screen becomes Visible, when a map load starts for this game instance
(`PreLoadMapWithContext`), or on `Request_BeginDrainWindow()` - and never over a window that is still
holding. Complete / TimedOut / Idle windows are re-begun. Work that reappears after Complete re-opens the
same window inside the same MaxWait budget, which is what lets CkLoadingScreen's `HoldAdditionalSecs`
window cover late requests instead of being a blind timer.

**A Complete window only re-opens while the loading screen is still showing.** Once the screen has dropped,
the subsystem stops feeding the tracker: PSO work that arrives during gameplay belongs to gameplay, and
re-opening for it would put the loading screen back up mid-game. The engine's own proxy-creation delay
covers those requests.

### Timing

The tracker advances on the tickable's DeltaTime, clamped to 0.1s per tick: a blocking `LoadMap` arrives as
one huge tick and must not burn the MaxWait budget before the new map has requested anything. That delta is
game time, so anything that freezes the game clock (pause, a snapshot load's time-dilation floor) also pauses
the window's quiet period and budgets. A snapshot load holds the screen with its own task anyway.

## Settings and CVars

| Setting (`[/Script/CkPso.Ck_Pso_ProjectSettings_UE]`) | Default | Meaning |
|---|---|---|
| `_WaitForPsoPrecache` | `Enable` | Hold the screen while a window drains |
| `_DrainQuietPeriod` | 0.25s | Count must sit at 0 this long before Complete |
| `_DrainMaxWait` | 60s | Fail-open budget per window (0 disables) |
| `_DrainStallTimeout` | 10s | Fail-open when the count has not CHANGED this long (0 disables). A rising count is work still arriving; only `_DrainMaxWait` bounds that |

`FCk_Time` settings serialize as `_DrainMaxWait=(_Seconds=60.000000)`.

| CVar | Meaning |
|---|---|
| `ck.Pso.WaitForPrecache` | -1 (default) = project setting, 0 = off, 1 = on |
| `ck.Pso.Debug.SimulatedRemaining` | Non-Shipping. >= 0 replaces the engine's pending count and makes the gate active even in the editor; -1 = off |
| `ck.Pso.Debug.StallTimeoutOverride` | Non-Shipping. >= 0 replaces `_DrainStallTimeout`, in seconds, for windows armed afterwards (0 disables the stall check); -1 = off. The fail-open AutoTest uses it so it does not sit through the project budget |
| `ck.Pso.LogDrainEveryFrame` | Log state/pending/progress every frame while holding |

Console command: `ck.Pso.DumpReport` - drain snapshot, gate state, and the gameplay-window hitch report.

## The inert rule (and why)

The gate is **inert** - never holds, tracker kept Idle - when ALL of these hold:

- `ck.Pso.Debug.SimulatedRemaining` < 0,
- `PipelineStateCache::IsPSOPrecachingEnabled()` is false,
- `FShaderPipelineCache::IsPrecompiling()` is false.

PSO precaching is hard-disabled under `WITH_EDITOR` (`PipelineStateCache.cpp:4408`, "Disables in the editor
for now"), and the shader pipeline cache only exists when the shader code library is enabled
(`ShaderPipelineCache.cpp:738`), i.e. cooked builds. So the editor, PIE and toolbox Gauntlet runs on editor
binaries have nothing real to wait on, and the existing loading-screen tests assert exact release timing.
Inert keeps all of them byte-for-byte unchanged. **The real drain is only observable in a packaged
(non-editor) build.** Use the simulated count to iterate on UI in PIE.

## Telemetry

- **Drain transitions** log once each: `Log` on Complete (duration + peak), `Warning` on TimedOut (reason,
  pending, peak). A timeout is an environment outcome (cold driver cache, slow machine), not a programming
  error, so it fails open and warns instead of ensuring. CSV events in category `CkPso`:
  `DrainWindowBegin`, `DrainComplete`, `DrainTimedOut`.
- **Gameplay window** = loading screen Hidden -> next Visible. On Hidden the subsystem baselines
  `PipelineStateCache::GetPSORuntimeCreationStats()` (and the validation counters when available);
  `Get_HitchReport()` is current-minus-baseline; one summary line is logged when the screen next becomes
  Visible and at Deinitialize. Before the first Hidden the baseline is the subsystem's creation.
  The subsystem never calls `ResetPSOHitchTrackingStats()` - other systems may read those counters.
- With presentation suppressed (`-unattended`, null RHI, `ck.LoadingScreen.Disable`) the loading screen never
  broadcasts visibility, so no gameplay window opens and the report covers everything since creation.

### Dev-build recipe: full-PSO hit/miss counters

`FCk_Pso_HitchReport::_Validation` is `Enable` only when validation is compiled in
(`PSO_PRECACHING_VALIDATE`: non-editor, platform supports precaching) **and** enabled at launch.
`r.PSOPrecache.Validation` is `ECVF_ReadOnly` (`PSOPrecacheValidation.cpp:53`): the console cannot change it,
so set it before boot:

```ini
; DefaultEngine.ini (or a dev-only platform ini) - 1 = counters, 2 = counters per pass/VF + miss logging
[SystemSettings]
r.PSOPrecache.Validation=1
```

or on the packaged command line: `-ini:Engine:[SystemSettings]:r.PSOPrecache.Validation=2`. At level 2 the
`PSO PRECACHING MISS` header block (type, state, material, vertex factory, pass, shader hashes) is logged in
Development and Test builds; the diagnosis that follows it is Development-only (`PSO_PRECACHING_TRACKING`
excludes Test/Shipping).

## Anti-patterns

- Don't gate gameplay on `Get_IsDraining()` - the window is a presentation concern and is inert in the editor.
- Don't call `ResetPSOHitchTrackingStats()` to "start a window" - diff against a baseline, as this module does.
- Don't try to toggle `r.PSOPrecache.Validation` or `r.pso.PrecompileThreadPoolSize` at runtime - both are
  read-only; use the ini recipe.
- Don't treat a drain timeout as a bug to ensure on - it is the fail-open path doing its job.
- Don't add `PrecachePSOsBoostToHighestPriority` here - the engine never calls it and it does not re-prioritise
  the existing queue.

## Packaged-build-only to verify

The real drain reaching 0 (Epic staff note it may not on later runs - the stall timeout covers it), real
progress values, the hitch counters, and the validation counters. Nothing in the editor exercises them.
`[EDITOR-VERIFY]` the simulated path: in PIE set `ck.Pso.Debug.SimulatedRemaining 500`, travel, confirm the
screen holds with the PSO reason under `ck.LoadingScreen.LogReasonEveryFrame 1`, set it to `0`, confirm the
release after the quiet period.

## See also

- `Source/CkLoadingScreen/Claude.md` - the holder seam this module plugs into.

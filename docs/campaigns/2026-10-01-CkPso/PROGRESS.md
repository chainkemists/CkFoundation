# CkPso — PROGRESS

PSO preparation for CkFoundation loading screens. Plan of record:
`C:\Users\neilj\.claude\plans\d-users-neilj-downloads-pso-smart-utils-compiled-orbit.md` (approved 2026-10-01).
Module doc: `Source/CkPso/Claude.md`. Engine research for P2: `P2_WARMUP_RESEARCH.md` (this folder).

## State as of 2026-10-01

| Phase | State | Evidence |
|---|---|---|
| P1 drain gate, progress, hitch telemetry | Implemented; editor-side gate run | see "Gate" below |
| P3 miss-log parser (pure text → record) | Implemented and unit-tested; nothing consumes it yet | `Ck.Pso.MissLogParser.*` 14/14 |
| Measurement stop | **Next. Neil's step** — packaged BusterBlock run | not started |
| P2 manifest warm-up | Research only (`P2_WARMUP_RESEARCH.md`); scope waits on the measurement | — |
| P3 capture device, miss report, debugger tab | Not started | — |
| BusterBlock adoption | Separate follow-up | — |

## What P1 is

- `FCk_Pso_DrainTracker` (`Source/CkPso/Public/CkPso/Drain/`): pure state machine, `Idle → Draining → Settling → Complete | TimedOut`.
- `UCk_Pso_Subsystem_UE` (`Subsystem/`): GameInstance subsystem; holds the loading screen through `ICk_LoadingProcess`
  + `Register_LoadingProcessor`; feeds the tracker `FShaderPipelineCache::NumPrecompilesRemaining()`.
- `UCk_Pso_ProjectSettings_UE` / `UCk_Utils_Pso_Settings_UE` (`Settings/`), cvars `ck.Pso.*`, console command `ck.Pso.DumpReport`.
- Tests: `Ck.Pso.DrainTracker.*` (C++), `Ck_AutoTest_Pso_HoldReleasesAfterQuietPeriod` and
  `Ck_AutoTest_Pso_HoldFailsOpenOnStall` (AngelScript, in CkTests `Script/CkPso/`).

## Gate (2026-10-01, CkPlugins2 host, toolbox v1.51)

Pre-change baseline (serial, fresh boot, untouched tree + the host `PlaceableTests` script fix): 3819 run, 3775 passed,
**44 failed**, 0 contaminated. The 44 are recorded as `knownReds` in the host's `AutomationGate.json`. The four roots
that file adds to the gate (108 tests) all passed on the unmodified tree.

Post-change:
- Build: Succeeded; `Module.CkPso` compiled and linked on the first attempt, no source fixes needed.
- Scoped `Pso`: 37/37 (final run after cleanup, `Saved/Logs/Final-Pso.log`). `LoadingScreen`: 8/8.
- Full suite (two lanes, machine shared with three other sessions' test runs): 3959 run, 52 failed = the 44 known reds
  + **8 new**. None of the 8 is attributed to CkPso:

| New failure | Attribution | Evidence |
|---|---|---|
| `Ck.DebugScene.Target.CapacityAndInvalidInputAreAtomic`, `Ck.Jolt.BakeExtraction.Text3DGlyphExclusion`, `Ck.PathNetwork.Vectorize.Loop`, `Ck.UnrealComponent.TransformPropagation.DirtyOwnersOnly` | Victims of a headless-editor crash, not test failures | Same engine fatal each time: `GenericWindow.cpp:113 GetRestoredDimensions is not expected to be called on this platform`, from `FTabManager::SavePersistentLayout` on a ticker. The same fatal is in the baseline log (it killed `Ck.CrowdDebugger.ProductionLifecycle.Release`, a known red). No CkPso frame in any callstack. All four pass alone. |
| `Angelscript.CppTests.AngelscriptCodeCoverage.IntegrationTest` | Passes alone | Failed on missing coverage reports for host scripts |
| `Ck_AutoTest_GroundNav_Link_DisabledMidCrossingHoldsTheBodyAndResumesOnEnable`, `Ck_AutoTest_GroundNav_Link_TraversalHandshakeFiresExactlyOnce` | Pass alone | Their errors in the gate were another test's log lines (a GameSettings deferred-cvar timeout and its console warning) |
| `Ck_AutoTest_Crowd_AvoidanceVolume_InitialPathAvoidsExpandedObb` | **Pre-existing order/load dependence** | Passed in the serial baseline. Fails alone 2/2 with CkPso, fails after its baseline predecessor, and **fails alone with the CkPso module unloaded and its tests removed** (`Saved/Logs/AB-NoCkPso.log`). Same assertion every time: `Avoid If Possible did not fall back through the sealed corridor: ECk_Nav_PathStatus::Partial`. |

Not covered by any of this: the real PSO drain. Precaching is hard-disabled under `WITH_EDITOR`
(`PipelineStateCache.cpp:4408`), so only the simulated path (`ck.Pso.Debug.SimulatedRemaining`) ran.

## Never compiled

`CkPso_Subsystem.cpp`'s `#if PSO_PRECACHING_VALIDATE && UE_WITH_PSO_PRECACHING` branch (validation hit/miss counters)
only exists in non-editor builds. It was checked against `PSOPrecacheValidation.h` by reading. The first packaged
build is its first compile.

## Decisions on record

- New module, not an extension of CkLoadingScreen; subsystem-shaped, API on the subsystem.
- Gate is default-on, bounded by `_DrainMaxWait` (60s) and `_DrainStallTimeout` (10s).
- A stall is a count that has not CHANGED for the stall timeout. A rising count is work still arriving.
- A Complete window re-opens only while the loading screen is still showing — otherwise gameplay PSO requests would
  put the screen back up mid-game.
- Tracker time is game time clamped to 0.1s per tick (Neil, 2026-10-01): avoids a new row in the wall-time fence
  `Ck.Snapshot.Meta.WallTimeReadsAreAllowListed`. Consequence: pause or a snapshot load's time freeze pauses the budgets.
- A drain timeout logs a Warning, not an ensure.
- Miss capture will be a dev-only log parser (Neil, 2026-10-01), not an engine-fork delegate.
- `ck.Pso.Debug.StallTimeoutOverride` exists so the fail-open AutoTest takes about half a second; a 10s test fails on
  any unrelated Warning the editor logs in its window.

## Open items

1. **Measurement (Neil):** package BusterBlock (Development) against this branch; run cold (`-clearPSODriverCache`)
   and warm; collect `ck.Pso.DumpReport` and the log. For miss counters add
   `-ini:Engine:[SystemSettings]:r.PSOPrecache.Validation=2`.
2. P2 scope. The research found four Ck render paths with no PSO precache coverage at all, which no manifest can fix:
   CkIskmRenderer batched clusters (`CkIskmVF4/8` lack `SupportsPSOPrecaching` and a collector), CkPmg procedural
   meshes, the CkPixelArtRenderer upscaler, and the CkUsf outline compute shaders. Details and citations in
   `P2_WARMUP_RESEARCH.md` §6. Confirmed by reading code, not observed in a packaged run.
3. P3 remainder: capture `FOutputDevice`, miss report, debugger surface. The parser reports blocks it recognises but
   cannot decode as unset; the capture layer must count and surface those (engine format drift would otherwise read
   as "0 misses").
4. Progress ratio stays at 100% if a window re-opens. Knob: `DoUpdate_ProgressRatio` in `CkPso_DrainTracker.cpp`.
5. Host-side, not CkPso: the `GetRestoredDimensions` headless crash, and the order-dependent Crowd AutoTest above.

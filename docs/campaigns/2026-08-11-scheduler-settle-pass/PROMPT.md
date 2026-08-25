# Scheduler settle pass — pump-originated state becomes phase-visible in-frame

**Status: SUPERSEDED 2026-08-21 — do not execute.** Upstream landed group-local settle barriers (`e4cf54edf`, `LocalSettleAfter`/`LocalSettleTrigger`) and `FGroup_Transform_Derived` (`f7703bef9`) on 2026-08-20; this package proposed a parallel post-pump mechanism. The replacement direction is a `LocalSettleAfter = FGroup_Gameplay_Script` barrier on the SM cascade + EntityScript spawn-pipeline processors — package: `docs/campaigns/2026-08-21-gameplay-cascade-settle-barrier/` (consumed markers — conformant) so cascade-originated transform requests reach the ordinary drain/push same-frame, with #717 kept as the floor for tail-pump-drained writes. See BusterBlock `docs/digests/2026-08-21-settle-pass-frame-timing.html` ⑥/⑦. Original line follows for history: Do not begin Phase 0 until this line is
replaced with `Status: APPROVED <date>`.

## Problem

The scheduler tick has two phases: the main pass (every group, topological order) and the pump
loop at the very end (`FProcessorScheduler::Tick`, `CkProcessorScheduler.cpp` — main-pass block,
then the `_MaxPumpIterations` loop). The pump delivers **request-level** quiescence but not
**phase-level** quiescence: a transform request drained in a pump pass mutates the fragment
correctly, but every propagation phase (`FGroup_Transform` → `Transform_Finalize` →
`FGroup_PostTransform`) already ran this frame. Derived views of that state — scene-node
composition, `USceneComponent` pushes, world-space widgets — catch up one frame later at best.

The stated design intent of `FGroup_PostTransform` ("runs after everything is done — when we
know we have the correct transforms") is today only satisfied **across a frame boundary**.
Measured consequences: the rewind-station one-shot bugs (fixed at the consumer by
`fix(unrealcomponent): deliver pump-drained one-shot transform pushes` — the
`LastPushedTransform` memory, PR #717) and, predicted but unverified, the one-frame
giant-scale flash when placing items.

Evidence (all from the 2026-08-11 investigation, this repo + BusterBlock):
- `Saved/CkEcs/SchedulerOrder.txt` (regenerate via in-PIE `Ck.Ecs.Scheduler.ExportOrder`):
  Cleanup at slot ~588, `Transform_HandleRequests` ~603, `UnrealComponent_PushTransform` ~677,
  pump after the entire main pass.
- Instrumented interleave (BusterBlock `Build/postrebase_*.log` frames 210–220): pump-drained
  write tagged after the push slot, cleared by next frame's Cleanup before its push slot.

## Chosen approach — the SETTLE PASS (option "1b-lite" from the design discussion)

After the pump loop converges, dispatch **once** a small, explicitly whitelisted chain of
propagation processors ("the settle set") over the settled state, with `DeltaT = 0`:

```
main pass (unchanged) → pump loop (unchanged) → SETTLE: whitelisted propagation, once → done
```

- Propagation runs **once per frame, after all pumping** — never per pump pass (that was a
  different, rejected attempt; see below).
- **No outer loop** in this campaign: if the settle pass itself enqueues new gameplay work, it
  drains next frame's main pass, exactly as pump leftovers do today.
- **Opt-in whitelist**, not opt-out: only processors explicitly annotated participate. Phase 2's
  whitelist is exactly seven processors + one template (see PHASE_2.md). Everything else —
  physics, external-state samplers, event emitters (probes/RaySense/EQS), FireSignals,
  replication — is untouched and keeps today's timing.
- Gated behind a project setting, **default OFF**, so the merged code is provably zero-change
  until the maintainer flips it.

## Rejected approaches (do not resurrect)

1. **Per-pump-pass participation** (`MarkedDirtyBy = FTag_Transform_Updated` on the push) —
   tried 2026-08-11: fires the scheduler's dirty-marker-conflict diagnostic (7 unordered
   co-consumers of the tag; the required `RunAfter`s are cross-module and layering-impossible)
   and destabilized a BusterBlock test. Also violates "handle once at the end".
2. **Mid-frame pump (pump before the transform phases)** — relocates the deferred edge to
   *phase-output → gameplay-reaction*, a busier edge, and still needs a post-phase pump.
   The frame's dependency graph is cyclic; killed in design review.
3. **Ungated per-tick push** — broke the external-drift contract pinned by
   `Ck.UnrealComponent.TransformPropagation.DirtyOwnersOnly`.
4. **Opt-out settle membership (whole groups by default)** — `FGroup_Transform` contains
   non-re-runnable members (`FProcessor_JoltWorld_WaitForAsync`, VoxelNav Jolt-touchers) and
   `FGroup_PostTransform` contains event emitters; auditing ~40 processors for idempotency is
   the risk. The whitelist inverts the risk: nothing participates until proven safe.

## Deliberately deferred (future campaigns, each needs its own approval)

- Emitter relocation (probes/RaySense/EQS evaluate settle-only — single-eval on final data).
- Tail reorder (DeferredApply/Replication/lifecycle after the settle — fixes the same-frame
  replication gap for pump-originated state).
- Outer fixpoint loop (settle output re-pumps, capped).
- `FireSignals` in the settle set (same-frame OnTransformUpdate for pump writes).
- Removing `FFragment_UnrealComponent_LastPushedTransform` — do NOT remove it in this
  campaign; it is the graceful-degradation layer when the setting is off and under caps.

## File inventory (why each matters)

| File | Why |
|---|---|
| `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorScheduler.h/.cpp` | Tick's main/pump blocks; settle block + `_SettleOrder` + stats go here |
| `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorTraits.inl.h` | Trait detection (mimic `PumpPolicy`/`EmptyViewPolicy` SFINAE); descriptor derivation |
| `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorDescriptor.h` | `_ParticipatesInSettle` descriptor field |
| `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessorGraph.h/.cpp` | Node fields copied from descriptor; where `_MainPassOrder`/`_PumpOrder` are built |
| `Source/CkEcs/Public/CkEcs/Subsystem/CkEcsWorld_Subsystem.cpp` | `Ck.Ecs.Scheduler.ExportOrder` command — extend the dump with a `[Settle]` section |
| `Source/CkEcs/Public/CkEcs/Settings/CkEcs_Settings.h/.cpp` | `_EnableSchedulerSettlePass` toggle + getter (mimic `_EnableEmptyViewMainPassSkip`) |
| `Source/CkEcsExt/.../Transform/CkTransform_Processor.h` | Whitelist annotations: `HandleRequests`, `SyncToActor` |
| `Source/CkEcsExt/.../SceneNode/CkSceneNode_Processor.h` | Whitelist: `SceneNode_HandleRequests`, `TProcessor_SceneNode_Update<LayerN>` |
| `Source/CkUnrealComponent/.../CkUnrealComponent_Processor.h` | Whitelist: `PushTransform` |
| `Source/CkWorldSpaceWidget/...` (find the processor header) | Whitelist: `UpdateLocation`, `UpdateScaling` |
| BusterBlock `Plugins/BusterBlockTests/Script/Tests/UnrealComponent/` | The red spec test (Phase 0) + the existing `OneShotPushReachesComponent` regression pin |

## Glossary

- **Pump / pump pass**: end-of-tick loop re-running `MarkedDirtyBy` processors until no work.
- **Settle pass**: the new, single post-pump dispatch of the whitelisted propagation chain.
- **Settle set / whitelist**: processors annotated `SettleParticipation = Participate`.
- **Deferred edge**: the leg of the gameplay→phases→reactions cycle that waits a frame.
- **`LastPushedTransform`**: per-owner memory from PR #717; the push's own change detector.

## Skills to load, and when

- Before ANY code: `ck-macros-and-codegen` (trait/descriptor conventions), root `CLAUDE.md`
  style (already summarized in each phase's signatures).
- Before Phase 0: `ck-game-testing-discipline` + BusterBlock `Script/CLAUDE.md` AutoTest section.
- Before Phase 2 gates: `ck-change-control`.
- If any build/AS failure: `ck-debugging-playbook`; check `ck-failure-archaeology` before
  retrying anything twice.

## Executable spec

Phase 0 authors it: `Bb_AutoTest_UnrealComponent_OneShotPush_SettlesSameFrame` — asserts the
component matches a pump-drained one-shot **at the immediately-next sequencer step** (zero
settle frames). Expected RED under current code (delivery is next-frame), GREEN exactly when
the settle pass works. Exact invocation and expected outputs in PHASE_0.md.

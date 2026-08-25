# Phase 2 — whitelist the propagation chain; the spec goes green

## Entry criteria
- Phase 1 exit criteria met (plumbing committed, toggle off, suite at baseline).

## The whitelist — exactly these, nothing more

Annotate each with `static constexpr auto SettleParticipation = ECk_ProcessorSettleParticipation::Participate;`:

| # | Processor | File | Why safe (idempotent w.r.t. re-dispatch at dt=0) |
|---|---|---|---|
| 1 | `FProcessor_Transform_HandleRequests` | `CkEcsExt/.../CkTransform_Processor.h` | queue drain; empty queue = no-op (its own `Has_AnyLiveEntityWith` guard already short-circuits) |
| 2 | `FProcessor_SceneNode_HandleRequests` | `CkEcsExt/.../CkSceneNode_Processor.h` | queue drain |
| 3 | `TProcessor_SceneNode_Update<LayerN>` (the template — annotation covers all layers) | same | tag-gated recompose; no tag = no-op |
| 4 | `FProcessor_Transform_SyncToActor` | `CkTransform_Processor.h` | dirty-gated actor-root mirror |
| 5 | `FProcessor_UnrealComponent_PushTransform` | `CkUnrealComponent/.../CkUnrealComponent_Processor.h` | memory-gated (`LastPushedTransform` equality) |
| 6 | `FProcessor_WorldSpaceWidget_UpdateLocation` | CkWorldSpaceWidget (locate the processor header) | transform mirror |
| 7 | `FProcessor_WorldSpaceWidget_UpdateScaling` | same | transform mirror |

## Fences — explicitly NOT whitelisted, with reasons
- `FProcessor_SceneNode_FollowUnrealAnchor` — samples external Unreal components; once/frame by design.
- `FProcessor_Transform_FireSignals` — deferred-scope decision (same-frame signal fires change
  observable ordering); future campaign.
- `FProcessor_JoltWorld_WaitForAsync`, any Jolt/VoxelNav member of `FGroup_Transform` — physics
  stepping/wait, not idempotent.
- Probes / RaySense / EQS / OverlapBody — event emitters; re-evaluating mid+final would churn
  enter/exit. Deferred (emitter-relocation campaign).
- `FProcessor_Transform_Cleanup` — must NOT run in settle (it would clear tags the settle's
  scene-node layers still gate on mid-chain... and it is the frame-boundary janitor, not propagation).
- If the scene-node layer template's declaration shape resists a single annotation (per-layer
  explicit classes rather than one template), annotate each layer class individually — do not
  restructure the template.

## Steps
1. Add the seven annotations (+ template) — nothing else in those files changes.
2. Flip the setting ON for verification only: add `_EnableSchedulerSettlePass=True` under
   `[/Script/CkEcs.Ck_Ecs_ProjectSettings_UE]` in BusterBlock `Config/DefaultCkFoundation.ini`.
   (This edit ships only if the maintainer wants default-on; otherwise revert after gates and
   note the flip procedure in VALIDATION.md.)
3. Build + run, editor closed, `-DisablePlugins=RiderLink` env:
   `./CkAuto/UnrealToolbox.exe --build --test --test-pattern SettlesSameFrame`

## Decision gates
- **Expected: `SettlesSameFrame` PASSES** (Total 1, Passed 1). Still red → STOP, blockers —
  paste the fresh failure line AND a fresh ExportOrder dump's `[Settle]` section (does it list
  the seven?); do not iterate blindly.
- `--test-pattern OneShotPushReaches` → pass (Total 1, Passed 1).
- `--test-pattern Transform` → **47/47 — `DirtyOwnersOnly` is the one to watch**: the settle
  pass runs the push more often, but the memory gate means idle owners are still never written.
  If `DirtyOwnersOnly` fails → STOP; the whitelist or the push gate is wrong, do not patch the spec.
- `--test-pattern Tween` → 23/23. `--test-pattern Rewind` → compare against the recorded
  baseline (2 known inherited JoltShape-content reds may still be present — same names only).
- Full suite: counts vs PROGRESS.md baseline; only the recorded known-reds. Grep the run log
  for `Dirty marker conflict` → must be 0.
- Anything else → STOP, blockers.

## Exit criteria
- Spec green; all gates at baseline; commits on the campaign branch:
  `feat(scheduler): whitelist the propagation chain for the settle pass` (+ the BusterBlock
  test/config commits in the BB repo on a matching branch).

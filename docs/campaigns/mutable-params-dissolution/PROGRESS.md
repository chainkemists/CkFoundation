# PROGRESS — mutable-params-dissolution

Living doc. Newest entries on top within each section. See PROMPT.md for scope, rules and method.

## State

- **Phase:** NOT STARTED. Scope defined 2026-08-06 from the audit at the end of the
  `spec-fragment-granularity` campaign; nothing executed yet.
- **Branch:** `refactor/spec-fragment-params-residues` (CkFoundation), tip `6f5d12b12`, based on
  `dev` at `4afcd039f`. Local only — nothing pushed, no submodule pointer bumps.
- **Baseline to diff against:** 1004 total / 1002 passed / 2 failed. The two reds are the
  pre-existing `PathNetworkFollower_*` pair (sibling navmesh work), NOT ours. Known contention
  flake: `CkJolt_ChaosParity_CcdProjectileStopsAtThinWall` — re-run the full suite before blaming a
  change for it. Full detail in PROMPT.md § Baseline.

## Feature ledger

Order is the recommended one; CkCompass first because it is CkMinimap's twin and the least
defensible thing to leave behind.

| # | Feature | Status | Commit | Gate |
|---|---|---|---|---|
| 1 | CkCompass | not started | — | — |
| 2 | CkUI WorldSpaceWidget | not started | — | — |
| 3 | CkPathNetwork + Follower | not started | — | — |
| 4 | CkGoap Action → CkAStar Params | not started | — | — |
| 5 | CkCameraShake | not started (confirm the mutation exists first) | — | — |
| 6 | CkCrowdAgent | not started (confirm the mutation exists first) | — | — |
| 7 | CkPmg Text | not started | — | — |
| 8 | CkVoxelNavPath | not started | — | — |
| 9 | CkAStar test utils | not started | — | — |

## Log

- 2026-08-06: campaign scoped. Scan A (`TReadWrite<FFragment_*_Params>` in a processor view) is the
  reliable structural tell and returned the six processor-side entries above; scan B
  (`Set_*` onto a retained Params) added three Utils-side ones and ~170 legitimate
  build-a-local-Spec-then-Add hits that are NOT defects. Predecessor campaign fixed CkMinimap
  (`9347e2062`), which is the template.

## Scope (measured 2026-08-06 on the branch)

- **24 Spec-wrapper fragments** (22 files; CkAcceleration and CkVelocity carry two each):
  CkAnimation ×3, CkCamera ×2, CkChaos, CkEntityCollection, CkFx ×2, CkGraphics,
  CkInteraction ×3, CkOverlapBody ×2, CkPhysics ×6, CkResolver ×3.
- **72 `FFragment_*_Current` fragments** to retire (ruling 3). Full list reproducible with:
  `Select-String -Pattern '^\s*struct\s+\w*API\s+FFragment_(\w+)_Current\b'` over `Source/**/*.h`.

~96 conversions total, each needing per-field read analysis, consumer updates across
processor/utils/debugger, and its own gate. This is a multi-session campaign; work it in gated
batches and keep this ledger current, because the checkout is shared and context is lost easily.

## Wrapper residue worksheet (derived 2026-08-06)

For a WRAPPER fragment the analysis is mechanical: every `Get_Params().Get_X()` read in the tree IS
a steady-state read, so that set is the residue and every other Spec field is construction-only.
Regenerate with:

```powershell
Get-ChildItem -Recurse -Include *.h,*.cpp -File |
  Select-String -Pattern 'Get_Params\(\)\.(Get_\w+)' -AllMatches
```

**Validate each hit before trusting it** — the pattern also catches `Get_Params()` on things that
are not our fragment (`CkMeter.cpp|Get_Capacity`, `CkWidgetComponent.cpp|Get_WidgetSpacePolicy` are
known false positives from this run).

| Feature | Residue fields (steady-state reads) |
|---|---|
| CkAcceleration | `_AccelerationParams`, `_TargetChannels` |
| CkAnimPlan | `_AnimGoal` |
| CkAutoReorient | `_ReorientPolicy` |
| CkEntityCollection | `_Name` |
| CkGeometryCollection | `_GeometryCollection` |
| CkInteraction | `_InteractionChannel`, `_Source`, `_Target`, `_CompletionPolicy`, `_Instigator` |
| CkInteractSource | `_ConcurrentInteractionsPolicy`, `_InteractionChannel` |
| CkInteractTarget | `_CompletionPolicy`, `_InteractionChannel`, `_InteractionDuration`, `_ConcurrentInteractionsPolicy` |
| CkMarker | `_AttachmentParams`, `_RelativeTransform`, `_ShapeParams`, `_DebugParams`, `_PhysicsParams`, `_ReplicationType` |
| CkMontagePlayer | `_SkeletalMeshComponent` |
| CkPmg Donut | `_EnableCollision`, `_FillAngle`, `_InnerRadius`, `_Material`, `_OuterRadius`, `_RenderMode`, `_Segments` — likely 1:1 with its Spec, so a strong **alias** candidate (ruling 1) |
| CkResolverDataBundle | `_Causer`, `_Instigator`, `_Phases`, `_Target` |
| CkResolverSource | `_ResolutionPhases` |
| CkSensor | `_AttachmentParams`, `_FilteringParams`, `_RelativeTransform`, `_SensorName`, `_ShapeParams`, `_DebugParams`, `_PhysicsParams`, `_ReplicationType` |
| CkVelocity | `_TargetChannels`, `_VelocityParams` |
| CkVfx | `_AttachmentSettings`, `_ParticleSystem` |

**Wrappers with ZERO `Get_Params()` reads** — CkAnimAsset, CkCameraShake, CkRenderStatus, CkSfx,
CkPredictedVelocity, CkBulkAccelerationModifier, CkBulkVelocityModifier, CkResolverTarget. Their
whole retained Spec is read by nothing. Check whether the fragment survives ONLY as the Has/Cast
anchor; if so the residue may be empty, and the anchor should move to the feature's state fragment
(the Timer pilot's precedent) rather than keeping an empty Params alive.

## Log (cont.)

- 2026-08-06: **CkCamera converted** (`fd38e91bb`) — first application of ruling 1. `FCk_Camera_Spec`
  is two fields; only `_DriveControllerControlRotation` is read back, so an entire
  `FCk_CameraProfile` was retained per camera entity as dead weight. Residue is that one bool.
  **Gate incomplete:** compiles clean (no CkCamera diagnostics), but the full suite was never
  reached — CkTests and CkGameplayDebugger had been moved by a sibling session to
  `feature/usf-celshade-dither-stylize` and `dev`, neither of which pairs with this branch. Restore
  with `git -C Plugins/CkTests switch --detach 2cd9c62` and
  `git -C Plugins/CkGameplayDebugger switch --detach 28fad97`, then re-gate.
- **Submodule pairing is a PRE-GATE CHECK.** Three times this campaign a sibling session moved a
  shared checkout mid-work. Before every gate, verify all three submodules:
  CkFoundation on this branch, CkTests at `2cd9c62`, CkGameplayDebugger at `28fad97`. A mismatch
  produces compile errors in modules you never touched, which reads exactly like your own regression.

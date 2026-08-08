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

## Log (cont. 2)

- 2026-08-06/07: **WRAPPER WORKSTREAM COMPLETE AND GATED.** `30ea12fd3` (the batch) + `ef5a8ffd6`
  (the fixes real compilation found) + CkTests `f9d592cc` + superproject `9261659`.
  **Gate: build clean, zero AngelScript errors, full suite 1004 / 1002 passed / 2 failed** — the two
  being the known pre-existing PathNetworkFollower pair BY NAME with identical assertion text.
  Final tally: **22 of 24 aliased**, 2 reverted (empty Spec), 1 untouched (CkMontagePlayer).

- **RULING 1 NEEDS A THIRD BRANCH: the EMPTY Spec.** `FCk_ResolverTarget_Spec` and
  `FCk_PredictedVelocity_Spec` have no data members at all. `CkRegistry.h` classifies EMPTY types as
  TAGS and static_asserts they derive from `ck::TTag`, so aliasing them turned two fragments into
  tags and broke the build. **The wrapper is load-bearing there** — wrapping an empty Spec in a
  struct with a member makes it non-empty, hence a fragment. Decision tree is therefore:
  - retained fields a strict subset of the Spec → residue struct;
  - retained fields match the Spec 1:1 → `using FFragment_X_Params = FCk_X_Spec;`;
  - **Spec is EMPTY → leave the wrapper.** The fragment carries no data and exists only as the
    Has/Cast anchor. Doctrinally that wants a real `CK_DEFINE_ECS_TAG`, but that changes
    Add/Has/Cast semantics and the anchor — a design call, not a mechanical conversion.

- **PRE-GATE CHECK IS FOUR REPOS, NOT THREE.** The earlier note said "verify all three submodules".
  That was wrong and cost a full gate cycle: the **superproject** is itself a consumer of the
  renamed API (`CkPlugins/Script/PlaceableTests/*.as`), and it was sitting on `dev` while the three
  submodules were on the branch. Worse, the superproject's campaign commit `af63213` was
  **ORPHANED** — `git branch --contains` returned nothing, no ref pointed at it, and it would
  eventually have been garbage-collected. It is now `9261659` on a real superproject branch.
  Before EVERY gate verify all four: superproject, CkFoundation, CkTests, CkGameplayDebugger.

- **What real compilation caught that static analysis could not** (4 classes, 3 compiles):
  1. An alias cannot be forward-declared (`struct FFragment_X_Params;` → C2371, ~46 cascade errors).
  2. Sweeps scoped to CkFoundation/Source MISS CkTests and CkGameplayDebugger — they are separate
     submodules. A "zero residual" audit is only as wide as the tree you walked.
  3. Empty Specs register as tags (above).
  4. Superproject AngelScript on an orphaned commit — and AS failures fail the run via
     `AS_COMPILE_FAILED` (exit 76) with **zero tests executed**, because the editor keeps stale
     bytecode. A suite that reports Total: 0 is not a pass.

## Log (cont. 3) — 2026-08-08: ruling 3 discharged, Params tail closed

- **RULING 3 DONE — `_Current` is purged.** 83 fragment types / 2380 refs across CkFoundation,
  CkTests and CkGameplayDebugger. Target name is the bare `FFragment_[Feature]` from the two-tier
  table; verified zero collisions against all 415 declared fragment names AND every `using`-alias
  first, and confirmed none of the 83 was a reflected USTRUCT (so no BP/serialization exposure).
  Parameter `InCurrent` followed its type to `In[Feature]`.

- **A REGEX THAT MISSES AND AN AUDIT THAT AGREES WITH IT.** The first sweep matched
  `FFragment_([A-Za-z0-9]+)_Current`, which cannot match a feature segment containing an underscore
  — so `Pmg_DebugShape`, `Pmg_Donut`, `Goap_Planner` and `Goap_Action` (246 refs) were silently
  skipped. The audit then reported "0 residual" because **it reused the same narrow pattern**. It
  surfaced only via an unrelated grep. Two rules follow, and they generalize past this campaign:
  1. Feature segments are NOT `[A-Za-z0-9]+`. Use `[A-Za-z0-9_]+` (greedy, anchored on the suffix).
  2. **Never audit with the expression you edited with.** A sweep and its verification must not
     share a regex, or the verification can only confirm the sweep's own blind spot. Verify with a
     different tool (`rg` over the tree) than the one that made the change.

- **SCAN D — direct member writes.** Scans A (writable view), B (`Set_` on a param) and C
  (`Get<>().Set_`) all miss `InParams._Field = ...`, which is legal inside a friend. That form is
  what hid CkCrowdAgent's `_MaxSpeed` and CkPathNetwork's `_Ribbons` through two earlier passes and
  produced two wrong "false positive" calls. Scan D is
  `In(Params|Tunables)\._[A-Za-z0-9]+ *=` — note it also matches `==`, so read the hits.

- **Scans key on the TYPE, not the parameter name.** `CkPathNetwork_Processor.cpp` hosts both the
  network handler and the follower handler, and both name their parameter `InParams` even though
  the follower's type is `_Tunables`. A file-scoped rename would hit the wrong one, so the name was
  left alone deliberately — expect scan B to report it forever. Judge by the fragment type.

- **`_Tunables` is now a documented category, not an ad-hoc escape.** Config that a request or the
  debugger REPLACES at runtime, where there is no immutable residue to split off. Members:
  CkCrowdAgent (19 fields; the Crowd debugger's tuner live-writes 5), CkAStar,
  CkPathNetworkFollower, CkVoiceTalker. Recorded in the root doctrine's two-tier table.

- **Params tail closed.** Split: CkVoxelNavPath (`_Volume` → its own binding fragment, deliberately
  NOT merged with `_Result._Volume`, which records what a COMPLETED plan was made against),
  CkPathNetwork (`_Ribbons` → `_Graph`, beside the `_Network` they build), CkPmg Text (`_Text` →
  `FFragment_Pmg_Text`). Dissolved the last two Spec-wrappers: CkMontagePlayer (one-field Spec, the
  field is a rebindable skeletal mesh) and CkPmg Donut (1:1 and unmutated → alias per ruling 1).

- **Still open, by choice:** CkUI `WorldSpaceWidget` (4 `Set_*`) — deferred at the maintainer's
  request while renames are in flight there. `FFragment_IsmRenderer_Params` keeps its name but is
  NOT a Spec-wrapper: it wraps `TWeakObjectPtr<const UCk_IsmRenderer_Data>`, a data-asset pointer.

- **Toolbox trap: do not redirect a detached run's stdout into `Saved/Logs/`.** The toolbox's
  editor guard probes `Saved/Logs/*.log` for an exclusive write lock. A `Start-Process`
  `-RedirectStandardOutput` pointed there holds such a lock, so the toolbox detects the launcher's
  own redirect file and waits forever on a nonexistent editor. Redirect to the scratchpad. Verify a
  claimed editor two ways before overriding — no `UnrealEditor` process AND `CkPlugins.log` free.

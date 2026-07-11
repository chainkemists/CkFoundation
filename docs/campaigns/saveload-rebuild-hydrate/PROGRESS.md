# PROGRESS.md — CkSnapshot Rebuild+Hydrate campaign

> Executor: update this file at the END of every session. Next session trusts THIS file over memory.
> Blockers go in §Blockers — never improvise around one.

## Status board

| Phase | Doc | Status | Session date | Commits (repo: hash) | Gate result |
|---|---|---|---|---|---|
| 0 | PHASE_0.md | DONE | 2026-07-11 | CkF: 68ba192dc (dt==0), 55521d493 (oracle), <docs> (this); CkTests: 14d65ac (harness) | GREEN: Ck.Snapshot 47/47/0 (46 baseline + Oracle.StructuralBaseline), Ck.Attribute.Net 17/17/0, Net 102/101/1 (baseline red only) |
| 1 | PHASE_1.md | **DONE** (1.1–1.6 committed+GREEN) | 2026-07-11 | CkF: 23b982c0d (framework), fd288efdd (6 migrations), 5d262f52a; CkTests: b9e7f86 | GREEN: Ck.Snapshot **48/48/0** delta-zero (all 11 Parity_MPReload + both Oracle tests pass), framework Ck.*.Net green (kiosk trio env-red, see baseline) |
| 2 | PHASE_2.md | **DONE** (2.1–2.7 committed+GREEN; 2.7-Test2 subsumed per [P2-D3]) | 2026-07-11 | CkF: 91b96a177 (load gate core), 8c4fbce7a (fire-gating), af9fad239 (retire NeedsSetup guards); CkTests: 4522c8e (LoadGate) | GREEN: Ck.Snapshot **49/49/0** (LoadGate + AccelerationParity + 11 Parity, no over-gating hang); Net 102/98/4 framework delta-zero (3 Attribute.Net pins green; 4 fails = recorded kiosk-trio env-red + StateMachine.Net flake) |
| 3A | PHASE_3A.md | NOT STARTED | | | |
| 3B | PHASE_3B.md | NOT STARTED | | | |
| 4A | PHASE_4A.md | NOT STARTED | | | |
| 4B | PHASE_4B.md | NOT STARTED | | | |
| 5 | PHASE_5.md + VALIDATION.md | NOT STARTED | | | |

## Unattended execution protocol (set 2026-07-11 by Adam — OVERRIDES the "STOP on divergence" default below)

Run the campaign UNATTENDED through all remaining phases: finish §1.6, then Phases 2 → 3A → 3B → 4A → 4B → 5 in
order. Per-phase loop: read PHASE_N.md (+ the spec and PHASE_1_RESEARCH.md), implement on Opus, gate via
UnrealToolbox (editor CLOSED; read verdicts from the --output logs), and when the gate is GREEN + delta-zero vs the
baselines, COMMIT that phase (never push; stage only files you changed by name) + update this file, then proceed to
the next phase automatically. Do not stop between green phases.

QUESTIONS / DIVERGENCES → delegate, don't halt. When a step is ambiguous, or reality diverges from the plan, and the
answer is NOT already in the PHASE docs / spec / PHASE_1_RESEARCH.md: launch a **Fable-class agent** (Agent tool,
`model: "fable"`) to research the codebase + decide the question (Fable has the reasoning depth for design/architecture
forks), VERIFY its ruling against the cited code yourself, record it in §Decisions here, then RETURN TO OPUS to
implement. Never improvise architecture on the Opus main loop — route every design fork through a Fable agent.
Reserve a true STOP (→ §Blockers, end the run) ONLY for: (a) a red gate you cannot fix against the plan/research even
after a Fable consult; (b) an irreversible/outward/destructive action (push, force-push, cross-repo merge, deleting
another session's work); or (c) a genuine human-only product/risk/authority decision the Fable agent explicitly flags
as needing Adam. Cross-repo/CkTests-ahead-of-CkFoundation discipline and "no push" still hold absolutely.

## Phase-0 baseline table (fill in Phase 0; every later phase diffs against THESE names)

Captured 2026-07-11 on `feature/save-load-improvements` @ `bbde1a9dd` (clean tree, no edits), via UnrealToolbox
`--build --test` (Editor/Development). Verdicts read from `CkAuto/logs/p0-baseline-*.log`.

| Pattern | Total | Pass | Fail | Failing test names |
|---|---|---|---|---|
| Ck.Snapshot | 46 | 46 | 0 | (none) |
| Ck.Attribute.Net | 17 | 17 | 0 | (none) |
| Net | 102 | 101 | 1 | Ck.StateMachine.Net.OwningClientAuth_SubSm_AuthorityGatedTask |
| (full suite) | not run this phase | | | (VALIDATION.md runs full suite at Phase 5) |

The lone `Net` red is PRE-EXISTING (zero source edits at capture time). Every later phase re-running `Net` must show
exactly this one failure by name and no others — **PLUS the kiosk-trio environmental caveat below.**

**[§1.6 update] `Net` pattern env-red trio (record + diff against these going forward).** The `Net` substring pattern
sweeps in BB gameplay AutoTests. Three BB kiosk-DESTRUCTION AutoTests fail environmentally (timing-sensitive settle
windows under machine load — NOT framework, NOT campaign): `Bb_AutoTest_RentnetKiosk_DamageToDestroy`,
`Bb_AutoTest_RentnetKiosk_DispensesLootOnDeath`, `Bb_AutoTest_RentnetKioskDriver_SpawnAndRelease`. Proven outside the
campaign's blast radius: the failing tests compose their entities `ECk_Replication::DoesNotReplicate`
(`Plugins/BusterBlockTests/Script/Tests/RentnetKiosk/BB_AutoTest_RentnetKiosk_DamageToDestroy.as:39,46,53,59`), so the
replicated-fragment registry / Produce / re-drive the campaign touches is never consulted (Fable ruling [BI-1], verified).
The framework `Ck.*.Net` set is delta-zero (the `Ck.StateMachine.Net.OwningClientAuth_SubSm_AuthorityGatedTask` baseline
red FLAKES green/red across runs — a known flake). So the `Net` gate for later phases = **framework `Ck.*.Net` delta-zero
+ the kiosk trio (env, ignorable) + StateMachine flake (ignorable)**. A NEW `Ck.*.Net` red = real stop-condition.

**[§1.6 note] Branch base changed mid-session (one-time integration, NOT ongoing).** `feature/save-load-improvements`
was rebased at 2026-07-11 16:55 local (reflog `rebase (finish)`) to fold in the **object-pooling-core** campaign
(+3758 lines: new CkObjectPooling/CkArchetype/CkDebugFeatureFlags code under CkCore/CkEcs, plus CkEcs Scheduler +
EntityScript edits). Old tip `49cfdb038` → new tip `951112723` (same subjects, new hashes; old commits survive in
reflog). HEAD stable since. The §1.6 build+gate ran AFTER this (build compiled `Module.CkObjectPoolingDebugger.cpp`),
so the green gate reflects the INTEGRATED base. **Consequence for Phase 2+:** the object-pooling work modified
`CkProcessorDescriptor.h` (+5), `CkProcessorScheduler.cpp` (+28), `CkProcessorGraph.h/.cpp` (+26) — exactly the Phase-2
scheduler targets. Phase-2 line refs in PHASE_2.md are SHIFTED; re-locate insertion points by PATTERN (PumpPolicy trait,
BuildDescriptor slot, DoCreateNodes mirror) against current code, and watch for interaction between object-pooling's
scheduler additions and the new `ECk_ProcessorLoadPolicy`/`FGroup_Hydration` work. Verified current lines this session:
`ECk_ProcessorPumpPolicy` enum at `CkProcessorDescriptor.h:79-84`, `_PumpPolicy` field at `:145`.

Census (invocation-only, Phase 0, 2026-07-11): CK_REGISTER_SNAPSHOTABLE = **127** across **20** modules
(CkAnimation, CkAttribute, CkDynamic, CkEcs, CkEcsExt, CkEntityCollection, CkEntityTag, CkGrid, CkInteraction,
CkInventory, CkLabel, CkObjective, CkPhysics, CkRelationship, CkRenderTarget, CkSnapshot, CkSpatialQuery,
CkStateMachine, CkTagSet, CkTimer); FCk_RepData_* struct decls = **23**. (Spec §2's "119 / 18 modules / 24 RepData"
reconciled to these in the same commit.)
Load-time baseline (Phase 3B, representative fixture): ___ ms.

## Phase-2 progress (sub-step tracker — for mid-phase continuation; Phase 2 is ONE atomic gate)

Base: integrated `951112723` + §1.6 commits (`5d262f52a`/`4fb178824`/`df06b8394`). PumpPolicy mirror-map (current lines):
`ECk_ProcessorPumpPolicy` enum `CkProcessorDescriptor.h:80`; `_PumpPolicy` field `:145`; BuildDescriptor slot
`CkProcessorTraits.inl.h:216-218`; node field `CkProcessorGraph.h:79-81`; DoCreateNodes copy `CkProcessorGraph.cpp:223,261`;
scheduler precompute `CkProcessorScheduler.cpp:70-81` (`_MainPassOrder`/`_PumpOrder`), main pass `:105-149`, pump `DoPump :205-325`;
`.h` members `_MainPassOrder`/`_PumpOrder` `CkProcessorScheduler.h:42-43`, `Tick` sig `:22`, `DoPump` sig `:27-29`.

- [x] 2.1 `ECk_ProcessorLoadPolicy` trait (mirror PumpPolicy) + scheduler `_LoadPassOrder`/`_LoadPumpOrder` + `ECk_SchedulerTickScope{Full,LoadKernel}` Tick/DoPump scope + subsystem `Get_/Set_IsLoadGateActive` + Actor::Tick threading (lazy weak-ptr `_OwningSubsystem`) + `Request_PumpToQuiescence` scope param — DONE
- [x] 2.2 Mark the kernel RunsDuringLoad — DONE, verified 12 (grep 13 incl. the descriptor doc-comment; reworded so exit grep == 12)
- [x] 2.3 `FGroup_Hydration` between PostTransform & Replication (fwd-decl + struct + re-point Replication RunAfter + chain comment + CK_REGISTER_GROUP) + BOTH dispatchers → `FGroup_Hydration`, deleted their `RunAfter FinishConstruction` (group order guarantees it) — DONE
- [x] 2.4 `FTag_EntityScript_ConstructedThisFrame` defer — **DONE.** Tag added (CkEntityScript_Fragment.h); FinishConstruction stamps it (.cpp:361 area); both dispatchers skip at top of ForEachEntity; hydration dispatcher `RunAfter net` + `DoTick` override clears it registry-wide (ck_exp pattern `TProcessor::DoTick(InDeltaT)` then `_TransientEntity.Clear<>()` — VERIFIED ck_exp::TProcessor exposes a public view-iterating DoTick, CkProcessor.h:350). [P2-D1] applied.
- [x] 2.5 Fire-gating — **DONE, via Fable ruling [P2-D2].** `Get_HasUndrainedReplicatedFragments_IncludingDependents` recurses the LIFETIME-DEPENDENTS tree (`Get_LifetimeDependents`, the same traversal the expected-dependent count derives from — DoCount at CkEntityReplicationDriver_Utils.cpp:53-79 is the mirrored precedent), NOT a driver-side list (none exists). Per-entity predicate is TWO checks: `Has<FTag_RepFragments_PendingApply>() || Has<FFragment_PendingHydration>()` (the tag subsumes queued removals; `_Fragments` is private/Utils-not-friend). Gate placed BEFORE `Remove<MarkedDirtyBy>` in FireOnDependentReplicationComplete. Does NOT prune at non-driver entities (hydration is net-mode-agnostic). Original design.md note.
- [x] 2.6 Retire `5eda3ac8a` guards — **DONE.** Removed the `Has<FTag_Velocity_NeedsSetup>→NotReady` / `Has<FTag_Acceleration_NeedsSetup>→NotReady` blocks from CkVelocity_Fragment.cpp + CkAcceleration_Fragment.cpp Apply lambdas (§2.4+§2.5+late group supersede them). Left a why-comment.
- [ ] 2.4-OLD-NOTES (superseded by [x] above) `FTag_EntityScript_ConstructedThisFrame` defer — design ref kept:
   - Add `CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityScript_ConstructedThisFrame);` in `CkEntityScript_Fragment.h:23-28` (beside the lifecycle tags).
   - `FProcessor_EntityScript_FinishConstruction::ForEachEntity` (`CkEntityScript_Processor.cpp:~350-361`, where it `Add<FTag_EntityScript_BeginPlay>`) also `InHandle.AddOrGet<FTag_EntityScript_ConstructedThisFrame>();`.
   - BOTH dispatchers (`CkReplicatedFragmentContainer_Processor.cpp`) skip at the TOP of ForEachEntity: `if (InHandle.Has<ck::FTag_EntityScript_ConstructedThisFrame>()) { return; }` (leaves the pending tag → re-dispatches later).
   - **WHY it works (verified against the pump semantics — load-bearing):** feature Setups live in FGroup_Gameplay (BEFORE FGroup_Hydration). In the MAIN pass the entity is composed in FGroup_Gameplay_Script AFTER Setups already ran, so its Setup is still pending → applying now would be stomped by the pending Setup. Skipping in the main pass + clearing the tag after both dispatchers → in the PUMP, FGroup_Gameplay Setup drains FIRST, THEN FGroup_Hydration applies (post-Setup, no stomp). The clear-then-pump-reapply is INTENDED, not a bug.
   - **[P2-D1] decision — which dispatcher clears + ordering (plan left ambiguous; resolved by the pump analysis):** the clear MUST run AFTER both dispatchers' skips in the main pass, else the earlier-cleared tag lets the later dispatcher apply too early. So: give `FProcessor_Hydration_Dispatch` `using RunAfter = TDepList<FProcessor_ReplicatedFragments_Dispatch>;` (runs last, dormant so it clears exactly once per main pass — it doesn't pump), and ONLY the hydration dispatcher clears. Forward-correct for Phase 3B when hydration activates.
   - The clear is a `DoTick` override on `FProcessor_Hydration_Dispatch` (idiom: `CkIsmRenderer_Processor.cpp:23-32` — override `DoTick`, call the base's view-iterating DoTick, then `_TransientEntity.Clear<FTag_EntityScript_ConstructedThisFrame>();`). **ck_exp DoTick-override mechanics UNVERIFIED** — the base is `ck_exp::TProcessor<...>`; add `using Super = ck_exp::TProcessor<FProcessor_Hydration_Dispatch, FCk_Handle, ck::TReadWrite<FFragment_PendingHydration>, FTag_Hydration_PendingApply, CK_IGNORE_PENDING_KILL>;` (CkAttribute pattern, `CkAttribute_Processor.h:34`) and call `Super::DoTick(InDeltaT)`. CONFIRM ck_exp exposes a callable view-iterating `DoTick(TimeType)` before trusting this; if not, mirror how a CkAttribute composite drives its inner loop. The dispatcher's ForEachEntity is `const`; the DoTick override that clears must be non-const.
- [ ] 2.5 Fire-gating: `Get_HasUndrainedReplicatedFragments_IncludingDependents` (Net utils, beside `Get_IsReplicationCompleteAllDependents` `CkEntityReplicationDriver_Utils.cpp:381-405`) traversing the dependent set the counters at `CkEntityReplicationDriver_Fragment.cpp:377-408` maintain (self + dependents: `FTag_RepFragments_PendingApply` / queued removals / `FFragment_PendingHydration`); gate `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` (`CkEntityReplicationDriver_Processor.cpp:19-32`) — if true, return WITHOUT consuming the fire tag (retry next tick).
- [ ] 2.6 Retire `5eda3ac8a` `Has<FTag_*_NeedsSetup> → NotReady` guards in the Apply lambdas of `CkVelocity_Fragment.cpp` (the guard I READ at `:42-44`) + `CkAcceleration_Fragment.cpp`. Keep everything else. Verify `git show 5eda3ac8a --stat` remnants.
- [x] 2.7 Tests — **Test 1 DONE; Test 2 subsumed per [P2-D3].** `Ck.Snapshot.LoadGate.GatedSkipsKernelTicks` written (hermetic scheduler à la Test_Scheduler_PumpGating: two test processors, one default + one RunsDuringLoad; `Tick(Full)`→both, `Tick(LoadKernel)`→only kernel [the inverse assert: over-gating hangs a real load], `Tick(Full)`→gated resumes). CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_LoadGate_Scope.cpp`. Compile-verified (p2-compilecheck3.log). **[P2-D3]** the standalone `Velocity_ApplyAfterLateSetup` MP repro is NOT written: the late-setup stomp (replicated value arrives while NeedsSetup pending) can only be forced ROBUSTLY via a reload (client re-constructs with the saved override as initial replicated data) — a non-reload override stomps server-side at spawn, or is a change-rep after the client already constructed. That exact scenario IS covered, robustly + in-gate, by `Ck.Snapshot.Parity.Acceleration_MPReload` (post-reload the client's Setup recomputes Current from Params{0} and the replicated override{3,4,5} must win) + the three `Ck.Attribute.Net` pins. A flaky standalone repro is worse than robust existing coverage.
- [x] 2.8 Gate + commit — **DONE, GREEN.** `Ck.Snapshot` 49/49/0 (--discover-fresh; LoadGate + AccelerationParity + all Parity; NO over-gating hang → kernel list correct). `Net` 102/98/4: framework `Ck.*.Net` delta-zero incl. the 3 Attribute.Net pins (`Values_AppliedBefore_OnReplicationComplete`, `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`) all GREEN — the "real regression" filter (non-kiosk, non-StateMachine) was EMPTY; the 4 fails are the recorded kiosk env-red trio + the StateMachine.Net flake. `Ck.Attribute.Net`/`Ck.Physics.Net` covered by the `Net` substring run (all pins green, no physics-net red). Commits: CkF 91b96a177 / 8c4fbce7a / af9fad239, CkTests 4522c8e. Nothing pushed. (Split 3 CkF commits not 4 — the trait/group/defer sub-steps interleave within shared files; fire-gating + guard-retire live in independent files so they got their own commits.)

**Phase-2 CHECKPOINT (2026-07-11): 2.1–2.3 IMPLEMENTED + COMPILE-VERIFIED (editor build clean, 0 errors, log
`CkAuto/logs/p2-compilecheck2.log`) + uncommitted on disk; 2.4–2.8 remaining.** Compile-check caught + fixed a dropped
`namespace ck {` opening in `CkProcessorScheduler.h` (my enum-insert edit had swallowed it — now restored). Phase 2 is ATOMIC —
2.1–2.3 alone are NOT gate-safe (they move BOTH dispatchers from FGroup_Gameplay_Script → FGroup_Hydration, which
without 2.4's ConstructedThisFrame defer + 2.5's fire-gating can expose the late-setup stomp). DO NOT commit or gate
until 2.4–2.7 land. The tree's committed tip is Phase-1-green (`df06b8394`); the uncommitted delta is Phase-2 2.1–2.3
(files: `CkProcessorDescriptor.h`, `CkProcessorTraits.inl.h`, `CkProcessorGraph.h/.cpp`, `CkProcessorScheduler.h/.cpp`,
`CkEcsWorld_Subsystem.h/.cpp`, `CkEntityScript_Processor.h`, `CkEntityLifetime_Processor.h`,
`CkReplicatedFragmentContainer_Processor.h`, `CkPersistence_ReDrive_Processor.h`, `CkEntityReplicationDriver_Processor.h`,
`CkActorRespawn_Processor.h`, `CkProcessorGroups.h/.cpp`). `git -C Plugins/CkFoundation diff --stat` shows the exact set.

## Campaign-added tests (protected inventory — grows as phases land)

| Test name | Added in | File |
|---|---|---|
| Ck.Snapshot.Oracle.StructuralBaseline | Phase 0 | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_Oracle_StructuralBaseline.cpp` |
| Ck.Snapshot.Oracle.ProduceDiffBaseline | Phase 1 §1.6 | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_Oracle_ProduceDiffBaseline.cpp` |

## Decisions made by executors (anything the plan left as A-or-B, with which was taken and why)

- **[P0-D1] Homing dt==0 fix lives in `CkProjectile`, not `CkPhysics`.** PHASE_0 §0.3b cites
  `Source/CkPhysics/.../Homing/CkHoming_Processor.cpp`; the Homing feature actually lives at
  `Source/CkProjectile/Public/CkProjectile/Homing/`. Same fix, real file — a doc path typo, not an architecture
  change. Guard placed at the top of `FProcessor_Homing_Update::ForEachEntity` (covers the finite-diff divide at
  `:236` AND, by returning before `Compute_HomingAcceleration`, prevents ProNav's existing `InDeltaT > 0` ensure
  from firing during a settle pass — so no `CkHoming_ProNav.cpp` edit is needed, matching the plan's "one guard at
  the top covers the ProNav calls it makes").
- **[P0-D2] Oracle `_LabelPath` left empty (tier fence); label-keyed identity deferred to Phase 3B.** PHASE_0 §0.5
  specifies the label path via `UCk_Utils_GameplayLabel_UE`, but the oracle lives in **CkEcs** and that util is in
  **CkLabel**, which *depends on* CkEcs — so the reverse include edge is impossible (would break the tier direction
  the same step's fence insists on: "keep the oracle in CkEcs"). `FFragment_LifetimeOwner` IS in CkEcs, but without
  CkLabel there is no stable per-entity label text to build a path from. Decision: Tier-1 signatures use the
  fragment/tag set (fully implemented) + `_ScriptClassPath` (CkEcs-visible EntityScript fragment); `_LabelPath`
  stays empty. The Phase-0 harness fixture carries no labels, so this does not affect the gate. **Forward note for
  Phase 3B:** when label-keyed cross-rebuild identity actually matters, supply the label via a CkEcs-visible hook
  (registered label-provider callback) or perform the label step from a higher tier — do not add a CkEcs→CkLabel
  edge.
- **[P0-D3] Tier-1 lumps tags into `_FragmentTypeNames`; `_TagTypeNames` left empty** — explicitly permitted by
  §0.5 ("put everything in `_FragmentTypeNames` … Tier-1 only needs stable signatures, not taxonomy"). Unregistered
  storage types are keyed by their entt type-hash (hex) to avoid the non-null-terminated `string_view`→`FString`
  hazard; registered types use their clean `_DisplayName`.

- **[P1-D1] §1.1 include-surface CORRECTION.** The blessed ".inl.h at the bottom of CkNet_Utils.h" design FAILS —
  CkNet_Utils.h is UHT-reflected and UHT forbids any `#include` after its `.generated.h`. Fix: `RegisterLazyTyped<T>`
  body lives in `CkReplicatedFragmentContainer.inl.h` (no includes), and each migrated registrar `.cpp` includes
  `CkNet_Utils.h` then that `.inl.h`. The Attribute registrars get both via `CkAttribute_RestorePersistence.h`.
- **[P1-D2] Attribute Produce = EMPTY-SEED (option A), not the research recipe's value-emitting option B.** The old
  attribute `ReplicateOnRestore` itself empty-seeds the owner container + re-arms Current/Min/Max
  `MayRequireReplication` (it reads NO values); `FProcessor_Attribute_Replicate` refills. So a behavior-neutral
  Model-A migration is empty-seed (shared `ck::attribute_restore::Produce/SeedContainer` in
  `CkAttribute_RestorePersistence.h`), which also sidesteps the per-owner upsert-merge risk. AnimPlan uses the same
  empty-seed shape. **Consequence for 1.6/Phase 3A:** an empty-seed Produce emits an always-empty payload, so a
  Tier-2 "mutate attribute value → diff line" test would never register a change; the value-emitting Attribute
  Produce is a Phase-3A/save-path concern. The 1.6 `ProduceDiffBaseline` test should exercise a value-emitting
  feature (**Velocity** — `Produce` emits `FCk_RepData_Velocity{Get_CurrentVelocity()}`), not an attribute.
- **[P1-D3] 1.3 re-drive done-marker.** `FTag_Snapshot_JustRestored` is added at `CkSnapshot_Restore.cpp:243` and
  NEVER removed (persists). So the re-drive views on it, populates `FFragment_Persistence_ReDrivePending._Remaining`
  on first sight, drains per-tick, and LEAVES the emptied fragment as the done-marker (does not re-populate). Gate =
  each `SeedContainer`'s own driver/owner check returns `NotAdded`→retry; 5s/2s timeout → loud drop.
- **[P1-D4] 1.5 net dispatcher NOT refactored.** Extracted `ck::persistence_apply::ApplyOne` (resolve+Apply+timeout)
  used by the new dormant `FProcessor_Hydration_Dispatch`; left the tested `FProcessor_ReplicatedFragments_Dispatch`
  inline to avoid regressing the green Net/Parity gate for a dormant feature (a future cleanup can adopt ApplyOne).
  Hydration processor view needs `ck::TReadWrite<FFragment_PendingHydration>` (ck_exp::TProcessor static_assert).

- **[P1-D5] §1.6 oracle Tier-2 shape.** `Capture_Payloads(SnapshotRegistryType&, FCk_RegistryHandle, const TSet<uint32>*
  =nullptr)` mints per-entity handles via `FCk_Registry{InRegistryHandle}` + `ck::MakeHandle(FCk_Entity{e}, CkRegistry)`
  (the `FCk_Registry(FCk_RegistryHandle)` ctor at `CkRegistry.h:186` makes the spec's `FCk_RegistryHandle` param work
  directly). Keyed by the Tier-1 signature; the storage-sweep signature builder was EXTRACTED into a shared internal
  `BuildEntitySignatures` helper used by both Capture_Structural (behavior-neutral — StructuralBaseline stays green) and
  Capture_Payloads. `Diff_Payloads` = per-(sig,type) composite-ExportText comparison (`~`/`+`/`-` lines): a VALUE change
  is ONE line, not an add/remove pair. New registry method `Get_ProduceHandlerTypes()` = superset of
  `Get_ReDriveHandlerTypes()` (every handler with Produce, incl. capture-only). Per [P1-D2] the ProduceDiffBaseline test
  uses the REAL value-emitting **Velocity** handler (Add creates Current immediately; Request_OverrideVelocity is an
  immediate write — no tick needed in a bare FEcsWorld); mutate → exactly 1 diff line, unmutated → 0.

## Decisions — Fable-agent rulings (unattended-protocol consults)

- **[BI-1] (2026-07-11, §1.6) — kiosk-destruction `Net` reds are pre-existing/environmental, NOT the campaign.**
  Consulted a Fable agent when the `Net` gate diverged (3 BB `Bb_AutoTest_RentnetKiosk*` destruction tests red; baseline
  had them green). VERIFIED its verdict against code: the failing tests compose all entities
  `ECk_Replication::DoesNotReplicate` (`BB_AutoTest_RentnetKiosk_DamageToDestroy.as:39,46,53,59`) → the replicated-fragment
  registry / Produce / re-drive surface (all the campaign touched) is never consulted for them → mechanically severed
  from Phase-1-core AND §1.6. Independently confirmed §1.6 is inert by construction: 274+/66− diff wholly additive to the
  registry (+19/+9) + gated to `#if CK_WITH_FIDELITY_ORACLE` (oracle); `Get_ProduceHandlerTypes`/`Capture_Payloads`/
  `Diff_Payloads` have ZERO production callers. Failure shape = hits 2–3 miss fixed 0.4s ScheduleSettle windows under
  machine load (a sibling save-load session was active on this box); the kiosk Setup file records prior settle-timer
  races. Recorded the trio as a known env-red in the baseline. Safe to commit §1.6 + proceed to Phase 2.

- **[P2-D2] (2026-07-11, Phase 2 §2.5) — fire-gating aggregates over the LIFETIME-DEPENDENTS tree, not a driver list.**
  Consulted a Fable agent when PHASE_2's "traverse the dependent set the counters maintain" didn't map to code (the
  driver tracks dependents only by COUNT — `_NumSynced`/`_ExpectedNumberOfDependentReplicationDrivers`; no owner→dependents
  collection). VERIFIED its ruling against code: the traversable set IS `UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents`
  (`FFragment_LifetimeDependents`), populated on the client before the count bump, and it is the exact traversal
  `DoCount_ReplicationDriversIncludingDependents` (Utils.cpp:53-79) derives the expected count from — so at fire-tag-set
  time every synced dependent is reachable. Chose option (b) recurse-the-tree (mirror DoCount) over (a) self-only [leaves
  `OnDependentsReplicationComplete` firing in the §2.4 ConstructedThisFrame window — a real hole with consumers like
  CkEntityCollection_Processor.cpp:184] and (c) push-aggregation [new owner-side state, overkill]. Two-check predicate
  (`FTag_RepFragments_PendingApply` subsumes removals + `FFragment_PendingHydration`); no non-driver prune (hydration is
  net-mode-agnostic). Stall-bounded by the dispatcher 5s/2s timeout. Caveat (noted): cross-registry children are excluded
  from LifetimeDependents (they carry no driver on-wire — acceptable).

## Decisions — planner rulings

- **[P1-R1] (2026-07-11, planning session — resolves [B1]): option (c), refined to a 6/6 split.** Phase 1 migrates
  ONLY the clean six (Velocity, Acceleration, Attribute×5, TagSet, MontagePlayer, AnimPlan — 10/16 registrations);
  Team + Player join the DEFERRED set alongside Inventory×2, RenderTarget, 2dGridOccupancy (their unconditional
  `FTag_TeamID/PlayerID` re-derive is the same non-container-reconstitution class, just smaller — the research
  table's own "UNCONDITIONALLY, pre-driver-gate" note contradicts a driver-gated SeedContainer home). Deferred
  processors stay VERBATIM: their repair work is Model-A-only and Model B retires it structurally (items→recipes,
  grids→Construct, RT→Construct+Phase-4B re-author, Team/Player tags→normal Assign in hydration Apply); they go
  inert at 3B (v3 never stamps JustRestored) and are deleted in Phase 5. Options (a) slimming and (b) a
  `Reconstitute` hook are REJECTED: churn without end-state value / framework surface that Phase 3B makes dead.
  New participation rule (encoded in PHASE_1.md + PHASE_3A.md §3A.4): `SeedContainer` present ⇒ handler joins the
  Model-A re-drive; `Produce`-without-`SeedContainer` ⇒ capture/oracle-only (how the deferred six gain Produce at
  3A with zero double-seed risk). The research doc's §1.1 `.inl.h`-at-bottom-of-CkNet_Utils.h include decision is
  BLESSED — re-apply verbatim. PHASE_1/3A/3B/5 docs revised accordingly (same commit as this entry).

## Blockers

- **[B1] — RESOLVED 2026-07-11 by [P1-R1] above.** Original text kept below for the record.
- **[B1] (2026-07-11, Phase 1) — Plan's "12 ReplicateOnRestore = deletable container re-seeds" is false for 4
  features; where their non-container reconstitution goes is an unmade architecture decision.** REQUIRES a design
  ruling before Phase 1 can proceed (executor may not improvise architecture — PROMPT line 5).
  - **What diverges:** PHASE_1.md §1.2/§1.4 assume every `*_ReplicateOnRestore` processor only re-seeds the
    replication container from live state, so `Produce`/`SeedContainer` (§1.1) replaces them and §1.4 deletes them
    behavior-neutrally under Model A. Verified against code, that holds for 8 features but **Inventory Spatial,
    Inventory DataOnly, RenderTarget, and 2dGridOccupancy** restore processors ALSO do non-container reconstitution:
    child-entity re-replication (`Request_TryReplicateExisting`), grid re-stamp (`Request_PlaceItemOnGrid`) gated on
    CkGrid's `FProcessor_2dGridSystem_RestoreRecompose`, render-target re-create + repaint (`DoApplyBatch`), and an
    unconditional derived-fragment re-seed for local-only grids. The generic re-drive (§1.3) and Produce/SeedContainer
    cannot house this (cross-processor ordering; Produce-before-Seed circularity for Inventory; unset-Produce-skips-
    Seed for local grids; once-only repaint vs per-retry SeedContainer). Deleting these 4 per §1.4 breaks restore
    under Model A. HAND-VERIFIED: `CkInventory_Spatial_Processor.cpp:62-175`. Others agent-cited (consistent pattern).
  - **Decision needed (maintainer/CTO — do NOT pick unilaterally):** (a) keep slimmed restore processors for these 4
    doing only reconstitution + Produce for the payload; (b) add a `Reconstitute(Entity)` re-drive hook (unconditional,
    retry, separate from the container path); or (c) migrate only the 8 clean features in Phase 1 and DEFER the 4 to
    Phase 3/4 (Model B's Construct-rerun may moot most of it; interacts with CTO-addendum N1 + PHASE_4B RenderTarget).
  - **Full analysis + the 8 ready-to-implement recipes + the (reverted, re-usable) §1.1 design:**
    `PHASE_1_RESEARCH.md`. Once resolved, §1.1 re-applies verbatim; the 8 clean migrations follow the recipe table.
  - **Repo state:** clean at the gated-green Phase-0 boundary (CkFoundation `e5ffe028d`). No Phase-1 code landed;
    the §1.1 scaffolding was implemented then reverted (never build-verified) to keep the boundary clean.

## Session log

- 2026-07-11 — package authored (planning session, Fable). Branch `feature/save-load-improvements` @ `bc484d645`.
- 2026-07-11 — Phase 0 execution (Opus). Baselines captured @ `bbde1a9dd` (see table). Census re-derived
  (127/20/23). Decisions P0-D1..D3 recorded. Applied 3 dt==0 guards + `CK_WITH_FIDELITY_ORACLE` define + oracle
  Tier-1 (.h/.cpp) + harness test + spec §2 census reconciliation. **Gate GREEN, delta-zero:** Ck.Snapshot
  46→47 (new `Ck.Snapshot.Oracle.StructuralBaseline` PASSED, `--discover-fresh`), Ck.Attribute.Net 17/17/0,
  Net 102/101/1 (same pre-existing red). Build clean; no AngelScript errors naming campaign files;
  `rg CK_WITH_FIDELITY_ORACLE Source | wc -l` = 8 (≥3). Commits: CkF `68ba192dc`,`55521d493`,`<docs-this>`;
  CkTests `14d65ac` (on `dev`, unpushed — where the sibling snapshot tests live). Nothing pushed. Phase 0 DONE.
- 2026-07-11 — Phase 1 attempt (Opus, same session). Ran a 10-agent read-only census of all `*_ReplicateOnRestore`
  processors (recipes in PHASE_1_RESEARCH.md); designed + implemented §1.1 registry extension (Produce/SeedContainer/
  Transport/RegisterLazyTyped; include-surface decision made). **STOPPED on divergence [B1]** — 4 of the 12 restore
  processors do non-container reconstitution the plan's model can't house; where it goes is an unmade architecture
  decision (executor may not improvise). Reverted the unbuilt §1.1 scaffolding; tree back at gated-green Phase-0
  boundary. No Phase-1 code committed. Awaiting a design ruling on [B1] (see Blockers). Session ends here per the
  campaign's divergence rule.
- 2026-07-11 — Phase 1 RE-EXECUTION (Opus) after [B1] resolved by [P1-R1]. Implemented + COMMITTED the core:
  §1.1 registry contract, §1.3 generic re-drive, §1.2+§1.4 all SIX clean-feature migrations + deletions, §1.5
  dormant hydration queue. Two commits on `feature/save-load-improvements`: **d7956345a** (CkEcs framework),
  **4a6839afb** (6 migrations + deletions). **Gate GREEN:** Ck.Snapshot 47/47/0 delta-zero (all 11
  Parity_MPReload pass — proves the generic re-drive == the deleted per-feature processors), Net 102/102/0 (the
  baseline's lone red is a pre-existing flake, green this run). Executor decisions [P1-D1..D4] recorded above (esp.
  P1-D2: empty-seed Attribute Produce ⇒ 1.6 ProduceDiffBaseline must use a value-emitting feature like Velocity).
  Hit + fixed: UHT-forbids-include-after-generated.h (.inl.h moved to registrar .cpp), ck_exp::TProcessor needs
  TReadWrite on the hydration fragment. **REMAINING: §1.6** (oracle Tier-2 Capture_Payloads/Diff_Payloads +
  Ck.Snapshot.Oracle.ProduceDiffBaseline test) — the ONLY unfinished Phase-1 step. Nothing pushed. Handed off
  mid-phase (context full) with a continuation prompt.
- 2026-07-11 — **Phase 1 §1.6 execution + COMPLETE (Opus, unattended run).** Implemented oracle Tier-2:
  `Get_ProduceHandlerTypes()` (registry), `Capture_Payloads`/`Diff_Payloads` + shared `BuildEntitySignatures` extraction
  (oracle), `Ck.Snapshot.Oracle.ProduceDiffBaseline` test (Velocity value-emit). Decision [P1-D5]. **Gate GREEN
  (built against the integrated base — see below): Ck.Snapshot 48/48/0** (all 11 Parity + StructuralBaseline +
  ProduceDiffBaseline; `--discover-fresh`), framework `Ck.*.Net` delta-zero. Divergence handled via Fable consult
  [BI-1]: 3 BB kiosk-destruction `Net` reds are pre-existing/environmental (DoesNotReplicate → severed from campaign),
  NOT §1.6 (proven inert, zero prod callers). **Also discovered:** the branch was rebased at 16:55 (one-time) to fold in
  the object-pooling-core campaign (+3758 lines incl. CkEcs Scheduler/EntityScript) — see the baseline-section note;
  Phase-2 line refs shifted (re-locate by pattern). Commits: CkF 5d262f52a, CkTests b9e7f86.
  Nothing pushed. **Phase 1 DONE.** Proceeding to Phase 2.
- 2026-07-11 — **Phase 2 execution (Opus, same run) — 2.1–2.3 DONE + compile-verified; CHECKPOINT before 2.4.**
  Implemented the load-gate trait plumbing (2.1: `ECk_ProcessorLoadPolicy` mirroring PumpPolicy across descriptor/traits/
  graph/scheduler + `ECk_SchedulerTickScope{Full,LoadKernel}` Tick/DoPump scope + subsystem `Get_/Set_IsLoadGateActive` +
  lazy weak-ptr Actor→subsystem + `Request_PumpToQuiescence` scope param), the 12-processor kernel marking (2.2), and
  `FGroup_Hydration` + moving BOTH dispatchers into it with their `RunAfter FinishConstruction` deleted (2.3). Ran an
  editor compile-check: FOUND + FIXED a dropped `namespace ck {` in CkProcessorScheduler.h (enum-insert edit swallowed
  it); re-built CLEAN (0 errors, 529s). **STOPPED at the 2.1–2.3/2.4 boundary** — context depletion + Phase 2 is atomic
  high-blast scheduler core (2.4's ck_exp DoTick-override + dispatcher-ordering + 2.5 fire-gating want fresh context to
  get right; a half-done atomic scheduler phase is worse than a clean checkpoint). 2.1–2.3 code is UNCOMMITTED (atomic
  phase — not gate-safe alone) but compile-verified in the working tree; the sub-step tracker above carries the full
  design for 2.4–2.8 (esp. [P2-D1] + the ConstructedThisFrame pump/ordering reasoning). Continuation: implement 2.4–2.7,
  gate per §2.8, commit. Nothing pushed.
- 2026-07-11 — **Phase 2 COMPLETE (Opus continuation).** Implemented 2.4 (ConstructedThisFrame defer — verified the
  ck_exp DoTick-override pattern; [P2-D1]), 2.5 (fire-gating — Fable consult [P2-D2] resolved the "no traversable
  dependent set" divergence → recurse the lifetime-dependents tree), 2.6 (retire NeedsSetup guards), 2.7 Test 1
  (LoadGate.GatedSkipsKernelTicks; Test 2 subsumed per [P2-D3] — the late-setup stomp is only robustly forced via
  reload, already covered by AccelerationParity_MPReload + the Attribute.Net pins). Two compile-checks (the first caught
  a dropped `namespace ck {` in 2.1-2.3; final clean). **Gate GREEN:** Ck.Snapshot 49/49/0 (LoadGate + all Parity, no
  hang), Net 102/98/4 (framework delta-zero, 3 pins green, 4 fails = recorded kiosk env-red trio + StateMachine.Net
  flake). Commits CkF 91b96a177 / 8c4fbce7a / af9fad239, CkTests 4522c8e. Decisions [P2-D1..D3] + Fable [P2-D2]
  recorded. **Phase 2 DONE.** Nothing pushed. Next: Phase 3A.

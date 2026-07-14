# CkSnapshot v2 — Rebuild + Hydrate (save/load unified with replication)

**Status:** REVISED v2 (2026-07-11) — CTO review 2026-07-10 ruled **CHANGES REQUESTED on spec text, direction endorsed;
Phases 0–1 approved for plan authorship immediately** ([review](../reviews/2026-07-10-CkSnapshot-rebuild-hydrate-CTO-review.md)).
This revision closes the three sign-off conditions; the scoped re-review targets **§4.2, §4.4, §5** (+ their §6 rows) only.
**Author:** Claude (Fable 5), 2026-07-10; v2 revision 2026-07-11.

**v2 changelog (scoped to the review's sign-off conditions + non-blocking folds):**
- §4.2 rewritten — provenance taxonomy (EngineOwned / ConstructSpawned / RuntimeSpawned), subtractive reconciliation,
  loading-server duplicate-side-effect answer (blocker 1); audit-warning content (suggestion 3).
- §4.4 rewritten — race closure = gate the ReplicationComplete fire on pending-entry drain (blocker 2, reviewer's
  recommended mechanism); two-signal pins added to Phase 2's gate.
- §5 rewritten — oracle coverage taxonomy (structural / Produce-diff / gated deep-diff), `*_Params` in the capture
  set, residual blind spot named (blocker 3); Phase 5 row updated to retain registrations under a test-only gate.
- §4.1 — persisted-format discipline for Save-flagged payloads (suggestion 1). §6 — Phase 0 census re-derivation
  (suggestion 2), Phase 2 inverse gate assertion (suggestion 5), Phase 3 load-time baseline + `FInstancedStruct`
  5.7 smoke (suggestions 6, 7). §7 — forks replaced by the CTO's rulings. §8 — burden framing per review observation.
**Supersedes (if approved):** the registry-image restore model designed 2026-05-20 (CTO-reviewed in
[docs/reviews/2026-05-20-CkSnapshot-design-CTO-review.md](../reviews/2026-05-20-CkSnapshot-design-CTO-review.md), GREEN-LIT v2) and the
in-flight M2 reconstitution campaign (uncommitted working-tree changes as of this date).

Every load-bearing claim below is **confirmed** against code (file:line cited) unless explicitly marked *inferred*.
Six pillar claims were additionally adversarially verified by independent reviewers instructed to refute them; their
corrections are folded in and called out as **[Vn]**.

---

## 1. Verdict

The tech director's sketch — *build the entity as normal, gate processors while loading, load minimal per-fragment
data the way replication does, one Setup that runs the same for a fresh or loaded entity* — is architecturally right
for this codebase, and the machinery to implement it **already exists and is battle-tested: it is the replication
pipeline**. The recommendation is to converge save/load onto it in five staged phases, each shippable, ending with
the deletion of the registry-image restore model and its entire per-feature repair layer.

The one-sentence model: **loading a save = the server late-joining its own past session.** The save file plays the
authority; the loading world rebuilds entities through the front door (the same recipe replication already ships to
clients) and then overlays authoritative values through the same Apply handlers clients already run.

---

## 2. Why the current model leaks footguns into every feature (diagnosis)

The current model is **registry-image + repair**: capture opted-in fragments byte-exact
([CkSnapshot_Capture.cpp:26-118](../../Source/CkSnapshot/Public/CkSnapshot/Snapshot/CkSnapshot_Capture.cpp)), then on load wipe the registry and
deserialize the image back ([CkSnapshot_Restore.cpp:126-255](../../Source/CkSnapshot/Public/CkSnapshot/Snapshot/CkSnapshot_Restore.cpp)) —
**construction never re-runs**. That was a deliberate 2026-05 decision ("making Construct universally idempotent
would be a far bigger contract change" — CTO review, v1 blocker 3 resolution).

The structural consequence: restore bypasses every invariant that Add/Construct/Setup normally guarantees. Every
implicit rule of the form *"if fragment X exists, then Y also happened"* (an actor was spawned, a UObject rooted, a
transient sibling fragment added, a record connected, a replication container seeded) becomes a restore bug that some
feature author must discover and patch by hand. Measured surface today:

- **127** `CK_REGISTER_SNAPSHOTABLE` sites across 20 modules (61 in CkAttribute alone), each with the file-scope
  alias hoist and the Tier-A/B/C serialize choice. *(Census re-derived invocation-only 2026-07-11, Phase 0 §0.2:
  was "119 / 18 modules"; the 20 modules are CkAnimation, CkAttribute, CkDynamic, CkEcs, CkEcsExt,
  CkEntityCollection, CkEntityTag, CkGrid, CkInteraction, CkInventory, CkLabel, CkObjective, CkPhysics,
  CkRelationship, CkRenderTarget, CkSnapshot, CkSpatialQuery, CkStateMachine, CkTagSet, CkTimer.)*
- **12 features × ~16 registered hand-copied `*_ReplicateOnRestore`/`*_RestoreRedrive` processors** + 9 per-feature
  transient done-tags, all instances of one skeleton — and
  [CkSnapshot_RestoreMarker.h:17-19](../../Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_RestoreMarker.h) documents that **every future
  replicated feature must add another**.
- A forced RoundTrip/Transient classification on every holder/record family
  ([CkSnapshot_Policy.h](../../Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_Policy.h)) — plus the opt-out tag model
  (`CK_DEFINE_ECS_TAG_TRANSIENT`) whose misuse silently corrupts the *next* save.
- A growing set of restore special-cases inside feature code — four of them landed in a single day
  (`fa2b5ac9d` CkCamera adopt-or-add, ~6 hand-written blocks, `CkCamera_Utils.cpp:336-410`; `15434d8ef` CkEcsExt
  Transform `_Previous` re-seed, `CkTransform_Utils.cpp:97-108`; `5eda3ac8a` CkPhysics NeedsSetup rep-defer +
  restore-seed retry; `860ab0f2a` default-pawn suppression) — plus the earlier CkStateMachine RestoreRedrive
  (~220 lines, `CkStateMachine_Processor.cpp:1011-1235`).
- The reconstitution machinery itself: `ECk_ReconstitutionPhase {None, EarlyWindow, Full}` spawn suppression,
  `Get_IsSnapshotRespawnable` CDO opt-in, `OnPostWorldInitialization` early stamping, respawn quiescence frames —
  all of it exists to stop the **normal** spawn path from fighting the restored image.

This class of bug is unbounded: it grows with every feature, and it is invisible to the feature author (the Camera
author had to learn snapshot internals to write a camera). The maintainer's requirement — *"the original implementer
should NOT have to know about save load when designing the feature"* — is unsatisfiable under registry-image +
repair, because repair is definitionally per-feature.

---

## 3. The two candidate models

### Model A — keep registry-image, systematize the repair layer
Generalize the 12 restore processors into one framework re-drive, keep policies/tags/markers, keep reconstitution
gating. Cheaper short-term; the invariant-repair bug class remains unbounded; new features still must classify
fragments and may still need bespoke repair (Camera/Transform class of bug remains possible).

### Model B — rebuild + hydrate (recommended; the tech director's model)
- **Save** = per persistable entity: its **recipe** (EntityScript class + spawn params + lifetime/context topology +
  actor spawn intent) plus its **hydration payload** (minimal authoritative state — for replicated features this is
  byte-for-byte the existing `FCk_RepData_*`).
- **Load** = teardown → level reload (existing M2 machinery) → **rebuild entities through the normal spawn path**
  (drivers first, then Construct/Adds — identical ordering to a normal boot) with simulation processors gated →
  **hydrate** through the same handler registry replication uses → settle (kernel-scope zero-dt pump) → open the gate.
- Construction invariants hold **by construction** — the normal code built the world-side state, so the entire
  repair layer (respawn intent processing aside) has nothing left to repair.
- Replication to connected clients needs **zero** restore-specific code: rebuilt entities replicate exactly like
  freshly spawned ones. Verified for Velocity/FloatAttribute/TagSet/Team/Acceleration: the normal Add seeds the
  container and the normal triggers re-arm (e.g. `CkVelocity_Utils.cpp:41-44`, `CkFloatAttribute_Utils.cpp:87-107`,
  `CkTagSet_Utils.cpp:38-44`); the restore processors' own comments state Construct-abstention as their sole reason
  to exist (`CkVelocity_Processor.h:235-238`, `CkAttribute_ReplicateOnRestore.h:25-31`). **[V1]**

The precedent that makes Model B cheap here and expensive elsewhere: **this framework already ships a
recipe-replication path.** `_ReplicationData_EntityScript` carries class + spawn-params `FInstancedStruct` to
clients, who re-run Construct with them (`CkEntityReplicationDriver_Utils.cpp:336-347` capture;
`CkEntityReplicationDriver_Fragment.cpp:227-272` client rebuild; Construct is not authority-gated,
`CkEntityScript_Processor.cpp:170`). The non-EntityScript ConstructionScript path has an equivalent recipe
(`CkEntityReplicationDriver_Fragment.cpp:162-200`). **[V3]** Model B's save file is that recipe, written to disk.

**Recommendation: Model B**, staged so that Model A keeps working (and its tests stay green) until cutover.

---

## 4. Target architecture

### 4.1 One persistence contract per feature: `Produce` / `Apply`

Extend `FCk_ReplicatedFragmentHandlerRegistry` (rename at cutover: **persistence handler registry**) with an
authority-side counterpart to `Apply`:

```cpp
// Existing (client receive): Apply(Entity, New, Old) -> Applied | NotReady, optional Remove.
// New (authority emit):      Produce(Entity)         -> TOptional<FInstancedStruct>
```

- **Replication** uses `Produce` implicitly today (each feature's Replicate processor computes the payload); making
  it explicit lets the framework drive it.
- **Save** = for each persisted entity, run every registered `Produce` → (type, payload) list into the archive.
- **Load** = feed saved payloads into a local pending-hydration queue drained by the **same dispatcher** and the
  **same Apply handlers** as net receive (`FProcessor_ReplicatedFragments_Dispatch`,
  `CkReplicatedFragmentContainer_Processor.cpp`), with the same `NotReady` retry + loud 5s/2s timeout.
- Transport flags per handler: `Net`, `Save`, or both — a `DoesNotReplicate` server-side feature participates in
  save with the identical struct + handler shape, no driver required.
- The 21 features with existing handlers get save coverage **for free**. A new feature that replicates already wrote
  everything save/load needs. A non-replicated feature writes one payload struct + one handler — the same artifact
  it would write the day it becomes replicated.
- **Persisted-format discipline (locked in at Phase 1):** the moment a handler is Save-flagged, its payload struct is
  save-file surface — net payloads are ephemeral (both ends run the same build); save payloads cross builds. Doctrine:
  Save-flagged payloads serialize via **tagged-property (UPROPERTY-walked) serialization** — tolerant of field
  add/remove — never byte-exact memory images; renames go through CoreRedirects. This is strictly better than the
  current byte-exact fragment image (which tolerates nothing) and is adopted deliberately, not inherited.

Honest caveat **[V1]**: features whose payload is an instruction stream rather than a current value (RenderTarget's
authored batch ring — `CkRenderTarget_Processor.cpp:459-503`) hydrate by **re-authoring through their normal request
path** on the authority, not by value overlay. Same pipeline, feature-specific Apply body — exactly as their client
Apply is already feature-specific.

### 4.2 Recipe capture, provenance, and reconciliation (what replaces the fragment image)

*(Rewritten per CTO blocker 1: the late-join analogy breaks on a loading server because Construct replays WITH
authority — default grants would duplicate against their saved counterparts, and value overlay has no absence
semantics. The answer is a provenance taxonomy stamped at save time plus a framework reconciliation pass — no
per-feature suppression machinery.)*

**Every persisted entity carries exactly one provenance, stamped into its save entry:**

1. **EngineOwned** — entities of level-placed actors and engine/GameMode-spawned actors (the default pawn). **Never
   respawned from the save.** The level reload / engine flow re-creates them and their normal entity build runs; the
   save entry carries no recipe — only a stable rendezvous key (`FFragment_SaveKey` GUID for level actors;
   PlayerState/controller identity for pawns) plus hydration payloads. The loader **adopts** the engine-built entity
   by key and hydrates it. This retains the 2026-05 "Option A" for exactly the class it was right about, and it
   *replaces* the committed default-pawn suppression (`860ab0f2a`): a second pawn entity never exists, so there is
   nothing to suppress.
2. **ConstructSpawned** — entities whose lifetime owner had **not yet finished construction** when they were spawned.
   Classification is framework-central and mechanical: at spawn time, if the lifetime owner is still inside its
   construction window (pre-FinishConstruction — the spawn path can already see this), stamp a framework
   `FTag_ConstructSpawned` (queryable at capture; never a per-feature decision). Saved **without a recipe** — the
   owner's replayed Construct is their creator. The entry carries identity (owner's saved id + GameplayLabel) plus
   hydration payloads. The loader **adopts by identity** — a ConstructSpawned entry never spawns, so
   **construction-side duplicates are structurally impossible**, for the same reason a client never duplicates them:
   there, replication suppresses the grant and ships the child; here, the grant runs and the save ships no child.
3. **RuntimeSpawned** — everything else persistable: full recipe (EntityScript class path + spawn-params
   `FInstancedStruct` + lifetime/context topology + `FFragment_ActorSpawnIntent` where present), respawned by the
   loader, id-mapped. **[V5]**

**Reconciliation (the subtractive half — hydration must express absence).** After Construct replay and before
gate-open, one framework pass per persisted owner compares the rebuilt **labeled** ConstructSpawned children against
the saved child set:

- Rebuilt labeled child **absent from the saved set** → destroyed through the normal `Request_DestroyEntity`
  teardown. A starting sword the player lost stays lost. This is child-set-level absence; value-level absence stays
  on the handler contract's existing `Remove` path.
- Saved entry **with a rebuilt identity match** → adopt + hydrate (rule 2 above).
- Saved entry **with no rebuilt match** (a content patch removed the default grant) → orphan-hydration report entry,
  dropped **loudly**. Content wins; old saves degrade gracefully instead of resurrecting retired grants.
- **Unlabeled ConstructSpawned children are invisible to reconciliation in both directions** — never destroyed,
  never hydrated: save-transient exactly as ruled (CTO fork ruling 1). Label matching is trusted only where the
  record enforces uniqueness (`DisallowDuplicateNames`, `CkRecord_Utils.h:949-968`) — attributes, inventory
  containers; the contract lands on the line the code already draws.

**Construct contract (doctrine line, enforced in review):** Construct composes the entity and spawns its own
subtree — nothing else. Cross-entity or global authoritative mutations inside Construct are *already* latently
broken under client rebuild (they re-run wherever Construct re-runs) and are a documented anti-pattern under this
design; stateful world effects belong to game systems whose own entities persist them as Save-flagged payloads.
Entity-shaped side effects are exactly what the taxonomy + reconciliation covers.

**Disk-specific recipe constraints, verified [V3]:**

- `FCk_Handle` members inside spawn params **must** route through the existing snapshot remap
  (`FSnapshotContext::Snapshot_Handle`) — handle UPROPERTYs are Transient by contract (`CkHandle.h:318-338`); a
  plain archive silently yields invalid handles.
- Non-asset `UObject*`/archetype refs in params: **loud ensure at save time** (mirror the existing
  `IsNameStableForNetworking` ensure, `CkEntityReplicationDriver_Utils.cpp:300-308`); assets round-trip by path;
  ConstructionScript archetypes fall back to the class CDO when not an asset.
- (Modifiers stay save-transient; hydrated attribute final values bake through the existing synthetic-modifier
  Apply path — CTO fork ruling 2, with the double-count hazard documented in §7.)

**Audit warnings are actionable or they are relocated footguns** (review suggestion 3): a hydration payload
targeting an unlabeled child, an orphaned save entry, or a non-asset param ref must name the **owner entity, the
EntityScript class, and the child's fragment set/label** — a designer acts on the warning (name the timer, re-scope
the ref) without learning snapshot internals.

### 4.3 Processor gating: `LoadPolicy` trait + kernel

New trait following the exact `PumpPolicy` precedent (enum in `CkProcessorDescriptor.h`, one `if constexpr` in
`BuildDescriptor` at `CkProcessorTraits.inl.h:~216`, mirrored to the graph node, consumed by a precomputed
`_LoadPassOrder` subset in `FProcessorScheduler`):

```cpp
enum class ECk_ProcessorLoadPolicy : uint8 { GatedDuringLoad /*default*/, RunsDuringLoad };
```

- **Default = gated.** A feature author who writes nothing gets safe behavior — their processors simply do not run
  mid-load. This answers the "categorize processors like the snapshot policies" question: yes, but with a safe
  default so that only the **framework kernel** ever declares the trait. Feature code stays load-ignorant.
- **Kernel (framework-owned, explicitly marked `RunsDuringLoad`):** EntityScript pipeline (SpawnEntity_HandleRequests,
  ContinueConstruction, FinishConstruction, BeginPlay, Replicate), deferred-entity/lifetime + destruction pipeline,
  record/label plumbing, `FProcessor_ActorRespawn`, and the hydration dispatcher. Per-processor gating is necessary —
  group-level gating is impossible because Setup/HandleRequests/Update share groups (Timer: all three in
  `FGroup_Gameplay_TimeDelta`). **[V6]**
- Gate state lives on `UCk_EcsWorld_Subsystem_UE` (superseding `ECk_ReconstitutionPhase`), read in
  `FProcessorScheduler::Tick`. There is no existing runtime gate; this is a new, small, central mechanism.
- **Settle before open:** the load's final step pumps **kernel-only** to quiescence, then opens the gate; the first
  normal frame drains all accumulated `NeedsSetup`/request/recompute markers through the existing pump machinery.
  The settle must NOT be a full-set zero-dt pump: three processors misbehave at dt=0 today
  (`FProcessor_PredictedVelocity_Update` div-by-zero + `_PreviousDeltaTime=0` poison,
  `CkPredictedVelocity_Processor.cpp:40-48`; Homing NaN, `CkHoming_ProNav.cpp:37-65`,
  `CkHoming_Processor.cpp:236`; Substep one-shot signals, `CkSubstep_Processor.cpp:26-39`). Those three get dt==0
  guards regardless — the **existing pre-save** `Request_PumpToQuiescence` is full-set zero-dt and is exposed to the
  same hazard today. **[V6]**

### 4.4 Hydration ordering (the "one Setup" answer)

The director's three alternatives (teach Setup about loaded data / skip Setup / two Setups) all dissolve under one
global ordering rule: **the hydration dispatcher runs in a late group, after every feature group's Setup**, so a
hydrated value always lands on an already-set-up entity, and Setup never needs to know loads exist.

- Today's dispatcher runs early (`FGroup_Gameplay_Script`) and demonstrably races later Setups — that is the
  Velocity/Acceleration stomp the uncommitted `NeedsSetup → NotReady` patches work around (Setup seeds
  `_CurrentVelocity` from Params *after* the early apply, `CkVelocity_Processor.cpp:85-119`). Moving dispatch late
  (immediately before `FGroup_Replication`, preserving the applied-before-OnReplicationComplete contract) fixes the
  same bug class for **network** replication too, and the two uncommitted per-feature guards become unnecessary.
- **Residual same-frame race + its closure** *(rewritten per CTO blocker 2 — this is a contract decision, not an
  implementation detail)*. The DAG half is sound: every Setup-bearing group precedes `FGroup_Replication`, so a
  pre-Replication dispatch slot preserves apply-before-`OnReplicationComplete` in the common path. But pump passes
  run after the full main pass — for an entity composed in `FGroup_Gameplay_Script` whose feature Setup lives in an
  *earlier* group, `NeedsSetup` drains in the pump **after** even a late dispatch, and the stomp survives. Deferring
  those entries to the next tick, alone, would break the pinned contract: the ReplicationComplete fire-tag is added
  at construction completion (`CkEntityReplicationDriver_Fragment.cpp:290-295`) and fires in `FGroup_Replication`
  the same frame — `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete` would fail on the common
  newly-replicated-entity path. **Chosen mechanism (the reviewer's recommendation): gate the fire on drain** —
  `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` additionally requires the entity's pending-apply
  queue empty (no `FTag_RepFragments_PendingApply`, no queued removals). The common case still fires same-frame
  (dispatch precedes `FGroup_Replication`); a deferred apply *delays* the fire instead of violating the contract —
  "values applied before `OnReplicationComplete`" becomes true **by construction, uniformly**, rather than by
  group-order luck. Bounded delay: a `NotReady`-stuck entry holds the fire at most until the existing loud 5s/2s
  dispatcher timeout drops it, then the fire proceeds. This also *restores* a contract that `5eda3ac8a`
  (NotReady-while-NeedsSetup for Velocity/Acceleration) had already quietly weakened per-feature — those guards are
  retired in Phase 2, replaced by the one global rule.
- **No generic "setup settled" predicate is buildable from existing metadata** — refuted by counterexample:
  Goap Setup has no `MarkedDirtyBy`, CrowdAgent uses an inverted done-marker, Probe's marker hides inside an
  unregistered aggregate, and chained flows (IsmProxy → NeedsInstanceAdded, SM initial-state-entry) extend past the
  first marker (`CkGoap_Planner_Processor.cpp:206-208`, `CkCrowdAgent_DrawBody_Processor.h:25-42`,
  `CkProbe_Processor.cpp:57`, `CkIsmProxy_Processor.cpp:170`). The design therefore deliberately does NOT rely on
  one. **[V2]**

### 4.5 What load orchestration keeps and deletes

**Keeps** (from the M2 campaign — all of it remains correct): the frame-spanning `FTSTicker` load state machine,
EndPlay-driven teardown, OpenLevel/seamless-ServerTravel selection by client count, world-ready detection, the
SaveGame/header/manifest IO layer with byte-jump skip of unknown types, corrupt-stream abort, processor-graph rebuild
after world swap.

**Deletes at cutover** (Phase 5): raw fragment capture for feature fragments from the *shipping* path — the
registrations themselves move under the `CK_WITH_FIDELITY_ORACLE` test-only gate rather than being deleted (§5); the tag
section, TagRegistry, TagDriver, `CK_DEFINE_ECS_TAG_TRANSIENT`-for-snapshot semantics; `FSnapshotPolicy_*`
classifications; `FTag_Snapshot_JustRestored` + all 12 restore processors + 9 done-tags; `ECk_ReconstitutionPhase`,
`Get_IsSnapshotRespawnable`, the `OnPostWorldInitialization` EarlyWindow stamp, spawn suppression, respawn quiescence
frames; the Camera adopt-or-add blocks, the Transform `_Previous` re-seed, the physics rep-defer guards. The
suppression machinery is unnecessary because under Model B the normal spawn path **is** the creator — there is no
restored image for it to duplicate.

**Params coverage** **[V4]**: no `*_Params` replicates via containers (confirmed census, 23 RepData structs — all
state-shaped), and Params normally regenerate from the recipe. But **8 features mutate Params post-construction
without replicating** (2dGridCell tags, Goap/AStar budgets, Timer re-Add, Substep, WorldSpaceWidget config,
CameraLayer post-create config, Pmg text, MontagePlayer rebind). On a rebuilt **server** those mutations would
silently revert — worse than late-join staleness, which only affects clients. Each of the 8 must either add the
mutated data to its hydration payload or be explicitly declared out of save scope. This is a closed, enumerated list,
not an open-ended class.

---

## 5. Debuggability: the fidelity oracle

*(Rewritten per CTO blocker 3: the oracle's post-Phase-5 coverage mechanism must be stated, `*_Params` must be in
the capture set, and the coverage boundary must be enumerated honestly.)*

Model A's capture code gets a second life as a **test-only oracle**: capture the pre-save world, run
save → rebuild+hydrate, capture again, **diff** and report divergences per entity with labels/script class. Every
"feature X forgot coverage" bug becomes a red diff line in an autotest instead of a playtest mystery (CI precedent:
autotests already capture worlds — `CkSnapshot_Audit.cpp:48-62`). Coverage is a three-tier taxonomy:

1. **Structural diff — generic, all fragments, forever, zero per-feature cost.** Entity set, per-entity storage
   membership (entt storage iteration + type names — needs no serialize path), record/label topology, tag presence.
   Catches the missing-sibling / ghost-entity / duplicate-child classes (Transform `_Previous`, Camera collision,
   escapee entities) for **every** fragment a future author ever writes, including ones with no registration of any
   kind.
2. **Produce-diff — generic over declared state, forever.** Run every Save-flagged `Produce` on the pre-save world
   and on the post-load world; diff the payloads. Value-level fidelity for exactly the state the system claims to
   persist, at zero marginal cost per feature (the payload **is** the feature's declaration).
3. **Deep value diff — opt-in, test-only.** The per-fragment serialize registrations Phase 5 would have deleted are
   instead **retained under a test-only compile gate** (`#if CK_WITH_FIDELITY_ORACLE`, off in shipping; the
   registration macro compiles to nothing outside oracle builds). Kept from day one for the **8 Params-mutator
   features [V4]** — their `*_Params` fragments are in the oracle capture set from Phase 0, so the closed list has
   an enforcement instrument, not just a census — and available for any fragment under suspicion. This is a small,
   honest, non-behavioral per-fragment obligation (the "opt-in line, honestly" precedent from the 2026-05 review):
   it exists only where deep-diff assurance is wanted; it is not required for a feature to save correctly.

**Residual blind spot, named:** an *undeclared* value mutation on a fragment with no Tier-3 registration and no
`Produce` coverage is invisible to the oracle (Tier 1 still catches its entity-shaped consequences). Mitigations:
review rule — any `Request_*` that writes a `*_Params` fragment must name its save coverage in the PR; plus a cheap
CI heuristic (grep for non-const `_Params` fragment access outside construction paths) to flag new members of the
[V4] class as they appear.

---

## 6. Staged migration plan

Each phase is independently shippable; Model A tests stay green until Phase 5.

| Phase | Delivers | Gate (success criteria) |
|---|---|---|
| **0 — Baseline + oracle** | Record current snapshot-suite pass/fail baseline; **re-derive the census counts with invocation-only patterns** so they're reproducible (review suggestion 2); build the fidelity oracle (Tier 1 structural + Tier 3 gated deep-diff incl. the 8 mutators' `*_Params`) + harness autotest; fix the 3 dt==0 misbehavers (they expose today's pre-save pump) | Oracle diffs a save→restore round-trip today and reports the *known* gaps; suite delta = baseline |
| **1 — Unify handler registry** | `Produce` added to the handler contract (+ tagged-property serialization doctrine for Save-flagged payloads, §4.1); ONE framework re-drive processor replaces all 12 `*_ReplicateOnRestore` (still Model A, behavior-neutral); local pending-hydration queue beside the FastArray in the dispatcher; oracle Tier 2 (Produce-diff) lands with it | All existing snapshot + net tests green; 12 processors + 9 done-tags deleted; net A/B: restored values still reach clients |
| **2 — Load gate + late dispatch + fire-gating** | `ECk_ProcessorLoadPolicy` trait + scheduler `_LoadPassOrder`; dispatcher moved to the late group; **ReplicationComplete fire gated on pending-entry drain (§4.4)**; the `5eda3ac8a` per-feature NotReady guards retired | Net stomp repro (Setup-after-apply) passes without per-feature guards; **the two-signal lifecycle pins stay green** (`Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`, `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`); gated-load smoke: no non-kernel processor ticks during a synthetic load window **and kernel-listed processors DO tick** (over-gating = silent hang, review suggestion 5) |
| **3 — Recipe capture + rebuild** | Provenance-stamped save path (§4.2 taxonomy: EngineOwned key-rendezvous / ConstructSpawned adopt / RuntimeSpawned recipe); load path rebuilds through normal spawn (drivers before Adds — loud ensure on `NotAdded` during rebuild); **reconciliation pass (subtractive)**; hydration dispatch; settle; gate-open. Reconstitution suppression retired; FormatVersion hard break lands here (fork ruling 5) | Fidelity oracle on a representative world: zero unexplained diffs for recipe-covered entities; existing M2 e2e (position restore, actor respawn, travel) reproduced under Model B; **duplicate/absence tests: default-grant lost stays lost, no duplicate grants**; `FInstancedStruct` disk round-trip smoke on 5.7.4 (review suggestion 7); **record a load-time measurement as a tracked baseline** (not pass/fail, review suggestion 6) |
| **4 — Coverage sweep** | The 8 Params-mutators + RenderTarget re-author + MontagePlayer rebind + SM redrive-as-hydration; save-transient contracts documented (modifiers incl. the double-count hazard, unlabeled children); AS surface: script SaveGame-field hydration post-Construct/pre-BeginPlay + the AS smoke matrix from the 2026-05 review | Oracle report clean or every remaining diff line annotated as declared-transient |
| **5 — Decommission Model A** | Delete list from §4.5 — except the per-fragment capture registrations, which **move under the `CK_WITH_FIDELITY_ORACLE` test-only gate instead of deletion** (§5, blocker-3 resolution); docs/skills updated | Full plugin gate green; grep-zero on deleted symbols in shipping-reachable code; feature-author doc: "write Add/Setup/payload — never think about save/load" |

**Multiplayer note:** a server load with connected clients rides the existing seamless-travel path; clients rebuild
via ordinary replication of the rebuilt entities — Model B makes the client side *less* special than today, not more.

---

## 7. Risks and forks — RULED by CTO review, 2026-07-10

All five forks are settled ([review](../reviews/2026-07-10-CkSnapshot-rebuild-hydrate-CTO-review.md), "Rulings"):

1. **Unlabeled Construct-children: save-transient — ACCEPTED.** No persistent per-child IDs (a parallel identity
   system; SaveKey reborn at child granularity). Naming the timer is the designer fix; the §4.2 audit warning is the
   enforcement. Label-matching boundary verified at `CkRecord_Utils.h:949-975`.
2. **Modifiers: save-transient; final values bake via the synthetic-modifier Apply — ACCEPTED.** Documented
   **double-count hazard**: a game system that re-applies a persisted buff on load *on top of* a baked final value
   counts it twice — owning systems either re-author modifiers on hydrate (their own payload, through the request
   path) or accept the bake, **never both**. Revisit only on a concrete revocation-across-load requirement.
3. **Mid-load construction signals: ACCEPTED — identical to normal boot.** Do not queue construction signals. Noted
   as a win: Model B retires the old "signal bindings don't survive snapshot" contract class — BeginPlay genuinely
   re-runs, so boot-time binds are rebuilt by the same code that builds them at boot.
4. **Image capture: KEEP, test-only** — with the §5 coverage taxonomy as the condition (registrations retained under
   the oracle gate, not deleted).
5. **Format break: NOW, pre-ship** — at Phase 3, no migration tooling. The format already hard-breaks with no
   cross-compatibility (`CkSnapshot_Header.h:49-53`); cost is zero until real player saves exist.

Biggest remaining technical risk (named per protocol): the **§4.4 fire-gating mechanism** is designed but not
executed — it must hold the two-signal contract uniformly while closing the pump-phase stomp, and it interacts with
the dispatcher timeout. Phase 2's gate (stomp repro + the three lifecycle pins) exists precisely to pin it before
Phase 3 builds on it. Second: the §4.2 reconciliation pass touches entity teardown mid-load — its destroy path must
ride the normal destruction pipeline (which is kernel-listed) and is covered by Phase 3's duplicate/absence tests.

---

## 8. Direct answers to the brief

- **"Replication and save/load should be the same code"** — yes, via `Produce`/`Apply` on the existing handler
  registry; 21 features already have the Apply half; the census and dispatcher timing guarantees are verified.
- **"All processors gated, including Setup"** — gated by default via `LoadPolicy` (safe default = the footgun-free
  property); Setup stays gated and drains post-open; the framework kernel is the only code that ever opts in.
- **"One Setup that happily runs regardless"** — achieved by ordering (hydration dispatch after all Setup groups),
  not by teaching Setup anything. Setup code is untouched; the two uncommitted NeedsSetup workarounds become dead.
- **"Restitution enums for processors"** — the analogue of the fragment policies is `ECk_ProcessorLoadPolicy`, with
  the deliberate difference that the default is safe, so unlike `FSnapshotPolicy_*` it is not a forced per-feature
  choice.
- **"No footguns creeping into every feature"** — the CameraLayer-class bug is impossible under Model B (Construct
  builds the layers; nothing restores stale ones); the remaining per-feature obligation is exactly the one
  replication already imposes: *declare your authoritative runtime state*. Stated honestly (per review): the burden
  **converges to replication's declaration burden — it does not go to zero.** Server-only stateful features (GOAP
  state, aggro tables, cooldowns) owe a Save-flagged payload they never owed the net. The fidelity oracle turns any
  omission into a red test, not a playtest mystery.

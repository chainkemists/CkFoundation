# CkSnapshot

**Purpose:** Save / restore whole-world ECS state — the **v3 rebuild+hydrate** save system. A save captures a
per-entity construction recipe (classified by provenance) plus each feature's persisted payload; a load rebuilds
every entity from its recipe, then hydrates the payloads back onto the freshly-constructed entities. There is no
byte-image snapshot and no per-fragment registration macro — `CK_REGISTER_SNAPSHOTABLE` and the Model-A "fidelity
oracle" were deleted 2026-07-13. A feature persists by registering a `Produce`/`HydrationApply` handler on the
`CkEcs` replicated-fragment registry — the SAME registry the network path uses.

**Depends on:** `Core`, `CoreUObject`, `CoreOnline`, `Engine`, `DeveloperSettings`, `CkCore`, `CkEcs`, `CkEcsExt`,
`CkLabel`, `CkLog`, `CkThirdParty` (see `CkSnapshot.Build.cs`).
**Used by:** the game / superproject save-load layer (e.g. BusterBlock's `Bb_SnapshotRestore.*`). No CkFoundation
module depends on CkSnapshot — features participate through the `CkEcs` handler registry, not a build dependency
on this module.

---

## How it works

- **Save** (`Snapshot/CkSnapshot_CaptureV3.*`): sweep the registry, classify every entity by **provenance**,
  capture its construction recipe + the payload of every save-participating handler —
  `FCk_PersistenceHandlerRegistry::Get_SaveHandlerTypes()` returns exactly the handlers that pair a
  `Produce` (emitter) with a `HydrationApply` (applier), sorted by type path for deterministic files.
- **Load** (`Subsystem/CkSnapshot_Subsystem.cpp` `DoHydrate_Enqueue`): rebuild entities from their recipes, map
  saved-id → live handle, then queue each payload on the target entity's `FFragment_PendingHydration`.
  `FProcessor_Hydration_Dispatch` drains the queue by calling the handler's `HydrationApply` (authority-side),
  retrying while it returns `NotReady`.

The handler registry, the `NetApply`/`HydrationApply`/`NotReady` contract, and the deferred-dispatch machinery are
owned by `CkEcs` (`CkEcs/Persistence/CkPersistenceHandlerRegistry.h` + the net + hydration dispatchers) — read
[`../CkEcs/Claude.md`](../CkEcs/Claude.md) first.

---

## Authoring a persistence handler (the load-bearing recipe)

To make a feature survive save/load, add a handler alongside its replication wiring. Copy the smallest matching
exemplar (table below) and mimic it.

### 1. Payload struct (`_Fragment_Data.h`)

- Wire **and** save → reuse the feature's replicated `FCk_RepData_<Feature>` (Team, TagSet, attributes, inventory).
- Save-only (never on the wire) → `FCk_SaveData_<Feature>` (EntityScript fields: `FCk_SaveData_EntityScriptFields`).

House style — private `_Members`, `CK_PROPERTY`, `CK_GENERATED_BODY` (see root [`../../CLAUDE.md`](../../CLAUDE.md)).

### 2. Registrar (`_Fragment.cpp`)

Register inside a **named** registrar struct with a module-prefixed instance name — unity-build-safe, no anonymous
namespace, no file-local `static` helper (Team: `FTeamRepHandlerRegistrar` / `GTeamRepHandlerRegistrar`;
SaveFields: filename-derived namespace + `GCkEntityScriptSaveFieldsRegistrar`). Include
`CkEcs/Persistence/CkPersistenceHandlerRegistry.h` + `.inl.h` (the latter carries the templated bodies).

**Prefer a named participation shape** over hand-building an `FHandler` — the shape name states the transport the
author chose, and each shape takes a **designated-init args struct** so every lambda is LABELED at the call site
(`.Produce =`, `.NetApply =`, `.HydrationApply =`) instead of positional. Required slots are **compile-enforced**:
the args struct's required fields are non-default-constructible wrappers, so an omitted `.Produce`/`.HydrationApply`/
`.NetApply` does not compile (the `Produce`-without-`HydrationApply` misconfig is uncompilable, not merely ensured).
`.NetRemove` is optional (defaults to empty). Each is `template <typename T_RepData>` and resolves the type lazily
via `T_RepData::StaticStruct()`, so it is static-init-safe. Write designators in declaration order:

| Shape | Transports | Args struct fields (declaration order) |
|---|---|---|
| `Register_NetOnly<T>` | wire only, never saved | `.NetApply` (req), `.NetRemove` (opt) |
| `Register_SaveOnly<T>` | save only, never on the wire | `.Produce` (req), `.HydrationApply` (req) |
| `Register_NetAndSave_SharedApply<T>` | both; one authority-safe applier | `.Produce` (req), `.SharedApply` (req), `.NetRemove` (opt) |
| `Register_NetAndSave_SplitApply<T>` | both; distinct net vs load appliers | `.Produce` (req), `.NetApply` (req), `.HydrationApply` (req), `.NetRemove` (opt) |

```cpp
FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_TagSet>({
    .Produce        = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct> { ... },
    .NetApply       = NetApplyFn,        // client-coupled net receive
    .HydrationApply = HydrationApplyFn,  // authority-side load
});
```

They forward to `RegisterLazyTyped<T>`, which fills the four `FHandler` slots
(`CkEcs/Persistence/CkPersistenceHandlerRegistry.h`):

| Slot | When | Rule |
|---|---|---|
| `Produce` | save capture | Mirror the feature's Replicate-processor live-state build. `return {}` (UNSET) = feature absent on this entity; a SET-but-empty payload = seed-empty (meaningful). **READ-ONLY — never mutate the entity.** Presence of `Produce` *is* save participation — there is no separate opt-in flag. |
| `HydrationApply` | authority-side load | REQUIRED whenever `Produce` is set — `Register_SaveOnly`/`_SharedApply`/`_SplitApply` all take it by-signature (a `Produce` without one is uncompilable), and `Get_SaveHandlerTypes` excludes any leftover `Produce`-without-`HydrationApply` (fails loud, never silently corrupts the save). |
| `NetApply` / `NetRemove` | net receive | **Absent on save-only handlers** — the type never rides a replicated container (`Register_SaveOnly` leaves them unset). |

Choosing between the two net-and-save shapes: if the net `NetApply` is authority-safe (no ClientOnly sync processor
in its loop), use `Register_NetAndSave_SharedApply` — one lambda serves both `NetApply` and `HydrationApply` (Team,
MontagePlayer, Velocity/Acceleration, Player all do). When it isn't — e.g. TagSet's `NetApply` only stamps a fragment
that a **ClientOnly** SyncReplication processor drains, so on the loading authority it would never restore — use
`Register_NetAndSave_SplitApply` with a distinct authority-side `HydrationApply` (TagSet: `Set_Tags` directly).

### 3. NotReady-before-any-mutation (non-negotiable)

`HydrationApply` is retried every load-kernel tick until it returns `Applied`. **Every `return NotReady` must
precede the first write** — a mutate-then-`NotReady` retry stacks state. The canonical trap is the attribute
family: `ApplyReplicated*Entry`'s `Add_Revocable` creates a **new** modifier per call, so a second pass would stack
a second replication modifier (see the rationale at
`CkAttribute/Public/CkAttribute/CkAttribute_RestorePersistence.h:68-71`). Put the composition guard
(`if (NOT ...Has(Entity)) { return NotReady; }`) first, before any `Set_`/`Request_`.

### 4. Re-arm the Replicate pass

Authority hydration writes values directly, but must leave the feature's Replicate pass **armed** so post-load
clients converge. Three legitimate idioms:

- **Implicit** via a deferred `Request_*` that already re-arms — Grid Occupancy's `Request_AddPlacement` re-arms
  `MayRequireReplication` (`CkGrid/.../2dGridSystem/Occupancy/Ck2dGridOccupancy_Fragment.cpp`).
- **Explicit utility call** — Inventory's `HydrationApply` calls
  `UCk_Utils_Inventory_UE::Request_TryReplicateInventory` (`CkInventory/.../Spatial/CkInventory_Spatial_Fragment.cpp`).
- **Raw tag add** — TagSet does `Entity.AddOrGet<ck::FTag_TagSet_MayRequireReplication>()`
  (`CkTagSet/CkTagSet_Fragment.cpp`).

An **unreplicated** feature re-arms nothing — e.g. a Timer persisted this way has no Replicate pass to drive.

### 5. Lazily-composed, data-defined features (reconstitute-by-request)

Some features have no structural composition step: the fragment exists iff it holds data — composed lazily by the public `Add`, auto-removed at zero (EntityTag's `FFragment_EntityTag_Current`). For these, the §3 `Has<> → NotReady` gate never opens on a v3 load (Construct does not re-compose them) and the payload would drop at the hydration timeout. The sanctioned shape is **reconstitute via the feature's own public deferred requests** from `HydrationApply` (precedent: Grid Occupancy's `Request_AddPlacement` re-drive). ALL of the following must hold:

1. **Data IS existence** — composed by `Add`, removed at zero; no composed-but-empty state exists.
2. **HydrationApply-only** — never assign a reconstituting lambda to the net `NetApply` slot (use `Register_SaveOnly`, or a `_SplitApply` whose `NetApply` does NOT reconstitute). On clients it races construct-time composition — the exact race anti-pattern #1 exists to prevent.
3. **REPLACE must ride the feature's request FIFO, never a read-live-then-clear.** Construct/BeginPlay run during the gated rebuild (`RunsDuringLoad`) and may seed the same feature through the same deferred requests, but feature request-drain processors are `GatedDuringLoad` — at `HydrationApply` time those seeds are enqueued and INVISIBLE to any `Has<>`/`Get_` read. Post-construction is not post-construction-effects. Enqueue a single composite restore-set request: FIFO order guarantees it lands after the seeds regardless of pump ordering.
4. **Idempotent under double-apply** — re-applying the payload yields the saved set, not a stacked one.
5. **Absence is ambiguous** — `Produce` returning UNSET cannot distinguish "never had data" from "all removed pre-save"; construct-seeded defaults therefore resurrect when the saved set is empty. Document and pin this.

Restored values are readable at `OnLoadComplete` (the settle phase pumps to quiescence first) — pin that, plus exact counts across TWO save/load cycles, in the feature's round-trip test.

### Exemplars by shape

| Shape | File |
|---|---|
| Simplest — one shared lambda for `Apply` + `HydrationApply` | `CkRelationship/Public/CkRelationship/Team/CkTeam_Fragment.cpp` |
| Direct authority write + re-arm (distinct `HydrationApply`) | `CkTagSet/Public/CkTagSet/CkTagSet_Fragment.cpp` |
| Record re-drive + all-or-nothing `NotReady` retry | `CkInventory/Public/CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.cpp` |
| Shared template across a family | `CkAttribute/Public/CkAttribute/CkAttribute_RestorePersistence.h` |
| Save-only (`Register_SaveOnly`, no `NetApply`) | `CkEcs/Public/CkEcs/EntityScript/CkEntityScript_SaveFields.cpp` |

---

## Provenance + skip/orphan diagnostics

Every saved entity carries one of four provenances (`ECk_Snapshot_V3_Provenance`, `SaveGame/CkSnapshot_Header.h`),
which drives how it is re-created on load:

| Provenance | Meaning |
|---|---|
| `EngineOwned` | boot infra / level-owned rendezvous (keyed by save-key or player id) |
| `ConstructSpawned` | spawned + labeled by a construction script (keyed by label) |
| `RuntimeSpawned` | loader-owned runtime entity — a script class, or an explicitly snapshot-respawnable actor bridge |
| `DefinitionBuilt` | rebuilt from a captured construction recipe |

A saved entity that never maps to a live handle **and** wasn't deliberately skipped (boot-infra / unloadable) is
an **orphan** — its payloads drop. `DoHydrate_Enqueue` (`Subsystem/CkSnapshot_Subsystem.cpp`) emits one
`v3 load ORPHAN: saved-id [..] provenance [..] identity [..] owner [..] reason [..]` Warning per orphan and records
a `FCk_Snapshot_OrphanRecord` in `FCk_Snapshot_LoadReport::_Orphans` (`Snapshot/CkSnapshot_LoadReport.h`, added
Phase 0). The reason bucket is one of: `owner-orphaned` (cascade), `owner-mapped-label-miss` (content/label drift),
`savekey-miss`, `player-miss`, `bridge-never-linked` (actor spawned, bridge never linked), `unresolved-other`.
These are **diagnostics only** — the loader does not act on them.

A saved entity the loader deliberately does NOT rebuild is a **skip**. `DoRecord_Skip` is the only writer of
`_SkippedIds`, so every skip carries a `FCk_Snapshot_SkipRecord` (same shape as the orphan record) in
`FCk_Snapshot_LoadReport::_Skips`, with an `ECk_Snapshot_SkipReason` naming the site that took it:
`ClassUnloadable`, `SpawnFailed`, `NonPersistedOwnerNotRespawnable`, `NoOwnerRecipe`, `OwnerNotPersisted`,
`NoLoadableSteps`, `BuildFailed`. Every site also logs — the `NonPersistedOwnerNotRespawnable` one emits
`v3 load SKIP: saved-id [..] provenance [..] identity [..] owner [..] reason [..]`, the rest an Error/Warning
naming the failure. Skips are not necessarily losses: `NonPersistedOwnerNotRespawnable` is the *expected* fate of
boot infra the fresh world re-creates itself, while `BuildFailed` is real data loss. This module does not judge
which is which.

**The report accounts for 100% of the save.** Entities partition into restored + skipped + orphaned; payloads into
enqueued + on-skipped + on-orphaned + on-unresolved-owner + dropped (a failed deserialize). `Get_IsAccountingClosed`
asserts both sums, `DoHydrate_Enqueue` ensures on it, and the summary Display line prints every bucket. A payload
whose owner id is absent from the entity table, or whose mapped handle went invalid, is the `on-unresolved-owner`
bucket — it exists so nothing has to be inferred by subtraction.

`SaveKey` is stable identity, not provenance. A SaveKey-only level actor remains `EngineOwned` and must already
exist in the fresh world. A bridged entity carrying `FFragment_ActorSpawnIntent` is explicitly snapshot-respawnable,
so it is `RuntimeSpawned` even when keyed; capture retains the key and load republishes it after actor-first rebuild.

---

## Capture classification rules

`Run_CaptureV3_Registry` (`Snapshot/CkSnapshot_CaptureV3.cpp`) walks every entity carrying ≥1 fragment/tag and
classifies it in this exact branch order. The rule numbers are the module's shared vocabulary — other files and this
doc cite them; the code itself is now unannotated, so **this table is the definition**:

| Rule | Test | Outcome |
|---|---|---|
| 1 | `IsMarkedForDestruction` (any `FTag_DestroyEntity_*`) | skip |
| 1.5 | `FTag_Snapshot_SaveTransient` | skip + count; derived state the owner's construction/re-drive recreates on load. A payload on one is an AUDIT Warning (it will be dropped) |
| 2 | `FFragment_SaveKey`, or a player pawn/controller/state rendezvous (world path only) | `EngineOwned` |
| 3 | `FTag_ConstructSpawned` **with a real (named) label** | `ConstructSpawned`. Unlabeled ⇒ save-transient, skipped + counted; a payload on one is an AUDIT Warning |
| 4 | `FFragment_SpawnRecipe` (retained EntityScript spawn recipe) | `RuntimeSpawned` |
| 4.5 | `FFragment_BuildRecipe` (built via `Request_BuildAndReplicate`) | `DefinitionBuilt` |
| 5 | none of the above — anonymous scratch | skip + count |

The transient entity is resolved up front and never persisted (bookkeeping, not world state). Truly empty entities
never even appear as candidates; they would fall to rule 5 anyway. Classified entries are then sorted by lifetime
depth (ties by saved-id, for determinism) so **owners precede dependents** in the entity table — the load-side adopt
and owner-mapping passes rely on that ordering.

---

## The v3 load machine

`DoTick_Load` drives `TearingDown → AwaitingWorld → Rebuilding → Hydrating → Settling`. Rationale that used to live
as comments in `Subsystem/CkSnapshot_Subsystem.cpp`:

- **Hydrating is ATOMIC.** One ticker callback enqueues payloads, queues the reconcile-destroys, AND opens the gate,
  so no gated world-tick ever sees pending payloads; hydration can only drain in post-gate FULL passes (Setup then
  hydration, no stomp). `Settling` then lets the parked destroys finish.
- **Ownership restore runs before any payload.** Rebuild APIs establish a *valid temporary* ownership chain so
  Construct can run, but not necessarily the saved one: RuntimeSpawned scripts can override ContextOwner after spawn,
  and DefinitionBuilt items are rebuilt under a driver-bearing context owner (historically they waited for a feature
  payload — Inventory — to transfer LifetimeOwner later). `DoRestore_SavedOwnership` restores every resolvable hard
  link once rebuild mapping has settled and before hydration; endpoints absent from `_SavedIdMap` keep their
  rebuild-time relationship.
- **The saved world transform rides the entity table, not a `Produce` payload.** A payload handler would race
  `FProcessor_Transform_SyncFromActor`'s per-tick stomp on actor-backed entities. `DoApply_SavedTransforms` runs once
  from `DoHydrate_Enqueue`, before payloads are enqueued; its deferred Transform requests / `SetActorTransform` calls
  land in the load-kernel settle pumps. Actor-backed entities are driven through the ACTOR only.
- **`FTag_Snapshot_JustRestored` is stamped on every mapped entity before the gate opens.** Game-side rebind
  processors (BusterBlock's `Bb_SnapshotRestore` fleet) key off it to re-resolve handles their persisted fragments
  carry. The Model-A purge deleted the old stamp site and silently killed every consumer; v3 restores the semantic.
- **Orphan accounting is per-entry, not a subtraction.** It used to be a bare `N - mapped - skipped` count; the walk
  enumerates the identical set (`_SavedIdMap` / `_SkippedIds` are disjoint subsets of the entity table) but emits one
  Warning + one report record per orphan, so a lossy load is self-explaining.
- **Reconcile keeps two classes of child it would otherwise destroy.** (a) `FTag_Snapshot_SaveTransient` children —
  never captured as rows, so "absent from the save" is their NORMAL state; without the skip, reconcile destroyed the
  loaded pawn's live intent/camera/movement attribute children every load (the ConstructSpawned stamp is
  timing-dependent — possession-driven composition lands inside the construct window only under the load gate's
  stretched construction, so the loaded world stamps children the save world never captured). (b) payload-bearing
  children — feature STATE, not grants; destroying them dropped live state the save cannot express (the
  QuickUse-containers-destroyed-on-load incident, 2026-07-14). Leaving them keeps boot defaults, which is strictly
  better.
- **Settling waits on hydration, not just frames.** It previously finished on a bare frame countdown, so
  `OnLoadComplete` could fire with hydration still in flight. The frame cap is now a LOUD abort backstop — reaching it
  means some payloads never applied.

### Restore invariants

`Verify_AllStoredHandlesResolve` (`Snapshot/CkSnapshot_RestoreInvariants.*`) reports every structurally-inconsistent
stored handle in a restored registry: a dangling HARD ref (`LifetimeOwner` / `ContextOwner`), or a live
`LifetimeDependent` whose own `LifetimeOwner` names someone else (aliasing / registry-rehome corruption). Unset and
tombstone handles are skipped, and an empty result means the backbone is consistent. The dependents leg asserts
**back-pointer consistency, not resolvability**: `FFragment_LifetimeDependents` is a lazily-pruned WEAK-ref list, so
an entry pointing at a destroyed entity is by design and is not reported.

---

## Anti-patterns

1. **Don't compose the feature from inside `HydrationApply`.** Composition belongs to construction (the recipe
   rebuild); hydration only writes values onto an already-composed entity. Return `NotReady` until composed.
   **Exception:** lazily-composed data-defined features — see §5.
2. **Don't mutate in `Produce`.** It is READ-ONLY by contract — emit a payload, change nothing.
3. **Don't set `Produce` without `HydrationApply`** — it fails loud at registration and the type is dropped from
   the save set.
4. **Don't look for `CK_REGISTER_SNAPSHOTABLE`, a "fidelity oracle", or a byte-image snapshot** — all deleted with
   Model A (2026-07-13). Persistence is the `Produce`/`HydrationApply` handler and nothing else.

---

## See also

- [`../CkEcs/Claude.md`](../CkEcs/Claude.md) — the replicated-fragment registry
  (`FCk_PersistenceHandlerRegistry`), the `NetApply`/`HydrationApply`/`NotReady` contract, and the two-signal client lifecycle
  this reuses.
- Root [`../../CLAUDE.md`](../../CLAUDE.md) — code style, macros, naming, and the `CK_ENSURE_IF_NOT` /
  no-silent-fallback non-negotiables (not restated here).
- `docs/campaigns/saveload-rebuild-hydrate/` and `docs/campaigns/saveload-v3-parity/` — the design + parity
  campaigns (spec §4.2 rebuild+hydrate, §4B.3 EntityScript SaveGame fields).

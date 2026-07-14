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
  `FCk_ReplicatedFragmentHandlerRegistry::Get_SaveHandlerTypes()` returns exactly the handlers that pair a
  `Produce` (emitter) with a `HydrationApply` (applier), sorted by type path for deterministic files.
- **Load** (`Subsystem/CkSnapshot_Subsystem.cpp` `DoHydrate_Enqueue`): rebuild entities from their recipes, map
  saved-id → live handle, then queue each payload on the target entity's `FFragment_PendingHydration`.
  `FProcessor_Hydration_Dispatch` drains the queue by calling the handler's `HydrationApply` (authority-side),
  retrying while it returns `NotReady`.

The handler registry, the `Apply`/`NotReady` contract, and the deferred-dispatch machinery are owned by `CkEcs`
(`CkEcs/Net/ReplicatedFragmentContainer/`) — read [`../CkEcs/Claude.md`](../CkEcs/Claude.md) first.

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
SaveFields: filename-derived namespace + `GCkEntityScriptSaveFieldsRegistrar`). Call `RegisterLazyTyped<T>` — it
resolves the type lazily via `T::StaticStruct()`, so it is safe during static init. Slots on `FHandler`
(`CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h`):

| Slot | When | Rule |
|---|---|---|
| `Produce` | save capture | Mirror the feature's Replicate-processor live-state build. `return {}` (UNSET) = feature absent on this entity; a SET-but-empty payload = seed-empty (meaningful). **READ-ONLY — never mutate the entity.** Presence of `Produce` *is* save participation — there is no separate opt-in flag. |
| `HydrationApply` | authority-side load | REQUIRED whenever `Produce` is set — a registration-time `CK_ENSURE_IF_NOT` fires otherwise, and `Get_SaveHandlerTypes` excludes a `Produce`-without-`HydrationApply` (fails loud, never silently corrupts the save). |
| `Apply` / `Remove` | net receive | **Absent on save-only handlers** — the type never rides a replicated container. |

If the net `Apply` is authority-safe (no ClientOnly sync processor in its loop), assign the **same lambda** to both
`Apply` and `HydrationApply` (Team, MontagePlayer, Velocity/Acceleration, Player all do). When it isn't — e.g.
TagSet's `Apply` only stamps a fragment that a **ClientOnly** SyncReplication processor drains, so on the loading
authority it would never restore — write a distinct authority-side `HydrationApply` (TagSet: `Set_Tags` directly).

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
2. **HydrationApply-only** — never assign a reconstituting lambda to the net `Apply` slot. On clients it races construct-time composition — the exact race anti-pattern #1 exists to prevent.
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
| Save-only (no `Apply`) | `CkEcs/Public/CkEcs/EntityScript/CkEntityScript_SaveFields.cpp` |

---

## Provenance + orphan diagnostics

Every saved entity carries one of four provenances (`ECk_Snapshot_V3_Provenance`, `SaveGame/CkSnapshot_Header.h`),
which drives how it is re-created on load:

| Provenance | Meaning |
|---|---|
| `EngineOwned` | boot infra / player rendezvous (keyed by save-key or player id) |
| `ConstructSpawned` | spawned + labeled by a construction script (keyed by label) |
| `RuntimeSpawned` | spawned at runtime — a script class, or an actor bridge |
| `DefinitionBuilt` | rebuilt from a captured construction recipe |

A saved entity that never maps to a live handle **and** wasn't deliberately skipped (boot-infra / unloadable) is
an **orphan** — its payloads drop. `DoHydrate_Enqueue` (`Subsystem/CkSnapshot_Subsystem.cpp`) emits one
`v3 load ORPHAN: saved-id [..] provenance [..] identity [..] owner [..] reason [..]` Warning per orphan and records
a `FCk_Snapshot_OrphanRecord` in `FCk_Snapshot_LoadReport::_Orphans` (`Snapshot/CkSnapshot_LoadReport.h`, added
Phase 0). The reason bucket is one of: `owner-orphaned` (cascade), `owner-mapped-label-miss` (content/label drift),
`savekey-miss`, `player-miss`, `bridge-never-linked` (actor spawned, bridge never linked), `unresolved-other`.
These are **diagnostics only** — the loader does not act on them.

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
  (`FCk_ReplicatedFragmentHandlerRegistry`), the `Apply`/`NotReady` contract, and the two-signal client lifecycle
  this reuses.
- Root [`../../CLAUDE.md`](../../CLAUDE.md) — code style, macros, naming, and the `CK_ENSURE_IF_NOT` /
  no-silent-fallback non-negotiables (not restated here).
- `docs/campaigns/saveload-rebuild-hydrate/` and `docs/campaigns/saveload-v3-parity/` — the design + parity
  campaigns (spec §4.2 rebuild+hydrate, §4B.3 EntityScript SaveGame fields).

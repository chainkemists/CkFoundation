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

## Slot metadata (the save/load MENU surface)

The snapshot itself carries no player-facing description — `FCk_Snapshot_HeaderV3` has engine version,
timestamp, world path and provenance counts, and reading even those costs a full `LoadGameFromSlot`
that deserializes `_SnapshotBytesV3` (the whole world) to return six fields. A nine-slot menu drawn off
`Get_SaveSlotHeader` deserializes nine worlds.

So a menu reads `FCk_Snapshot_SlotMeta` (`SaveGame/CkSnapshot_SlotMeta.h`) instead — title, timestamp,
world path, PNG thumbnail, and a `TMap<FName, FString>` of game-defined fields CkSnapshot round-trips
opaquely. It lives in a **sidecar slot**, `<Slot>.meta`, written by the same `Request_Save_WithMetadata`
that wrote the snapshot.

| Call | Use |
|---|---|
| `Request_Save_WithMetadata` | the save path any listing UI should use — a bare `Request_Save` leaves NO sidecar, and the slot lists untitled and thumbnail-less |
| `Get_AllSaveSlotNames` | enumerate (sidecars filtered out; see below) |
| `Get_SaveSlotMeta` | one row's metadata, payload untouched |
| `Request_DeleteSaveSlot` | deletes snapshot + sidecar |
| `Get_IsSaveInProgress` | the save-side twin of `Get_IsLoadInProgress` |

Three things bite:

- **Sidecars share the slot namespace.** `ISaveGameSystem::GetSaveGameNames` returns `Slot0` *and*
  `Slot0.meta`; every enumeration must filter with `ck::snapshot::slot_meta::Get_IsMetaSlotName` or the
  menu shows every save twice. `Get_AllSaveSlotNames` already does.
- **A missing sidecar is normal, not an error.** Saves written by `Request_Save`, or before this
  existed, are occupied slots with a default-constructed meta. `Get_IsPopulated()` is the
  discriminator — treat it as "no details to show", never as "not loadable".
- **The thumbnail capture is frame-deferred, and there is deliberately no synchronous one.**
  `Request_CaptureViewportThumbnail` routes through the engine's screenshot pipeline
  (`FScreenshotRequest` + `UGameViewportClient::OnScreenshotCaptured`), whose readback runs INSIDE the
  render frame.

  A synchronous capture cannot be written correctly, which is why the one that used to exist was
  deleted (2026-08-16) rather than fixed. Reading the viewport's render target from the game thread
  only works while Slate composites the viewport into its own buffer — i.e. the editor. A packaged
  game builds its viewport widget with `RenderDirectlyToWindow`, so `FSceneViewport` holds the
  backbuffer only between `BeginRenderFrame` and `EndRenderFrame` on the RENDER thread and the
  game-thread ref is permanently null. Reading it anyway does not fail: D3D12's `RHIReadSurfaceData`
  **memzeroes the output for a null texture**, so `ReadPixels` returns success with an all-black
  bitmap. That was the QA-reported "editor screenshots fine, packaged is always black" bug.

  Consequences for callers:
  - **The save never captures for you.** `Request_Save_WithMetadata` stores
    `FCk_Snapshot_SaveMetadata::_ScreenshotPng` verbatim or leaves the slot pictureless; the old
    `_CaptureScreenshot` / `_ScreenshotMaxWidth` fallback fields are gone. Folding a frame-deferred
    capture into a synchronous save would make the whole save span frames.
  - **Request it as the menu OPENS**, not when Save is clicked — the readback happens before Slate
    composites UMG, so the shot is the gameplay frame rather than the save screen. BusterBlock's esc
    menu does this from its `Construct`.
  - The callback fires **exactly once**: with bytes, or empty on a 2s timeout (no viewport, nothing
    rendering).
  - **HDR is handled.** `ProcessScreenShots` only broadcasts `OnScreenshotCaptured` when it produced an
    LDR bitmap; an HDR viewport writes an `.exr` and fires only `OnScreenshotRequestProcessed`. Both
    are bound, and the file path is captured at REQUEST time because `FScreenshotRequest::Reset()` runs
    before that second broadcast. (SPUD, the 5.5 save plugin, solved it the same way — worth reading
    `SpudSubsystem.cpp:314-416` if this area needs changing.)

## A load travels to the SAVED world, not the current one

`DoInitiate_Travel` opens `_V3Header._WorldAssetPath`, falling back to the current map when the save
records none. This is what lets a **frontend** Load button work: re-opening the current map would
re-open the main menu and then rebuild a gameplay world into it, and a cross-map save would restore
into the wrong level.

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

## Dynamic-fragment snapshot opt-outs

CkDynamic registers ONE blanket save handler for every dynamic (AS `Add_Fragment`-declared) struct on an entity
(`CkDynamic/CkDynamic_Fragment.cpp` — `FCkDynamicFragmentsSaveHandlerRegistrar`,
`Register_SaveOnly<FCk_SaveData_DynamicFragments>`): `Produce` captures **every** dynamic fragment, replicated or
not, and `HydrationApply` re-composes each onto the rebuilt entity — no per-feature registrar needed to opt IN.
Opting a fragment (or a field) OUT is two mechanisms, chosen by granularity:

**Whole fragment — `FCk_DynamicFragment_SnapshotTransient`.** C++ structs *derive* from the marker; AngelScript
structs cannot inherit, so the AS spelling is a **field** of that type instead (`CkDynamic_Fragment_Data.h:31-34`).
`UCk_Utils_DynamicFragment_UE::Get_IsSnapshotTransient` (`CkDynamic_Utils.cpp:452-470`) accepts either spelling —
an `IsChildOf` test on the struct itself, or a scan for a marker-typed `FStructProperty` — and it is honoured on
BOTH sides: `Produce` strips these before saving, `HydrationApply` skips them on load. Reach for it when the
fragment's ENTIRE content is rebuilt by `DoConstruct` replay: cached child-entity handles, `_Signals`, `_Requests`,
`_NeedsSetup` tags. ~25 BB script files already do (`BB_Door_Feature.as`, `BB_CombatReceiver_Feature.as`).

**Automatic — delegate fields.** Since 2026-08-16 hydration ALSO preserves every top-level delegate and
multicast-delegate field, with no opt-in (`Get_IsLiveSessionField`, `CkDynamic_Fragment.cpp`). A delegate binds
UObject + FunctionName, so across a rebuild+hydrate its saved form is stale or empty *by construction* — the
object it named belonged to the torn-down world — while the live value is the set of subscribers that bound
during the rebuild. Copying the saved list over them silently unsubscribes everyone, and the failure is quiet in
the worst way: the feature keeps its state and its one-shot reads still work, so it looks healthy, but it never
notifies again. **"Correct initial value, then frozen"** is the signature (dead HUD counters, prompts that never
re-show). Found 2026-08-16 across 46 unmarked BB `_Signals` fragments. No fragment needs to opt in, and no
future one can forget.

Its limit: **top-level fields only**, matching the `CPF_Transient` rule it sits beside. A delegate nested inside
a struct- or array-typed member is still copied wholesale — "preserve the live one" has no meaning once the
saved and live arrays differ in length. A fragment shaped that way (`FBb_Fragment_Interactable_Signals`, whose
channeled bindings are `TArray<{FGameplayTag, delegate}>`) wants the whole-fragment marker instead. The two
mechanisms divide cleanly: the guard covers delegate FIELDS, the marker covers subscriber-list FRAGMENTS.

**Per-field — `UPROPERTY(Transient)`.** The persistent archive never writes a `CPF_Transient` property
(`FProperty::ShouldSerializeValue`), and hydration copies the payload back onto the rebuilt entity field-by-field
via `CopyFragment_PreservingLiveSessionFields` (`CkDynamic_Fragment.cpp`): if the struct carries ANY preserved
field, every OTHER field copies from the saved payload and each preserved field is left
untouched, so the rebuilt world's freshly-CONSTRUCTED value survives instead of being stomped by the saved
default. Reach for it when one fragment mixes durable state with runtime-only children (`FBb_Fragment_Shelf_State`
— `Inventory`/`StockedSku`/`NextProxyID` persist while probe/interactable/outline/cosmetic-proxy handles are
`Transient`). **The handle-id stream is positional** — a fragment's persisted handle fields must be declared
BEFORE its `Transient` ones: the save's handle-remap walk is a straight positional field walk, not name-keyed, so
a reordered struct attributes a durable saved value to the wrong field.

**Omitting whichever opt-out a fragment needs is the recurring shape of a real incident class.** Hydration's
whole-fragment assign runs AFTER `DoConstruct` has already composed the entity fresh — so a Transient-worthy
runtime handle a construction script just built (a child probe entity, a signal binding, an AI task handle) gets
overwritten by the SAVED value for that field, which typically remaps to `entt::null` because the referenced
child was never independently persisted. Door commit `52309113c`, the BusterBlock NPC freeze-after-load
incident, and the 2026-08-16 QA pair (a ChangeablePoster that focuses but never changes texture; a checkout
counter whose settle offered no cash/credit option because its presented-hand SceneNodes came back as
tombstones) are all this shape: construct-fresh handles stomped by stale hydrated values.

The recurring driver is that **`utils_scene_node::Create` gives its child a debug name, never a
GameplayLabel** — so every SceneNode / probe-node child is an unlabeled `FTag_ConstructSpawned` entity, which
capture rule 3 classifies save-transient. A handle to one can NEVER round-trip. Treat any fragment field naming
a SceneNode, probe, Interactable, UnrealComponent or Tween child as `UPROPERTY(Transient)` by default.

**Backstop:** since 2026-08-16 `CkDynamic`'s `HydrationApply` refuses to DOWNGRADE a live handle — after the
field copy, any handle the save could not resolve is restored from the construction-fresh value
(`Restore_UnresolvedHandles`, `CkDynamic_Fragment.cpp`). It stands down when the saved and fresh layouts hold
different numbers of handle slots, because positional correspondence no longer holds there; and a field the
save deliberately CLEARED keeps its construction-fresh value, since an empty saved handle and an unresolvable
one are the same value by the time hydration sees them. It is a safety net for unaudited fields, **not a
substitute for the opt-out** — a marked field also keeps the dead reference out of the save entirely.

Don't try to express either contract with USTRUCT `meta=(...)` metadata — Game and cooked targets strip metadata
behind `WITH_METADATA` (`CkDynamic/CLAUDE.md`), so a metadata-only opt-out is silently inert in a packaged build.
The `IsChildOf`/marker-field runtime scan above is the only opt-out CkDynamic actually evaluates.

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

Because the intent outranks the key, the two are stamped as ALTERNATIVES, not together — the choice is made at
construction by whoever creates the entity, and it turns on who re-creates the thing after a load:

| The entity's actor / spawner is… | Stamp | Load behaviour |
|---|---|---|
| level-placed (`ck::save_key::Get_IsLevelPlaced`) | `FFragment_SaveKey` from (level package + actor name) | the level re-creates it; the save rendezvouses onto that copy |
| runtime-spawned | `FFragment_ActorSpawnIntent`, if the class opted in | the loader is the only re-creator, so it respawns the actor |

`UCk_EntityScript_WithActor_UE::Construct` and `ACk_EntitySpawner_UE` both apply exactly this split, which is why a
snapshot-respawnable opt-in on a class that is ALSO placed in a level is inert rather than a duplicate source.

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

## Save cost: a save is ONE frame, so everything here is hitch

`Request_Save` pumps the world to quiescence, captures, serializes and writes the slot **synchronously on the game
thread**. There is no frame-spanning save machine (unlike the load). A game with an autosave feels every millisecond
spent in `DoRequest_Save`, so both the audit knob and the timing line below exist to make that budget legible.

### The timing line (always on)

Every save logs one Display line after the write:

```
Request_Save TIMING slot [BbQuickSave]: total [228.81ms] = pump [17.49ms] ([3] passes) + capture [205.02ms]
(classify [40.27ms], payloads [155.88ms] (produce [47.28ms], serialize [93.87ms]), tables [8.87ms])
+ write [4.04ms] (serialize [1.11ms], io [2.93ms]) + sidecar [2.26ms]. Audit [0.00ms] over [0] probes
(inside classify). [4411] entities, [6100] payloads ([20] distinct types), [9589774] bytes
([7992490] payload + [1597284] structural).
```

**Audit is a SUBSET of classify, not a sibling** — it is reported separately rather than added into the total —
and produce/serialize are the two halves of payloads. `TRACE_CPUPROFILER_EVENT_SCOPE`s cover the same stages for
Insights. Read the line before theorising about a save hitch; it was added because CkSnapshot previously had no
instrumentation at all and every attribution was a guess.

### Payload serialization is fork-join parallel

Capture runs every handler's `Produce` on the game thread (it reads live ECS state), collecting the produced
`FInstancedStruct` copies; the per-payload serialization then fans out over a `ParallelFor`. This is GC-safe with
NO guard by construction: GC only ever starts from the game thread, and the game thread is captive inside the
fork-join. Each task owns its payload copy end-to-end (`Serialize_OwnedStruct` builds its archive, proxy and
`FSnapshotContext` per call; the handle walk has no shared state), and slot *i* of the payload table is pending
payload *i*, so the file is byte-identical to the serial order —
pinned by `Ck.Snapshot.V3.ParallelSerializeParity` (CkTests).

Consequences for handler authors: `Produce` must return an OWNED payload (a copy — never a view into live
fragment memory), and a payload struct's `Serialize` path must not touch shared mutable state. Both already
follow from the existing contract; this is why they are load-bearing.
`UCk_Snapshot_Settings::_ParallelPayloadSerialization` (default `Enable`) is the escape hatch — `Disable`
forces the same code path single-threaded (`ForceSingleThread`), for ruling the parallel path out while
diagnosing a save-side crash.

### The save format uses NATIVE serializers, not tagged properties (v7)

`FArrayProperty`'s bulk-memcpy path is gated on **unversioned** property serialization
(`PropertyArray.cpp`, `CanBulkSerialize`) — a cooked-build feature a save archive never uses. So a
`TArray<uint8>` UPROPERTY in a save is walked **one virtual call per byte**: 17.5 MB measured at
554 ms, ~32 MB/s where a memcpy runs at GB/s. That single mechanism was ~75% of a 1273 ms save,
because the blob is serialized twice (the tables build it, then `SaveGameToMemory` re-serializes it).

Consequences for anyone touching the format:

- Every V3 table struct (`FCk_Snapshot_V3_{BuildStep,EntityEntry,PayloadEntry,Tables}`) declares
  `Serialize(FArchive&)` + `TStructOpsTypeTraits::WithSerializer`. **Adding a field means editing
  that serializer** — it is not reflection-driven, so a new `UPROPERTY` alone is silently not saved.
- Byte blobs go through `ck::snapshot::Serialize_BulkBytes`, never `Ar << Array` (`TArray`'s
  `operator<<` is per-element for `uint8`) and never a bare UPROPERTY.
- Nested arrays serialize as explicit count + per-element `Serialize`, because a `WithSerializer`
  USTRUCT gets no `operator<<`.
- `UCk_Snapshot_SaveGame::_SnapshotBytesV3` is deliberately **not** a UPROPERTY; the class's
  `Serialize` override appends it in bulk after `Super::Serialize` writes the reflected header.
- Layout is version-locked: bump `FCk_Snapshot_HeaderV3::CurrentFormatVersion` on any change.
  `Request_Load` compares by exact equality **before** teardown, so an older slot is refused loudly
  while the world is still alive rather than half-read.

### `CaptureAuditMode` — the dropped-payload audit is not free

Rules 1.5 and 3 skip entities, and the audit reports the ones that were carrying a payload (i.e. real data loss).
Deciding that means running **every registered `Produce`** on each skipped entity — including CkDynamic's, which
walks the registry's storage list — so the audit's cost scales with skipped-entity count, not with how many
problems it finds. `UCk_Snapshot_Settings::_CaptureAuditMode`:

| Mode | Behaviour | Cost |
|---|---|---|
| `Disabled` | no probe; no dropped-payload signal at all | free |
| `Summary` (default) | probes, then ONE aggregated Warning with counts + N example entities | probe only |
| `Detailed` | one Warning per entity carrying the full `ExportText` of the dropped payload | probe + ~0.45ms/entity |

Measured on BusterBlock 2026-08-16: `Detailed` (the old unconditional behaviour) spent **177 ms emitting 394
warnings** in a single 7 MB save, every one of them a `PROBE NODE` child. That is the *expected* steady state, not
a backlog — `utils_scene_node::Create` gives its children a debug name and never a GameplayLabel, so every
SceneNode/probe child is permanently an unlabeled rule-3 skip (see the recurring-driver note under
*Dynamic-fragment snapshot opt-outs*). A project shipping an autosave wants `Disabled` on its shipping
configuration and `Summary` while developing; reach for `Detailed` only when chasing a specific value that came
back missing.

---

## The v3 load machine

`DoTick_Load` drives `TearingDown → AwaitingWorld → Rebuilding → Hydrating → Settling`. Rationale that used to live
as comments in `Subsystem/CkSnapshot_Subsystem.cpp`:

- **Hydrating is ATOMIC.** One ticker callback enqueues payloads, queues the reconcile-destroys, AND opens the gate,
  so no gated world-tick ever sees pending payloads; hydration can only drain in post-gate FULL passes (Setup then
  hydration, no stomp). `Settling` then lets the parked destroys finish.
- **The SaveKey resolver is re-swept every rebuild tick, not once at world-ready.** On-demand infrastructure
  (ActorRelay channels) stamps its key ticks after BeginPlay, so a one-shot sweep left every such `EngineOwned` row
  unresolvable and orphaned its whole owned subtree. The sweep Resets and rescans the live
  `FFragment_SaveKey` view; that is lossless because the rebuild's own publish-back writes the fragment onto the
  entity it mapped, so the next rescan re-finds it.
- **The rebuild ESCALATES to full-scope ticks instead of orphaning at kernel stall.** A row can legitimately depend
  on work the kernel cannot run: a multi-stage construction (EntityScript `Continue`) is finished by a GAME
  processor (`GatedDuringLoad` — the load policy is framework-kernel-only by design), and the identity it stamps on
  completion — a child's GameplayLabel adopt key, a SaveKey — is exactly what the resolution scan waits on. When the
  kernel quiesces with unresolved rows, the loader sets `Set_IsLoadGateEscalated(true)` (world ticks the FULL
  processor scope while the load still owns completion) and concludes only when THAT quiesces too. Bridged
  respawn fallbacks fire only at final (post-escalation) quiesce, so a fresh copy created by a gated processor is
  adopted, never duplicated. Rows still unresolved after escalation are real losses: Error log,
  `Get_UnresolvedAfterEscalation` on the report, per-row orphan records. Loads that fully resolve in the kernel
  never escalate (`Get_UsedEscalatedRebuild` false) and behave exactly as before. Chaos exposure of pre-hydration
  full-scope ticks is the same class the design already accepts post-gate (hydration drains over live frames with
  `NotReady` retries), bounded to the escalation window. Fence:
  `Ck.Snapshot.StagedConstructionAdoption` (CkTests) — a parent that defer-spawns promise-labeled children whose
  construction only a gated processor can finish, three levels deep; the RetailGondola incident (2026-07-29)
  reduced to framework terms.
- **A bridged actor is spawned DEFERRED and its saved `UPROPERTY(SaveGame)` fields applied before `FinishSpawning`.**
  The recipe carries them in `_ActorSaveFieldBytes` (ArIsSaveGame tagged-property blob, captured off the owning
  actor). Plain `SpawnActor` gave BeginPlay — and the WithActor `Construct` it drives — class defaults, so an actor
  whose composition branches on a saved field (a world item's `ItemDefinition`) came back inert and a later payload
  hydration back-filled the field onto an entity that had already skipped composing. Empty bytes (no SaveGame
  property on the class) spawn exactly as before.
- **Spawn-params handle refs are remapped MID-rebuild, against whatever `_SavedIdMap` holds on THAT tick — there
  is no retry.** Unlike ownership restore and payload hydration below (both of which wait for mapping to settle
  before running), the RuntimeSpawned leg deserializes a recipe's spawn params through `DoDeserialize_V3Blob`
  (`Subsystem/CkSnapshot_Subsystem.cpp:598-624`) and remaps its handle refs via `ck::snapshot::RemapHandles`
  **at the moment that row is processed** (`:826-878`, gated only on its OWNER having resolved) — a handle
  referencing a saved-id not yet in `_SavedIdMap` remaps to `entt::null` PERMANENTLY; nothing revisits it once
  the target eventually maps on a later tick. The capture-side AUDIT ensure
  (`Snapshot/CkSnapshot_CaptureV3.cpp:447-463`, `v3 capture: RuntimeSpawned entity [...] spawn params reference
  entity [...] which is NOT persisted`) only catches a ref to an entity that will NEVER be persisted at all — it
  cannot catch a ref to an entity that IS persisted but simply hasn't been rebuilt yet on this tick, which is
  exactly the silent-loss case this bullet is about. Consequence for authors: never park a load-bearing world
  reference in an `ExposeOnSpawn` spawn-params handle field — re-resolve it at construction/BeginPlay from
  durable identity (a label, a SaveKey) instead, or make the reader tolerate an invalid handle.
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

## Load-gate spawn quarantine

While `Get_IsLoadGateActive()` is true, `UCk_Utils_EntityScript_UE::Request_SpawnEntity` suppresses any spawn that
is not the loader's own reconstruction — the check runs SYNCHRONOUSLY inside `Request_SpawnEntity` itself
(`CkEcs/EntityScript/CkEntityScript_Utils.cpp:161-202`, calling `Get_IsSpawnSuppressedByLoadGate` at `:36-74`).
Rationale is the save-inflation incident the code comment cites (2026-07-29): a census/adopt-or-spawn policy read
the half-rebuilt world's near-zero population and spawned to fill the gap, so the next capture recorded both the
loader's restored copy AND the policy's fresh one (+77 NPCs and a doubled StoreDriver subordinate family in one
save→load→save cycle).

Three windows are admitted (`CkEcs/Subsystem/CkEcsWorld_Subsystem.h:161-266`), each RAII-scoped — never set
directly:

| Window | Scope guard | Legitimate for |
|---|---|---|
| Loader spawn window | `FCk_ScopedLoaderSpawnWindow` (`Push_LoaderSpawnWindow`/`Pop_LoaderSpawnWindow`) | The loader's own recipe replays / definition rebuilds — orchestrator-only; game code must never open one |
| Rendezvous spawn window | `FCk_ScopedRendezvousSpawnWindow` (`Push_RendezvousSpawnWindow`/`Pop_RendezvousSpawnWindow`) | World bootstrap re-creating IDENTITY-BEARING content the loader ADOPTS instead of respawning (a SaveKey / adopt-label rendezvous target). Census/count-driven population spawns must NEVER open this — it is exactly the doubling class the quarantine exists to stop |
| Construction window | `UCk_Utils_EntityLifetime_UE::Get_IsInsideConstructionWindow(LifetimeOwner)` | A spawn issued by an owner still inside its own Construct — the owner's replayed construction is expected to re-create its children |

A suppressed spawn returns an **invalid** `FCk_Handle_PendingEntityScript`; `Promise_OnConstructed` no-ops on it
and the completion delegate reports `Failed_NotEnqueued` — reconcile-shaped callers converge on their next
real-world evaluation instead of erroring. The suppression site also logs a Warning naming the class and lifetime
owner and pointing at the two escape hatches below.

Two consumer-side tools, and a gap between them worth knowing:

- **`UCk_Utils_Snapshot_UE::Get_IsLoadInProgress(InHandle)`** (`CkSnapshot_Utils.h:49-60`) — gate
  construction-time SEEDING on this returning false. The hazard it documents (`:51-54`): a construction script
  that unconditionally seeds a separately-persisted entity creates a SECOND copy beside the one the load is
  about to restore (children composed UNDER the seeding script are fine — replayed construction re-creating
  them is how rebuild works — but a sibling/global seed is not).
- **`Request_SpawnEntity_LoadRendezvous`** (`CkEcs/EntityScript/CkEntityScript_Utils.h:93-107`, impl
  `.cpp:238-254`) — the sanctioned call for legitimate mid-load spawns: it opens a
  `FCk_ScopedRendezvousSpawnWindow` around an ordinary `Request_SpawnEntity`, so it behaves identically when no
  load is active and passes the quarantine when one is. Reserved for spawns that carry (or will acquire during
  construction) a stable SaveKey/adopt-label the loader rendezvouses onto — never for count-driven population.
- **The gap the quarantine cannot close by construction, not by oversight:** the suppression check keys off
  `Get_IsLoadGateActive()`, which the load machine turns off partway through Hydrating — the same ticker callback
  that enqueues payloads and opens the gate (see "Hydrating is ATOMIC" above) — while `Get_IsLoadInProgress` /
  `Promise_OnLoadComplete` stay live all the way through Settling until `OnLoadComplete` fires. A spawn issued in
  that window — e.g. from an EntityScript's `DoBeginPlay`, which fires only once construction/composition has
  finished and typically lands exactly there — sails through `Request_SpawnEntity`'s suppression check
  unchallenged, because `Get_IsLoadGateActive()` already reads false. If that `DoBeginPlay` unconditionally seeds
  a separately-persisted sibling, it creates a duplicate beside the entity the load is still in the middle of
  restoring, and the spawn-level quarantine never sees it as suppressed — it isn't a suppressible spawn by the
  time it fires. The guard belongs at the PRODUCER: gate the seed on `Get_IsLoadInProgress`, or — more robust —
  don't seed unconditionally at all; spawn only what a reconcile pass run from `Promise_OnLoadComplete` finds
  missing after the restore has actually landed.

---

## `Promise_OnLoadComplete` — the consumer settle point

Feature PROCESSORS are the only thing the load gate freezes; promise/signal CALLBACKS are not — a bound delegate
fires whenever the code that broadcasts it runs, gate active or not. Any consumer reading world-scoped state (an
occupancy roster, a population count, a tag scan) from inside a callback that CAN fire mid-load must route that
read through `UCk_Utils_Snapshot_UE::Promise_OnLoadComplete` (`CkSnapshot_Utils.h:62-75`, impl
`CkSnapshot_Utils.cpp:80-117`) instead of acting directly on the half-rebuilt world.

- **No load in progress** — the delegate fires IMMEDIATELY, synchronously, with a default-constructed
  `FCk_Snapshot_LoadReport{}` (there was no load to report on).
- **A load IS in progress** — binds `ck::UUtils_Signal_Snapshot_OnLoadComplete` on the world's transient entity
  with `ECk_Signal_BindingPolicy::IgnorePayloadInFlight` / `ECk_Signal_PostFireBehavior::Unbind`: a ONE-SHOT bind
  that fires exactly once, on THIS load's completion. `IgnorePayloadInFlight` is load-bearing, not incidental — a
  replay policy (`FireIfPayloadInFlight`) would fire immediately with a PRIOR load's already-in-flight report
  while the current load is still reconstituting the world, which is the exact half-coherent read this API exists
  to prevent.
- Fires post-settle: the load machine's Settling phase pumps hydration to quiescence before `OnLoadComplete`
  broadcasts (see "Settling waits on hydration, not just frames" above), so restored values are guaranteed
  readable by the time the callback runs.

`Get_IsLoadInProgress` is the POLL form of the same fact; `Promise_OnLoadComplete` is the PUSH form for a
consumer that would otherwise have to poll every tick.

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

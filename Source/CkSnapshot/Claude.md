# CkSnapshot

**Purpose:** Save / restore whole-world ECS state — the **v3 rebuild+hydrate** save system. A save captures a
per-entity construction recipe (classified by provenance) plus each feature's persisted payload; a load rebuilds
every entity from its recipe, then hydrates the payloads back onto the freshly-constructed entities. There is no
byte-image snapshot and no per-fragment registration macro — `CK_REGISTER_SNAPSHOTABLE` and the Model-A "fidelity
oracle" were deleted 2026-07-13. A feature persists by registering a `Produce`/`HydrationApply` handler on the
`CkEcs` replicated-fragment registry — the SAME registry the network path uses.

**Depends on:** `Core`, `CoreUObject`, `CoreOnline`, `Engine`, `ImageCore`, `CkCore`, `CkEcs`, `CkEcsExt`,
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

**And `NotReady` may mean COMPOSITION and nothing else.** Never gate on the feature's own
`NeedsSetup`/`RequiresSetup` marker, and never on the output of a processor outside the load kernel: the
quarantine holds a restored entity out of exactly those processors' views until its payloads apply, so the wait
cannot end and the entry is dropped at the timeout. The ordering such a gate wants is now a guarantee —
Setup runs AFTER hydration and reads the restored Durable fragments as its inputs, the way it reads params on a
fresh entity. When the restore truly needs Setup's output (a resolved render target, an allocated grid), apply
immediately anyway: enqueue the feature's own deferred `Request_*` (its `HandleRequests` processor already runs
after `Setup` and excludes the setup marker), or park the payload in a fragment a post-Setup processor consumes
(`FFragment_Sm_HydrationResume`, `FFragment_RenderTarget_HydrationReplay`), and return `Applied`.

**The mirror rule, on Setup's side: work Setup still owes a construct seed is tracked with a marker, never
inferred from the value.** A Setup that finishes what `Add` started — Velocity's and Acceleration's
local→world conversion is the shipped case, since the `Transform` to rotate against may not exist at `Add`
time — cannot ask "does this still look like the starting param?" to decide whether the work is outstanding.
It looks exactly like it for the entity whose saved value *is* the starting param, and that entity gets
converted a second time on every load. `Add` stamps a Session marker, `Setup` consumes it, and the hydration
handler removes it because a restored value is already in final form. The value answers *what*; only a marker
answers *what has been done to it*.

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

## Dynamic-fragment posture — what is captured, and what the rebuild owns

CkDynamic registers ONE blanket save handler for every dynamic (AS `Add_Fragment`-declared) struct on an entity
(`CkDynamic/CkDynamic_Fragment.cpp` — `FCkDynamicFragmentsSaveHandlerRegistrar`,
`Register_SaveOnly<FCk_SaveData_DynamicFragments>`), so no per-feature registrar is needed to participate. What
that handler does with a given fragment is decided ENTIRELY by the fragment TYPE's snapshot **posture**, resolved
by `ck::Get_FragmentPosture` (`CkEcs/Snapshot/CkSnapshot_Posture.h`) — the single authority both sides call:

| Posture | `Produce` | `HydrationApply` |
|---|---|---|
| `Session` | not captured | skipped — never written, never composed onto the rebuilt entity |
| `Durable` | captured whole | assigned whole onto the rebuilt fragment |
| `Undeclared` | captured | assigned whole — an author error the resolver already ensured on, see below |

**Declaring one.** Two symmetric marker structs live in CkEcs: `FCk_Snapshot_Durable` (part of the saved world)
and `FCk_Snapshot_Session` (rebuilt by the feature's own construction/setup). C++ structs *derive* from a marker;
AngelScript structs cannot inherit, so the script spelling is a **field** of that type, and the resolver accepts
either. There is deliberately no metadata spelling — Game and cooked targets strip metadata behind
`WITH_METADATA` (`CkDynamic/Claude.md`), so a `meta=(...)` opt-out is silently inert in a packaged build.
`FCk_Snapshot_Session` is the **deprecated** spelling of `FCk_Snapshot_Session` and still
resolves Session, so the script sites carrying it keep behaving identically; new code writes the new name.

**Some postures are DERIVED, and a derivation is a proof rather than a default** — it says the declared
alternative is unachievable, not that nobody got round to declaring:

| Shape | Pure ⇒ | The same shape ALSO carrying value fields ⇒ |
|---|---|---|
| no reflected field other than a posture marker — a tag | `Session` | n/a (a tag has no value fields) |
| the TYPE walk reaches a delegate / multicast-delegate at ANY depth | `Session` | `Undeclared` + a "SPLIT ME" reason |
| a script fragment whose name ends in `Requests`, or a type reaching an `FCk_Request_Base` | `Session` | `Undeclared` + a "SPLIT ME" reason |

- **Delegates** bind UObject + FunctionName, so across a rebuild+hydrate the saved form is stale or empty *by
  construction* — it named objects in the torn-down world — while the live value is the set of subscribers that
  bound during the rebuild. Restoring the saved list silently unsubscribes everyone, and the failure is quiet in
  the worst way: state and one-shot reads still look right, but the feature never notifies again. **"Correct
  initial value, then frozen"** is the signature (dead HUD counters, prompts that never re-show; found 2026-08-16
  across 46 unmarked BB `_Signals` fragments). Deriving Session closes that class structurally, at any nesting
  depth, with nothing for a future fragment to forget.
- **Tags** mark processing state that the replayed construction stamps again. Capturing one means hydration hands
  it back to an entity whose Setup already consumed it, and the setup runs twice.
- **Request queues** are in-flight work, not world state: a restored queue re-arms its processor against a world
  that has already moved on.

**A mixed shape is never silently reclassified.** A fragment matching a derived-Session shape that also carries
real value fields resolves `Undeclared` and reds as "split me" — it keeps being captured meanwhile, because a
derivation that dropped durable data would be the exact bug this design exists to prevent (two live BB cases are
plain `int32` accumulators sitting inside `…Requests` fragments). Split it: durable fields into a `Durable`
fragment, session content into a `Session` one.

**A declaration never overrides a derivation.** `Durable` on a delegate-carrying, request-carrying or field-less
type ensures and resolves `Undeclared`. So does `Durable` alongside a `UPROPERTY(Transient)` game field:
field-level opt-out is **retired as an author mechanism** — a fragment is Durable whole or Session whole, and a
mixture is a fragment that has not been split yet.

**`Undeclared` is now an author error, not a transitional path.** The posture ratchet
(`Ck.Snapshot.Meta.FragmentPostureCoverage`) reds on any `Undeclared` type outside the project's allow-list, and
that list only ever shrinks — BusterBlock drained it to zero, which is what let the transitional guard go. What
can still resolve `Undeclared` is a shape the resolver already ensured on (a contradiction, a `Durable`
declaration over session content) or a type outside the ratchet's scope; it is captured and assigned whole,
exactly like `Durable`, so nothing is silently dropped while the red is open.

**What the deleted guard used to do, and what replaced it.** Hydration once copied an `Undeclared` fragment
field-by-field, keeping every `CPF_Transient` field and every top-level delegate
(`CopyFragment_PreservingLiveSessionFields`), and refused to downgrade a live handle the save could not resolve
(`Restore_UnresolvedHandles`, a positional walk that stood down on layout drift). Both were safety nets over
fragments nobody had declared. The declaration replaced them, and it is strictly stronger: the delegate
derivation walks the type at ANY depth rather than top-level only, and a fragment holding construct-rebuilt
child handles is `Session` whole rather than patched field-by-field. One consequence retires with them — the
handle-id stream no longer has to be positionally ordered around skipped fields, because a captured fragment
has no skipped fields.

**The incident class all of this comes from.** Hydration runs AFTER `DoConstruct` has already composed the entity
fresh, so a runtime handle a construction script just built (a child probe entity, a signal binding, an AI task
handle) used to be overwritten by the SAVED value for that field — which typically remaps to `entt::null`,
because the referenced child was never independently persisted. Door commit `52309113c`, the BusterBlock NPC
freeze-after-load incident, and the 2026-08-16 QA pair (a ChangeablePoster that focuses but never changes
texture; a checkout counter whose settle offered no cash/credit option because its presented-hand SceneNodes came
back as tombstones) are all that shape. The recurring driver is that **`utils_scene_node::Create` gives its child
a debug name, never a GameplayLabel** — so every SceneNode / probe-node child is an unlabeled
`FTag_ConstructSpawned` entity, which capture rule 3 classifies save-transient, and a handle to one can NEVER
round-trip. A fragment whose fields name SceneNode, probe, Interactable, UnrealComponent or Tween children is
`Session` state: declare it, do not try to persist half of it.

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

**The report accounts for 100% of the save, and it closes on APPLY.** Entities partition into restored + skipped +
orphaned. Payloads partition by what each row RESULTED IN: applied + rejected + dropped-no-handler +
dropped-timeout + destroyed-with-entries + unapplied-at-finish + on-skipped + on-orphaned + on-unresolved-owner +
dropped (a failed deserialize). `Get_IsAccountingClosed` asserts both sums and `DoFinish_Load` ensures on it —
**not** `DoHydrate_Enqueue`, because at enqueue time nothing has been applied yet and the question the closure asks
has no answer. `_PayloadsEnqueued` survives as the useful mid-load diagnostic but is no longer a term: it says a row
reached the queue, so a report that balanced on it balanced just as happily on a load that dropped everything it
enqueued (F-U1.3). A payload whose owner id is absent from the entity table, or whose mapped handle went invalid, is
the `on-unresolved-owner` bucket — it exists so nothing has to be inferred by subtraction.

The four apply buckets come from `ck::FCtx_HydrationOutcomes` (`CkEcs/Persistence/CkPersistenceHydration.h`), a
registry context the load zeroes when the post-travel world comes up and `DoFinish_Load` reads exactly ONCE.
`ck::persistence_apply::ApplyOne` increments it per terminal outcome. Two consequences worth knowing:

- **Post-fold outcomes are logged, never retro-counted.** The dispatcher is not load-gated, so it keeps draining
  after the load goes Idle; anything still queued at the fold is counted as `unapplied-at-finish`, which is the
  honest name for "in flight when the snapshot was taken" — some of it may apply a frame later.
- **Entries on an entity entering destruction are written off where destruction begins** (`Request_DestroyEntity`),
  counted as `destroyed-with-entries` and REMOVED there. Counting without removing would put one payload row in two
  buckets: the dispatcher ignores pending-kill, so the same entries would also be applied or swept afterwards.

**`Result` answers one question — did the load COMPLETE?** `Success` (completed; everything applied),
`Succeeded_WithLoss` (completed; NAMED payloads did not, and the world is playable without them), `Failed_*` (it did
not complete). `Get_DidLoadComplete()` is true for both `Succeeded_*` and is what a consumer should branch on: a
`== Success` comparison silently starts meaning "and nothing was lost", which is how a lossy-but-fine load ends up
skipping a caller's entire post-load fixup. `NoLoadInProgress` is the fourth value and belongs to the promise path
below — it is the honest answer for "there was no load", which used to be reported as `Failed_IO`.

`DoCompute_LoadResult` runs LAST in `DoFinish_Load`, after the fold, and only ever DOWNGRADES `Success`: any of
rejected / no-handler / timed-out / destroyed-with-entries / unapplied-at-finish / failed-to-deserialize, or any
entity forced out of the quarantine, makes it `Succeeded_WithLoss` and logs an Error per named loss.
`_EntitiesOrphaned` and `_UnresolvedAfterEscalation` are deliberately NOT in that trigger set: orphans are a routine
outcome of the current loader and already carry their own per-row Warning, so including them would make virtually
every load report a loss and drain the distinction of meaning. That exclusion is a deliberate, temporary weakening
recorded as such, not an oversight.

Each unapplied payload is NAMED in `_PayloadLosses` (type, owning entity, reason bucket), sourced from the same
`FCtx_HydrationOutcomes` and capped so a pathological load cannot grow the list without bound. A count tells a
consumer its world came back incomplete; only a name tells it which part.

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

**The rule IDs below are stable identifiers, not the evaluation sequence** — ~20 call sites across CkFoundation,
CkTests and the game cite them by number, so they never get renumbered. The actual branch order is:
**1 → 1.4 → 1.5 → 4a → 2 → 3 → 4b → 4.5 → 5.**

| Rule | Test | Outcome |
|---|---|---|
| 1 | `IsMarkedForDestruction` (any `FTag_DestroyEntity_*`) | skip |
| 1.4 | `FTag_Snapshot_ReconstructOnly` | skip, NOT counted, NO audit — the feature declares the omission intentional (it rebuilds from authored defaults), so a `Produce`-able payload on one is dropped by design |
| 1.5 | `FTag_Snapshot_SaveTransient` | skip + count; derived state the owner's construction/re-drive recreates on load. A payload on one is an AUDIT Warning (it will be dropped) |
| 2 | `FFragment_SaveKey`, or a player pawn/controller/state rendezvous (world path only) | `EngineOwned` |
| 3 | `FTag_ConstructSpawned` **with a real (named) label** | `ConstructSpawned`. Unlabeled ⇒ save-transient, skipped + counted; a payload on one is an AUDIT Warning |
| 4 | `FFragment_SpawnRecipe` (retained EntityScript spawn recipe) | `RuntimeSpawned`. Two branch positions: **4a** = recipe **AND** `FFragment_ActorSpawnIntent`, tested BEFORE rule 2; **4b** = recipe alone, tested after rule 3 |
| 4.5 | `FFragment_BuildRecipe` (built via `Request_BuildAndReplicate`) | `DefinitionBuilt` |
| 5 | none of the above — anonymous scratch | skip + count; a payload on one is recorded in the SAVE REPORT (below), not warned about |

**Rule 5 and runtime-created features — the C5 statement.** A feature composed after construction (a timer an SM
state starts, one a request handler arms) has no construction recipe and no save identity, so rule 5 skips it and
its state does not persist. That is the DESIGNED outcome under C5: the durable intent is the deadline, held by the
owning feature, whose Setup re-creates the timer from it — a restored timer entity would be resuming a plan the
world has already moved past. What was wrong before is only that the omission was INVISIBLE: nothing counted it,
nothing named it, and an author could not tell a deliberate session-scoped loss from a silent drop (three
BusterBlock timers vanished exactly this way). So the capture now records each one in
`FCk_Snapshot_SaveReport::_UncapturedRuntimeEntities` (entity + first producing type), emits ONE summary Display
line per save, and puts the per-entity detail at Verbose. Deliberately NOT a Warning: it is a census of a designed
outcome, and the AngelScript autotest runner escalates warnings to failures, so warning here would fail every test
that saves. A feature that genuinely needs its runtime state to survive gives the entity durable identity (a
SaveKey) or moves the state to its persisted owner — the report is what makes that decision informed.

Three things the table alone does not say:

- **Rules 1.4/1.5 walk the `FFragment_LifetimeOwner` chain** (`DoGet_SnapshotExclusionPolicy`, depth-capped at
  256), not just the entity itself: an exclusion marker on an owner applies to its whole construction subtree,
  because a child restored without its intentionally-omitted owner would be an orphan on load.
- **`ReconstructOnly` wins over `SaveTransient`** when the walk finds both — the enclosing feature is explicitly
  declaring the omission is by design, so it must not raise the data-loss audit.
- **4a precedes rule 2**, which is what makes `ActorSpawnIntent` and `SaveKey` alternatives rather than a
  duplicate source: a KEYED bridged entity is still loader-respawned, and its key is retained and republished
  after rebuild (see the table under *Provenance*).
- **The world transient is resolved up front and skipped — and a handler that PRODUCES for it now warns.** That
  one IS a declaration defect rather than a design choice: durable state was put somewhere the save cannot reach,
  and no type-level ratchet can see it, because the payload type is perfectly fine and only its owner is
  unpersistable.

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
Request_Save TIMING slot [BbQuickSave]: total [110.33ms] = pump [12.77ms] ([3] passes) + capture [90.55ms]
(classify [25.46ms], payloads [53.14ms] (produce [33.02ms], serialize [9.88ms]), tables [11.95ms])
+ write [4.98ms] (serialize [1.88ms], io [3.10ms]) + sidecar [2.02ms]. Audit [0.00ms] over [0] probes
(inside classify). [4426] entities, [6118] payloads ([20] distinct types), [9618721] bytes
([8015572] payload + [1603149] structural).
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
- **The same callback QUARANTINES every mapped entity** (`ck::FTag_Hydration_Quarantine`,
  `CkEcs/Tag/CkTag_HydrationQuarantine.h`), because the pass that opens the gate is the pass a feature's Setup would
  otherwise run against construct-default Durable state. It is stamped HERE and not at row mapping: a RuntimeSpawned
  row is mapped while still constructing, and its finisher is a GAME processor, so quarantining at mapping starves
  the very processor the rebuild waits on. The whole set is released together (below), never one entity at a time —
  the exclusion is a view filter, not a memory barrier, so a released entity reading a still-quarantined sibling's
  fragment directly is exactly what a per-entity release would allow. Entering destruction leaves the quarantine
  (`Request_DestroyEntity`), which is why the 143 destruction-pipeline processors need zero exemptions.
- **`DoConstruct` and `DoBeginPlay` observe construct defaults; restored values are observable only in a
  quarantine-gated Setup processor and in `Promise_OnHydrated`.** Neither construction hook is held for the
  load — holding `DoBeginPlay` would deadlock the features that compose children or spawn from it, and the
  entity is not hydrated yet when they run, so a Durable fragment read there answers with what `Add` seeded.
  This is a contract, not a timing accident: the quarantine is what makes the Setup half of it reliable, and it
  is why a feature that needs a restored value reads it from Setup rather than from construction.
  `UCk_Utils_Snapshot_UE::Promise_OnHydrated` is the other half, for a consumer with no Setup processor of its
  own — a widget, a subsystem, another entity. Bind it FROM `DoBeginPlay`; that is the pairing the contract is
  built around, and it is why holding BeginPlay was never necessary.
  `Ck.Snapshot.Ordering.BeginPlayObservesConstructDefaults` pins both halves so a future change cannot quietly
  start holding BeginPlay instead.
- **`Promise_OnHydrated` fires once, after this entity's payloads — and every other mapped entity's — have
  applied, or immediately if nothing is pending for it.** The immediate path is not a degenerate case: a fresh
  spawn, a client, a world with no load in flight, and a bind made after the lift all mean the same thing, and
  a promise that stayed silent on any of them would put every consumer back to polling a marker. It fires for a
  cap-forced entity too, with that entity's loss already recorded in the report — see the two escapes below.
- **The SaveKey resolver is re-swept every rebuild tick, not once at world-ready.** On-demand infrastructure
  (ActorRelay channels) stamps its key ticks after BeginPlay, so a one-shot sweep left every such `EngineOwned` row
  unresolvable and orphaned its whole owned subtree. The sweep Resets and rescans the live
  `FFragment_SaveKey` view; that is lossless because the rebuild's own publish-back writes the fragment onto the
  entity it mapped, so the next rescan re-finds it.
- **The rebuild ESCALATES to full-scope ticks instead of orphaning at kernel stall.** A row can legitimately depend
  on work the kernel cannot run: a multi-stage construction (EntityScript `Continue`) is finished by a GAME
  processor (`GatedDuringLoad` — the load policy is framework-kernel-only by design), and the identity it stamps on
  completion — a child's GameplayLabel adopt key, a SaveKey — is exactly what the resolution scan waits on. When the
  kernel quiesces with unresolved rows, the loader sets `Set_LoadHold(ECk_EcsWorld_LoadHold::Escalated)` (world ticks the FULL
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
- **`FTag_Hydration_WasHydratedThisLoad` is stamped on every mapped entity before the gate opens**, beside the
  quarantine, and is never removed. Game-side rebind processors (BusterBlock's `Bb_SnapshotRestore` fleet) key off
  it to re-resolve handles their persisted fragments carry. The Model-A purge deleted the old stamp site and
  silently killed every consumer; v3 restores the semantic. Because it is permanent it answers "was this entity
  ever hydrated", not "is its restored state ready" — the latter is `Promise_OnHydrated`, and the reason the
  rename says `WasHydratedThisLoad` rather than `JustRestored` is that "just" was the part that was never true.
  Its reflected form is `UCk_Utils_Snapshot_UE::Get_WasHydratedThisLoad`, and its one sanctioned use is the
  restored-vs-replaced discriminator (adopt the subordinate the loader rebuilt, or spawn a second one). The
  deprecated alias of this tag and its "just restored" accessor are gone.
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
- **Two predicates gate Settling, deliberately not one.** `DoIs_PayloadDrainComplete` (no live
  `FTag_Hydration_PendingApply`) gates the quarantine RELEASE; `DoIs_HydrationComplete` (that, AND the quarantine
  already released) gates FINISHING. Collapsing them is circular — the release would wait on a condition only the
  release can make true, and every load would burn the frame cap. After the release, at least one further FULL pump
  runs before `OnLoadComplete`, and that is the pass the freed Setup processors take.
- **The quarantine has two bounded escapes, because fail-closed without one is a permanent wedge.** At
  `kLoad_HydrateFrameCap` with the queue still draining, and unconditionally in `DoFinish_Load` (covering the abort
  and teardown routes), every still-quarantined entity is released. Each one that still had queued payloads is logged
  as an Error AND recorded in `FCk_Snapshot_LoadReport::_QuarantineForced` with its outstanding-entry count — a load
  that came back incomplete says which entities, not just that it happened. Both sweeps skip stale handles: the mapped
  set is append-only and is not pruned when an entity is destroyed mid-load.

### Restore invariants

`Verify_AllStoredHandlesResolve` (`Snapshot/CkSnapshot_RestoreInvariants.*`) reports every structurally-inconsistent
stored handle in a restored registry: a dangling HARD ref (`LifetimeOwner` / `ContextOwner`), or a live
`LifetimeDependent` whose own `LifetimeOwner` names someone else (aliasing / registry-rehome corruption). Unset and
tombstone handles are skipped, and an empty result means the backbone is consistent. The dependents leg asserts
**back-pointer consistency, not resolvability**: `FFragment_LifetimeDependents` is a lazily-pruned WEAK-ref list, so
an entry pointing at a destroyed entity is by design and is not reported.

---

## The hold — a load does not simulate the world it is rebuilding

`ECk_EcsWorld_LoadHold` (`CkEcs/Subsystem/CkEcsWorld_Subsystem.h`) is ONE value spanning the whole load, in every
world it passes through, and the only input to what a world-actor tick may do. `ck::Get_TickPlanForHold` is that
policy as a pure function, so the table is unit-testable without a world:

| Hold | Scope | Tick time | Why |
|---|---|---|---|
| `None` | Full | real | normal play |
| `Teardown` | **Full** | 0 | kernel scope WEDGES a teardown — the destruction pipeline, the entity-lifecycle groups and all 136 feature EndPlay processors are outside the kernel |
| `Rebuilding` | LoadKernel | **real** | the only phase that keeps real time: the kernel's own watchdogs must keep making progress while nothing else does |
| `Escalated` | Full | 0 | full scope so multi-stage constructions finish; zero time so time-paced world policy cannot act on a mid-rebuild census |
| `Draining` | Full | 0 | payload applies, and the deferred requests those applies issue |
| `Converging` | Full | 0 | physics steps and probe overlaps converge before the player is handed the world |

**Game time is frozen by the engine's own dial, not by a new clock.** The loader holds
`AWorldSettings::TimeDilation` at `MinGlobalTimeDilation` for the whole load, which stops `GetTimeSeconds()`, every
actor tick's `DeltaSeconds`, every `CkTimer`, every cadence, and every AngelScript `utils_time::Get_TimeNow` caller
AT ONCE, with no edit at any of them. It is `transient` on a per-level actor, so it does NOT survive travel and is
re-applied at the post-travel boot seed. "Frozen" means at most ~17 ms across a whole load, never exactly zero, so
every threshold is an epsilon. Wall time keeps running deliberately, for a small named list of watchdogs — see the
allow-list fence `Ck.Snapshot.Meta.WallTimeReadsAreAllowListed`.

**The freeze is single-valued: ONE world, ONE captured prior dilation.** That fits the shape a load has — the
pre-travel world is frozen, dies, and the post-travel world is frozen in its place — but it cannot express two
worlds frozen at once, because the second apply would overwrite the prior value the first world has to be given
back. So `DoApply_TimeFreeze` **refuses** a second world while the one it holds is still LIVE (`CK_ENSURE` +
no-op), and re-arms freely over a world that has begun tearing down, which is every load's own travel.
`Ck.Snapshot.LoadHold.TimeFreezeRefusesASecondLiveWorld` / `…RearmsOverATearingDownWorld` pin both halves.

**A PAUSED world is reported, never obeyed.** A paused world does not tick and every phase of a load is driven by
world ticks, so a pause taken mid-load does not pause the load — it stops it, and if it outlives a phase's frame
cap the load escapes there and reports `Succeeded_WithLoss` for facts that never got a frame in which to converge.
The framework does not veto the pause (pausing is a game-side decision; the sanctioned guard is a pause UI that
declines while `Get_IsLoadInProgress`), but the loader emits **one** Warning per load naming the world and the
epoch, so the cap-escape is explained rather than mysterious. The predicate is `AWorldSettings::
GetPauserPlayerState()`, not `UWorld::IsPaused()` — the latter is also true for async-loading blocks, a
committing map change and an editor debug-pause, none of which the message describes and any of which would
consume the one-shot. Fence: `Ck.Snapshot.LoadHold.PauseUnderHoldIsReported`.

**The boot seed is `Converging`, for both roles.** A world coming up mid-load is held before anything in it ticks.
`Rebuilding` was tried here and broke the load: it is the one phase that suppresses `Request_SpawnEntity`, and the
level's own on-demand infrastructure spawns its entity and stamps the SaveKey the loader rendezvouses onto in
exactly those first frames — suppressing them left three `EngineOwned` rows `savekey-miss` and cascaded 64 more
into `owner-orphaned` (measured: mapped 85 / orphaned 67, against 152 / 0 on the reference). The BeginPlay-seeding
window that motivated `Rebuilding` is real, but it belongs to the PRODUCER — `Get_IsRebuildInProgress` is true in
every phase — because the framework cannot refuse spawns here without refusing the rebuild's own roots. The
role decision is made from load OWNERSHIP — a world carrying an epoch
this GameInstance did not itself produce — never from a net role, because `UWorld::InternalGetNetMode` falls back
to `PlayInEditorNetMode` when the net driver is not yet attached, which is exactly the instant the seed must answer.

**Convergence is a tri-state registry, and it reads STABILITY rather than silence.** Each module registers the
facts it owns (`Jolt.*`, `SpatialQuery.*`, `Ecs.*`); predicates are PURE reads and the work they measure is driven
once per frame by the loader. `NotApplicable` is what keeps a Jolt-less world from burning the budget on physics
facts it cannot have. The two `Ecs.*` rows are satisfied by a count that is zero **or** unchanged for
`kLoad_ConvergenceStableFrames`: a live world never goes silent — state machines re-evaluate, request queues
refill, NPC AI creates and destroys query entities every frame — so "another frame would change nothing" is the
observable, and absolute zero was simply the wrong question. Measured: framework loads converge in 2-3 frames, BB
content worlds in 7. Every phase is bounded, and each escape NAMES what it gave up on (`_ConvergenceUnmet`,
`Succeeded_WithLoss`).

**The client contract.** A client has no load, no report and no completion of its own, but on a listen-server
reload it travels and rebuilds too. It ARMS from the travel URL's `?CkLoad=<epoch>` option — readable at its first
world tick, and safe because `FURL::GetOption` matches by prefix so the engine's `load`-named-option trap does not
fire. It RELEASES on the server's ready-to-resume fact **wherever that fact lands**, AND that entity's
replication-complete signal. It deliberately does NOT acquire a relay channel of its own: ActorRelay channels are
pooled PER SIDE, so the entity a client would acquire is not the one the server's fact replicates onto — a client
that watched its own acquisition watched an entity the fact never reached.

Three properties that contract rests on, each of which used to be one edit away from silently failing:

- **The epoch identifies the LOAD, not its ordinal.** It is a session-unique salt in the high 15 bits over a
  per-instance load count in the low 16, because the arm gate decides ownership by `Epoch == _LoadEpoch` and a
  bare count made a machine that had hosted N loads read a host's load N as its own — no hold, no freeze, and a
  player walking around a world being rebuilt underneath them. Treat the value as opaque: monotonic within a
  session, never a count, never persisted. Accepted residual: 15 salt bits means two instances collide with
  p = 1/32768, ~4 orders of magnitude better than the count it replaces; more bits are cheap if that stops being
  acceptable. Fence: `Ck.Snapshot.LoadHold.LoadEpochIdentifiesTheLoadNotItsOrdinal`.
- **The option is consumed, once, from both copies.** `?CkLoad=` is struck from `FWorldContext::LastURL` (which
  every relative travel inherits its whole option array from) **and** from `LastRemoteURL` (which `reconnect`
  replays verbatim), so a finished load's epoch cannot re-arm a hold on a later map change, a reconnect, or a
  join URL. This is where and how the engine strikes its own one-shots (`Listen`, `failed`, `closed`).
  `UWorld::URL` is left intact: that is the record of how this world came up, and the arm gate reads it.
- **The client's budget covers the SERVER's whole load.** `kLoad_ClientHoldFrameCap` is the SUM of the server's
  phase caps, written as that sum rather than as a number, and it counts **rebuild twice** because an escalation
  re-arms that phase against a fresh budget. Sized like a single phase, a server that spent its teardown and
  travel budgets outlived the client watching it, and the client failed open into a half-rebuilt world reporting
  the fact as NEVER ARRIVED — a healthy slow load misreported as a broken one, in the load shape most likely to
  be slow.

**What the promise may claim.** `Promise_OnLoadComplete` = ready to resume. It does NOT mean "nothing has
simulated": construction and `DoBeginPlay` run throughout a load by `[G1-D16]`, so code there must be idempotent.

---

## Load-gate spawn quarantine

While the load hold is `Teardown`, `Rebuilding` or `Escalated`, `UCk_Utils_EntityScript_UE::Request_SpawnEntity`
suppresses any spawn that is not the loader's own reconstruction. Deliberately NARROWER than "a load is running":
`Draining` and `Converging` ADMIT spawns, because that is when payload applies drive composition and refusing there
would break the restore itself. The producer-side question — "may I seed?" — is `Get_IsRebuildInProgress`, which is
true in EVERY phase. Two predicates, two jobs — the check runs SYNCHRONOUSLY inside `Request_SpawnEntity` itself
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

Three consumer-side tools, and a gap between them worth knowing:

- **`UCk_Utils_Snapshot_UE::Get_IsRebuildInProgress(InHandle)`** — true while the load gate is held, kernel or
  escalated. This is the predicate for a construction-time SEED or SPAWN of a separately-persisted entity: while
  it is true the loader is the only legitimate creator of world population, and seeding anyway leaves the world
  holding both copies. It answers a question about WRITING, and only that — a reader polling it to time a READ
  is describing a race the quarantine already closed (Setup sees hydrated Durable state; everything else binds
  `Promise_OnHydrated`), and is one edit away from polling it in the wrong window instead.
- **`UCk_Utils_Snapshot_UE::Get_IsLoadInProgress(InHandle)`** — the wider window: true from `Request_Load` until
  `OnLoadComplete`, so it stays true through Settling after the gate has already dropped. That extra stretch is
  what covers the gap named below, which `Get_IsRebuildInProgress` by definition cannot. The hazard both of them
  guard: a construction script that unconditionally seeds a separately-persisted entity creates a SECOND copy
  beside the one the load is about to restore (children composed UNDER the seeding script are fine — replayed
  construction re-creating them is how rebuild works — but a sibling/global seed is not).
- **`Request_SpawnEntity_LoadRendezvous`** (`CkEcs/EntityScript/CkEntityScript_Utils.h:93-107`, impl
  `.cpp:238-254`) — the sanctioned call for legitimate mid-load spawns: it opens a
  `FCk_ScopedRendezvousSpawnWindow` around an ordinary `Request_SpawnEntity`, so it behaves identically when no
  load is active and passes the quarantine when one is. Reserved for spawns that carry (or will acquire during
  construction) a stable SaveKey/adopt-label the loader rendezvouses onto — never for count-driven population.
- **The gap this used to have, and what closed it.** The suppression check used to key off a single flag the load
  machine turned OFF partway through Hydrating, while `Get_IsLoadInProgress` stayed true through Settling — so a
  spawn issued in between sailed past unchallenged. C6 replaced that flag with the phase enum: the hold now runs
  `Teardown -> Rebuilding -> Escalated -> Draining -> Converging` and is only released at ready-to-resume, so there
  is no longer a window in which the load is running and the hold reads "off". What remains is a DELIBERATE
  admission rather than a gap: `Draining`/`Converging` admit spawns so payload applies can compose. A spawn issued
  there — e.g. from an EntityScript's `DoBeginPlay`, which fires only once construction/composition has
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
read through `UCk_Utils_Snapshot_UE::Promise_OnLoadComplete` instead of acting directly on the half-rebuilt world.

- **No load in progress** — the delegate fires IMMEDIATELY, synchronously, with `Result = NoLoadInProgress`, and
  the call RETURNS `ECk_Snapshot_PromiseResult::NoLoadInProgress` so a caller that cares can branch without a
  second query. It is not a failure, and it no longer claims to be one: the immediate path used to hand over a
  default-constructed report, whose `_Result` defaults to `Failed_IO`, so every consumer that branched on the
  result saw a FAILURE for a load that never happened.
- **A load IS in progress** — the delegate is queued on the SUBSYSTEM
  (`Request_AddLoadCompletePromise`) and the call returns `Bound`. One-shot by construction: `DoFinish_Load`
  swaps the list out before draining it, so it fires exactly once, on THIS load's completion, and a promise
  re-armed from inside a callback takes the immediate path (the load flag is already clear) instead of landing in
  the array being iterated.
- **Either way it fires post-settle**: the load machine's Settling phase pumps hydration to quiescence before
  `OnLoadComplete` (see "Settling waits on hydration, not just frames" above), so restored values are guaranteed
  readable by the time the callback runs.

**Why the subsystem and not an entity signal.** A load TRAVELS, and every ECS registry is world-scoped
(`UCk_EcsWorld_Subsystem_UE` is a world subsystem), so the old implementation bound
`ck::UUtils_Signal_Snapshot_OnLoadComplete` on the CALLER's world transient while the broadcast happened on the
POST-travel world's — a promise armed during a load, which is exactly when consumers reach for it, died with the
world it was armed in and nothing said so (F-U1.6). The snapshot subsystem is GameInstance-scoped and survives
the travel; a dynamic delegate is a weak UObject plus a function name, so a subscriber the travel destroyed
simply no-ops. The world-scoped signal itself STAYS and is still broadcast — it is a legitimate world-scoped
event; the promise just stopped being implemented on top of it.

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

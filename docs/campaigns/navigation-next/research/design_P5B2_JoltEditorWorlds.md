# [P5-B2] design — CkJolt in Editor and commandlet worlds (option 7a-1)

Read-only pass, 2026-09-06. Paths relative to `Plugins/CkFoundation/Source` unless absolute.
Every claim below is cited; where I could not verify, §5 says so.

---

## §1 What exists

### 1.1 The gates that stop a Jolt static world outside `Game || PIE`

1. **Both CkJolt subsystems inherit the `Game || PIE` world-type gate — neither overrides it.**
   `UCk_Jolt_Subsystem : UCk_Game_TickableWorldSubsystem_Base_UE` (`CkJolt/Public/CkJolt/Subsystem/CkJolt_Subsystem.h:35`)
   and `UCk_JoltStaticWorld_Subsystem_UE : UCk_Game_WorldSubsystem_Base_UE`
   (`CkJolt/Public/CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h:76`).
   The base gates are `CkCore/.../CkGameWorldSubsystem.cpp:70` (non-tickable) and `:190` (tickable), both
   `return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;`, consulted from
   `ShouldCreateSubsystem` (`:61`, `:173`). Neither CkJolt header declares `DoesSupportWorldType` (grep over
   both headers: zero hits). **This is the primary gate — in an Editor world neither subsystem is ever constructed.**

2. **Hard typed dependency on the runtime ECS world subsystem.**
   `CkJolt_Subsystem.cpp:485` and `CkJoltStaticWorld_Subsystem.cpp:115` both call
   `InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>()`. That class is itself a
   `UCk_Game_WorldSubsystem_Base_UE` (`CkEcs/.../CkEcsWorld_Subsystem.h:148`), so it does not exist in an
   Editor world either. The Jolt world is published into *that* registry at `CkJolt_Subsystem.cpp:664`
   (`SetContext<TSharedPtr<ck::FJoltWorld>>`), which is where every Jolt processor reads it from
   (e.g. `CkJoltStaticActor_Processor.cpp:33`). **Fixing (1) without fixing (2) yields a subsystem that
   nulls out during `Initialize`.**

3. **The static sweep runs only from `OnWorldBeginPlay`, which never fires in an Editor world.**
   `CkJoltStaticWorld_Subsystem.cpp:181-214`: the per-level `DoAdd_BodiesForLevel` loop and its summary log
   live entirely inside that override. `Initialize` (`:106-122`) only binds `LevelAddedToWorld` / `LevelRemovedFromWorld`.

4. **`FProcessor_JoltStaticActor_EndPlay` is `RuntimeOnly`** (`CkJoltStaticActor_Processor.h:27`), as are
   `FProcessor_JoltBody_*` (`CkJoltBody_Processor.h:336`), `FProcessor_JoltCharacter_*` (`:194`),
   `FProcessor_JoltConstraint_*` (`:187`). The graph builder turns a mismatched processor into a ghost node —
   edges preserved, `ForEachEntity` never fires (`CkEcs/.../CkProcessorDescriptor.h:39-46`,
   `CkProcessorGraph.cpp:84-97`).

5. **Downstream, GroundNav refuses to start a build with an unusable backend** —
   `CkGroundNav/.../CkGroundNavVolume_Processor.cpp:1417` (build) and `:1747` (repair):
   `CK_ENSURE_IF_NOT(BackendIsUsable, TEXT("GroundNav Volume [{}] cannot start a build: no physics world to read geometry from"))`.
   `Get_IsBuildCurrent` returns false for the same reason (`CkGroundNavVolume_Utils.cpp:473-477`).
   The chain is: backend `Get_IsValid` → `FCk_Jolt_QuerySession::Get_IsValid`
   (`CkGroundNav_GeometryBackend_Jolt.cpp:47-51`) → session ctor resolves
   `World->GetSubsystem<UCk_Jolt_Subsystem>()` (`CkJoltOccupancy_Session.cpp:37-47, 250-262`) → null in Editor.

### 1.2 How CkJolt's own cook already solves geometry in a commandlet — and what it does NOT solve

`UCk_JoltCook_Commandlet` (`CkJoltEditor/.../CkJoltCook_Commandlet.h:42`) loads each map with
`UEditorLoadingAndSavingUtils::LoadMap` (`CkJoltCook_Commandlet.cpp:452`) — an `EWorldType::Editor` world —
validates sublevels (`:355-430`), then calls `FCk_Jolt_WorldCooker::Cook_World[_Incremental]` (`:464`).

`Cook_World` (`CkJoltCook_WorldCooker.cpp:495-560`) **never creates a Jolt world or any UWorld subsystem.** It
calls `Request_GlobalJoltInit()` (`:508`, comment: cook vehicles have no game world), settles asset compilation
and re-runs construction scripts (`:520-528`), then walks `InWorld.GetLevels()` → `Level->Actors` and extracts
per actor into a shape cache (`:539-556`) via `ck::jolt::bake` (`CkJoltBakeExtraction.h:35-115`). Output is
cooked JPH shape blobs + an index asset.

Separately, `FCk_Jolt_CookedWorldQuery` **does** stand up a fully world-independent JPH world:
`CkJoltStaticWorld_CookedQuery.cpp:60-95` builds its own layer table, filters and `JPH::PhysicsSystem` with no
`UWorld` at all. But it exposes only occupancy / broadphase-count / segment queries
(`CkJoltStaticWorld_CookedQuery.h:78-85`) — **no triangle fetch, no body kind, no body description**, i.e. none
of the four calls the GroundNav bake needs (`CkGroundNav_GeometryBackend.h:88-140`).

**Net:** the Jolt cook proves the commandlet plumbing (map load, settle, per-actor extraction, asset write, map
selection, exclusion) is solved and reusable; it does **not** give GroundNav a geometry surface. 7B-B3 is
therefore not free — but it is cheap *if* the loaded Editor world can host a live Jolt static world (§2).

### 1.3 What the editor ECS world already provides

`UCk_EditorEcsWorld_Subsystem_UE` (`CkEcs/.../CkEcsEditor_Subsystem.h:24`) is a `UTickableWorldSubsystem` gated
to `World->WorldType == EWorldType::Editor` (`CkEcsEditor_Subsystem.cpp:45`). It owns its **own** registry and
transient entity (`:52-66`), tags it `FTag_EditorOnlyEntity` (`:81`), builds the processor graph under
`ECk_ProcessorWorldTypeContext::Editor` (`:333-337`), and ticks its schedulers every editor frame
(`:111-150`, `IsTickableInEditor() → true` at `.h:50`). It **skips all scheduler work while PIE is active**
(`:119-127`) and exposes `Get_IsEditorEcsMutationSafe()` for the same window (`:161-179`).

**No CkGroundNav processor declares `WorldTypeRequirement`** (repo-wide `RuntimeOnly` grep: zero hits under
`CkGroundNav`), so every GroundNav processor already builds into and ticks in the editor graph. That is what
PROGRESS's "everything else an editor-world volume needs already works" (`PROGRESS.md:4876-4879`) rests on.

`Request_GlobalJoltInit` is refcounted (`CkJolt_Utils.cpp:56-80`), so an editor Jolt world and a PIE Jolt world
can coexist without fighting over `JPH::Factory`.

---

## §2 The design for 7a-1

### 2.1 The opt-in setting

Add to `UCk_Jolt_ProjectSettings_UE` (`CkJolt/Public/CkJolt/Settings/CkJolt_ProjectSettings.h:78`), directly
beside `_PIEStaticWorldMode` (`:160-162`) and shaped exactly like it:

- `enum class ECk_Jolt_EditorStaticWorldMode : uint8 { Disabled, LiveExtract };` — default `Disabled`
- `UPROPERTY(Config, EditDefaultsOnly, Category = "Jolt Physics|Static World") _EditorStaticWorldMode`
- accessor `UCk_Utils_Jolt_ProjectSettings::Get_EditorStaticWorldMode()` (`:293` sibling)

**Default off**: an editor that opens a map today pays nothing. `Cooked` is deliberately *not* an editor mode
(FORK-5).

### 2.2 Which subsystems change world-type support

Both CkJolt subsystems gain a `DoesSupportWorldType` override:
`Super::DoesSupportWorldType(WorldType) || (WorldType == EWorldType::Editor && Get_EditorStaticWorldMode() != Disabled)`.
Precedent in-tree: `UCk_ObjectPooling_Subsystem` does exactly this shape
(`CkCore/.../CkObjectPooling_Subsystem.cpp:53-58`).

The ECS dependency (`CkJolt_Subsystem.cpp:485`, `CkJoltStaticWorld_Subsystem.cpp:115`) must become
world-type-aware. Add a CkEcs seam — `TryGet_RegistryForWorld(UWorld&)` / `TryGet_TransientEntityForWorld(UWorld&)`
— that dispatches Game/PIE → `UCk_EcsWorld_Subsystem_UE`, Editor → `UCk_EditorEcsWorld_Subsystem_UE`. Keep the
existing typed `InitializeDependency` on the runtime branch (it also buys init ordering); add an
`InitializeDependency<UCk_EditorEcsWorld_Subsystem_UE>` on the editor branch. The context publish at
`CkJolt_Subsystem.cpp:664` then targets whichever registry the seam returned.

**Processors: no `RuntimeOnly` flag changes in this design.** Editor-world body teardown goes through the
already-public `Request_RemoveActor` / `Request_RemoveComponent` (`CkJoltStaticWorld_Subsystem.h:112-131`) and
the `Deinitialize` drain (`.cpp:130-172`), not through `FProcessor_JoltStaticActor_EndPlay`. Flipping that
processor to `All` would also pull the async-step wait (`CkJoltStaticActor_Processor.cpp:74-80`) into a world
that never steps.

### 2.3 The sweep without `OnWorldBeginPlay`

Extract the body of `OnWorldBeginPlay` (`CkJoltStaticWorld_Subsystem.cpp:188-214`) into
`DoRun_InitialSweep(UWorld&)`; `OnWorldBeginPlay` keeps calling it, and a new public `Request_EnsureSwept()`
calls it **at most once per world** behind a `_HasSwept` flag.

Trigger: **lazy, on first geometry demand** — the editor-world GroundNav build path and the cook driver call
`Request_EnsureSwept()` before constructing the backend. Nothing sweeps a map merely because it was opened.
The already-bound `LevelAddedToWorld` handler (`:117`, `DoHandle_LevelAdded` at `:719`) keeps sublevels correct
after the first sweep, in editor as in PIE.

**Re-trigger on actor edits** (`#if WITH_EDITOR`, bound only when the setting is on):
`GEngine->OnLevelActorAdded` / `OnLevelActorDeleted` → `Request_BakeActor` / `Request_RemoveActor`;
`GEditor->OnActorMoved()` → `Request_RemoveActor` then `Request_BakeActor` (a moved static body's pose is baked
in). All three are already public and already do the right bookkeeping.

### 2.4 The revision contract — unchanged, and that is the point

`Request_BakeActor` / `Request_RemoveActor` / the removal funnel already call `Request_NoteStaticSceneChanged()`
(`CkJoltStaticWorld_Subsystem.cpp:466, 657, 1140`), which increments `FJoltWorld::_StaticSceneRevision`
(`CkJoltWorld.cpp:381-385`). `FCk_Jolt_QuerySession::Get_StaticWorldRevision` returns that token alone
(`CkJoltOccupancy_Session.cpp:584-602`) and is exactly what `ICk_GroundNav_GeometryBackend::Get_WorldRevision`
surfaces. So the editor world gets the *same* revision semantics the sliced build already fails-closed on —
**no new revision machinery, and no second contract.** This is the whole argument for 7a-1 over 7a-2.

### 2.5 What stays PIE-only

- **Simulation.** `UCk_Jolt_Subsystem::Tick` must early-return on `EWorldType::Editor`; nothing steps, no
  gravity, no contact drain. (`GetWorld()->GetGravityZ()` at `CkJolt_Subsystem.cpp:558` is still read at
  Initialize and is harmless.)
- Bodies, characters, constraints, debug-drag: all their processors stay `RuntimeOnly` (§1.1 item 4).
- The `Cooked` static-world path stays a PIE/packaged concern (`_PIEStaticWorldMode`, `:160`).

### 2.6 How 7B-B3 (the cook commandlet) sits on it

The commandlet's world is `EWorldType::Editor` (`CkJoltCook_Commandlet.cpp:452`), so the same setting turns the
Jolt world on there. A `UCk_GroundNavCook_Commandlet` in `CkGroundNavEditor` (today a bare scaffold — only
`CkGroundNavEditor_Module.*` and the log; `CkGroundNavEditor.Build.cs` already carries `UnrealEd`, `CkEcs`,
`CkGroundNav`) mirrors the Jolt one: map selection → `LoadMap` → settle → `Request_EnsureSwept()` → construct
`FCk_GroundNav_GeometryBackend_Jolt{World}` → drive the **pure bake API** directly
(`CkGroundNav_FieldBuild.h:72`, `CkGroundNav_Field.h:432/448`) → serialize with
`Field/CkGroundNav_FieldSerialize.h` → write `UCk_GroundNav_CookedTile_UE` (`Cook/CkGroundNav_CookedTile.h:95`)
and `UCk_GroundNav_CookedFieldIndex_UE` (`Cook/CkGroundNav_CookedFieldIndex.h:28`) at `Get_CookedIndexAssetPath`
(`:107`), stamping `Get_InputFingerprint` (`Bake/CkGroundNav_Fingerprint.h:123`).

**7B-B3 needs no editor ECS world and no processor tick** — no volume entity, no scheduler pump. That is
PHASE_7 §4 item 1's "one implementation, two drivers" taken literally, and it is why 7B-B3 is the *cheap* half
of [P5-B2]. Byte-identity (the 7B → 7C gate) holds by construction: same backend, same live-extract sweep, same
pure pipeline.

### 2.7 How 5D row 1 (editor snap) sits on it

The expensive half. Row 2 already landed — `CkPathNetwork_EditorUtils.cpp:46` calls
`UCk_Utils_NavSurface_UE::Try_ProjectPoint` through the facade. Row 1 needs a *published field in the editor
world*: an editor-world volume entity, its params, and `FProcessor_GroundNavVolume_*` ticking. Those processors
already build into the editor graph (§1.3), and the editor scheduler already ticks
(`CkEcsEditor_Subsystem.cpp:132-141`). What is missing is the authoring entry point that creates the volume
entity in the editor registry (`Request_SpawnEditorEntity`, `:225-256`, gated on
`Get_IsEditorEcsMutationSafe`) and a build trigger. This unit is **strictly downstream of units 1–3** and
should be sequenced last.

---

## §3 Units for an executor (numbered, disjoint file ownership)

**U1 — CkEcs world seam.** Owns: `CkEcs/Public/CkEcs/Subsystem/CkEcsWorld_Subsystem.{h,cpp}` (additive utils
only). Add `TryGet_RegistryForWorld` / `TryGet_TransientEntityForWorld` dispatching on `WorldType`.
*Done:* both worlds return a valid registry; Game/PIE behaviour byte-unchanged.
*Test pin:* `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkEcs/Test_EcsWorld_RegistryByWorldType.cpp`
(sibling naming per the existing `UnitTests/CkEcs/` folder).

**U2 — CkJolt opt-in + world-type support + tick guard.** Owns:
`CkJolt/.../Settings/CkJolt_ProjectSettings.{h,cpp}`, `CkJolt/.../Subsystem/CkJolt_Subsystem.{h,cpp}`,
`CkJolt/.../StaticWorld/CkJoltStaticWorld_Subsystem.{h,cpp}` (declaration + Initialize + Tick guard only).
*Done:* setting default `Disabled`; with it off, `World->GetSubsystem<UCk_Jolt_Subsystem>()` in an Editor world
is still null; with it on, both subsystems exist and `Tick` never steps in Editor.
*Test pin:* `UnitTests/CkJolt/Test_JoltEditorWorld_SubsystemGating.cpp` — sibling of
`UnitTests/CkJolt/Test_JoltCook_Driver.cpp`, which is already `EAutomationTestFlags::EditorContext` and guarded
by `#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS`.

**U3 — Sweep without BeginPlay + edit hooks.** Owns: `CkJoltStaticWorld_Subsystem.{h,cpp}` (the sweep refactor,
`Request_EnsureSwept`, the `#if WITH_EDITOR` delegate binds). *Sequenced after U2 — same files.*
*Done:* `Request_EnsureSwept` is idempotent; moving/adding/deleting a static actor in an Editor world changes
`Get_StaticSceneRevision`; a second `Request_EnsureSwept` does not re-add bodies.
*Test pin:* `UnitTests/CkJolt/Test_JoltEditorWorld_SweepAndRevision.cpp`.

**U4 — 7B-B3 cook commandlet.** Owns: new files only, under
`CkGroundNavEditor/Public/CkGroundNavEditor/Cook/CkGroundNavCook_{Commandlet,FieldCooker,AssetSave}.{h,cpp}`,
plus `CkGroundNavEditor.Build.cs` (add `CkJolt`, `CkSettings`). Mirrors
`CkJoltEditor/.../Cook/CkJoltCook_{Commandlet,WorldCooker,AssetSave}.*` one-for-one.
*Done:* `-run=CkGroundNavCookCommandlet` writes tile + index assets for a named map; a cooked field loaded at
runtime is byte-identical to a runtime bake of the same world (the 7B → 7C gate).
*Test pins:* C++ `UnitTests/CkGroundNav/Test_GroundNavCook_Driver.cpp` (mirrors `Test_JoltCook_Driver.cpp` —
DryRun, writes nothing) and `Test_GroundNavCook_MapSelection.cpp` (mirrors `Test_JoltCook_MapSelection.cpp`).
AS pin: `Plugins/CkTests/Script/CkGroundNav/CkAutoTest_GroundNav_Cook_CookedFieldMatchesRuntimeBake.as`
(sibling of the existing `CkAutoTest_GroundNav_Cook_MissingCookBakesAtRuntime.as` and
`CkAutoTest_GroundNav_Cook_DuplicateCookKeyIsRefused.as`).

**U5 — 5D row 1, editor-world field.** Owns: `CkGroundNav/.../Volume/CkGroundNavVolume_Utils.{h,cpp}` (editor
entry point) and a new `CkGroundNavEditor/.../Authoring/` file. *Last; do not start before U1–U3 are green.*
*Done:* with the setting on, a volume in an Editor world reaches `Built`, and `Try_ProjectPoint` answers
through GroundNav at authoring time; with the setting off, it reports `Unbuilt` (never `NoSurface`) and
PathNetwork's snap falls back exactly as today.
*Test pin:* `UnitTests/CkGroundNav/Test_GroundNav_EditorWorldField.cpp`; the human half is `[EDITOR-VERIFY]`
VALIDATION §4.3 (PHASE_5 §6 step 3) — an agent cannot perform or claim it.

---

## §4 Forks the orchestrator must rule

**FORK-1 — per-project setting or per-volume opt-in?**
*Recommend per-project*, matching `_PIEStaticWorldMode`'s shape. A per-volume flag is incoherent: the Jolt
static world is world-scoped (one `PhysicsSystem` per `UCk_Jolt_Subsystem`, `CkJolt_Subsystem.cpp:554`), so the
first opted-in volume pays the entire map's sweep anyway and the second pays nothing — a per-volume switch
would advertise a granularity that does not exist and make cost unpredictable per level.

**FORK-2 — does 7B-B3 drive the volume processor or the pure bake API?**
*Recommend the pure bake API* (§2.6). Driving the processor means pumping `UCk_EditorEcsWorld_Subsystem_UE`'s
schedulers by hand from a commandlet that never ticks; failure mode: the build slices forever, or completes
only because the cook fakes a tick loop — and the cook's output then depends on scheduler ordering rather than
on the bake pipeline.

**FORK-3 — eager sweep on map open, or lazy on first demand?**
*Recommend lazy* (`Request_EnsureSwept`). Eager makes every opened map pay a full static extraction even in
projects that never author nav; on a large map that is thousands of shape builds at open time, and the user
sees it as "the editor got slower after this change" with no way to attribute it.

**FORK-4 — incremental re-bake per edited actor, or full re-sweep on any change?**
*Recommend incremental* via the existing `Request_BakeActor` / `Request_RemoveActor`. A full re-sweep per edit
makes dragging one actor an O(map) cost and bumps the revision once per drag *tick*, which would invalidate an
in-flight sliced build continuously — the field would never converge while the user has a handle grabbed.

**FORK-5 — does the editor mode also get a `Cooked` variant (read cooked Jolt data instead of live-extracting)?**
*Recommend no, this campaign.* It would need triangle / body-kind / body-description fetch added to
`FCk_Jolt_CookedWorldQuery` (`CkJoltStaticWorld_CookedQuery.h:78-85`), which is a second geometry surface —
the thing 7a-2 was rejected for. Failure mode of saying yes: two editor geometry paths whose bakes must be
proven byte-identical to each other *and* to PIE, tripling the A9 obligation.

**FORK-6 — cost while the user edits, and does toggling the setting need a map reload?**
`ShouldCreateSubsystem` is evaluated when the world's subsystem collection is built
(`CkGameWorldSubsystem.cpp:47-71`), so flipping the setting in a running editor does **not** retroactively
create the subsystems. *Recommend accepting "takes effect on next map load"*, documented in the setting's
tooltip, over a bespoke rebuild path. Failure mode of building the rebuild path: tearing down a live
`UCk_Jolt_Subsystem` mid-session while GroundNav handles hold `FJoltWorld` shared pointers and the editor
registry holds the context (`CkJolt_Subsystem.cpp:664`) — teardown ordering nothing else in the codebase
exercises.

---

## §5 Risks, and what I could not verify

1. **Editor memory and open-time cost — NOT measured.** A live-extract sweep of a shipping map builds every
   static shape into a `JPH::PhysicsSystem` sized by `Get_MaxBodies()` (`CkJolt_Subsystem.cpp:552`). The Jolt
   cook's own comment records 4084 / 4066 / 4065 actors across three cooks of one measured map
   (`CkJoltCook_WorldCooker.cpp:518-521`), which is the right order of magnitude, but I have no editor-side
   number. **Ask for a measurement before defaulting the setting on anywhere.**

2. **PIE-from-editor double worlds — partly reasoned, not verified.** `Request_GlobalJoltInit` is refcounted
   (`CkJolt_Utils.cpp:56-80`), so two `JPH::PhysicsSystem`s coexisting is legal. The editor ECS subsystem
   already refuses to tick while PIE is up (`CkEcsEditor_Subsystem.cpp:119-127`), which covers GroundNav
   processors. But **I did not verify that an editor Jolt world holding bodies while a PIE world sweeps the
   same levels is safe** — in particular whether `LevelAddedToWorld` fires for PIE-duplicated levels into the
   editor-world handler (`CkJoltStaticWorld_Subsystem.cpp:719`). I read the binding, not the handler's world
   filter. **Verify before U3 lands.**

3. **Undo/redo — unverified.** `GEditor->OnActorMoved()` and the actor add/delete delegates fire on undo, but I
   did not confirm ordering (does a re-created actor arrive as `OnLevelActorAdded` with a registered primitive
   component?). A missed re-bake leaves stale geometry *with* a bumped revision — the worst combination,
   because the field then reads not-current forever.

4. **Hot reload — unverified.** Live-coding CkJolt while an editor Jolt world holds `JPH::Ref` shapes and the
   editor registry holds `TSharedPtr<ck::FJoltWorld>` is untested territory; the runtime path never faces a
   PIE-less reload today because the subsystem does not exist outside PIE.

5. **The `_PIEStaticWorldMode::Disabled` early-return** at `CkJoltStaticWorld_Subsystem.cpp:186-189` is
   `#if WITH_EDITOR` and keyed to the *PIE* setting. The editor path must not be gated by it; U3 must add its
   own gate rather than widening that one, or turning off PIE static world would silently kill editor authoring.

6. **`UCk_Utils_NavSurface_UE::Try_ProjectPoint`'s provider resolution in an Editor world — not traced.** The
   provider table is a static registration surface (`CkNavSurface_ProviderTable.h:93`); I did not verify how a
   provider is selected per world, so U5's "answers through GroundNav at authoring time" may need a
   provider-resolution change I have not scoped.

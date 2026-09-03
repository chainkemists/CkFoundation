# CkJoltEditor

**Purpose:** Editor-side cooker for the CkJolt static world. Extracts static collision (the SAME
`ck::jolt::bake` extraction the runtime PIE path uses — one extraction path, no divergence),
partitions actor groups into bake-grid cells, serializes deduped Jolt shape blobs
(`Shape::SaveWithChildren`), and saves the per-cell + index data assets under the configured
cooked-data root (`UCk_Jolt_ProjectSettings_UE::_CookedDataRootPath`). Also owns the per-mesh
pre-baked shape cook and the auto-cook-on-save behaviour that keeps both from going stale.

**Depends on:** `CkCore`, `CkEcs`, `CkJolt`, `CkLog`, `CkSettings`, `CkThirdParty` + `UnrealEd`,
`EditorSubsystem`, `AssetRegistry`, `ToolMenus`, `DeveloperSettings`, `SourceControl`,
`PhysicsCore`, `Slate`/`SlateCore`, `Landscape`.
**Type:** Editor (never loads in game/cooked).

---

## Two cooks, two staleness stories

They fail differently and are triggered by different edits — do not conflate them.

| Cook | Unit | Goes stale when | Runtime symptom |
|---|---|---|---|
| **Static world** (`FCk_Jolt_WorldCooker`) | one map | an actor MOVES, is added, or is deleted | `STALE cooked Jolt data for actor [X] … bodies are SKIPPED` (`CkJoltStaticWorld_Subsystem.cpp`) — cooked mode only |
| **Mesh shapes** (`FCk_Jolt_MeshShapeCooker`) | one static mesh asset | the mesh's `BodySetupGuid` / trace flag changes (re-import, collision edit) | `Cooked Jolt shape for mesh [X] is STALE` (`CkJoltMeshShape_Utils.cpp`) |

**The mesh-shape STALE and ORPHAN ensures are NOT gated on cooked mode** — only the *missing-asset*
ensure is (`Get_ExpectsCookedData()`). A pre-baked shape is consumed in LiveExtract PIE too, as a
build-cost optimization, so a drifted blob is loud on a LiveExtract project where the map-actor
stale ensure provably cannot fire. That asymmetry is deliberate: an absent asset is an expected
miss, a *present but wrong* one is always a defect.

## Key API

- `FCk_Jolt_WorldCooker::Cook_World(World, Mode)` — rebuilds every cell + the index from scratch.
- `FCk_Jolt_WorldCooker::Cook_World_Incremental(World, Mode)` — rewrites ONLY the bake-grid cells
  whose actors changed. Both full and incremental calls accept optional excluded level paths;
  incremental planning removes excluded cooked groups even from unloaded levels, while preserving
  other unloaded groups. Reports why it fell back through `FCookStats::_Outcome`.
- `FCk_Jolt_MeshShapeCooker` — `Cook_MeshShapes(Mode)` (blocking sweep) decomposed into
  `Collect_Candidates()` / `Cook_SingleMeshShape(Mesh, Mode, OutPath)` / `Report_Orphans(InUse)` so
  a caller can drive it across frames. The optional final `ForceRebuild` argument bypasses mesh
  freshness shortcuts for explicit full rebakes; existing editor callers remain incremental.
- `ck::jolt::cook::ComputeIncrementalPlan` / `ComputeIndexRemap` — the PURE halves of the
  incremental cook (which cells are dirty; how the index renumbers around them). Unit-tested by
  `Ck.Jolt.Cook.IncrementalPlan` / `Ck.Jolt.Cook.IndexRemap` in CkTests.
- `FCk_Jolt_IncrementalCookDriver` — the incremental cook as a resumable stepper
  (`Step(budget)` -> InProgress / Done / Failed / FullCookRequired). `Cook_World_Incremental` is
  this driven to completion in one call, so there is ONE implementation behind both the blocking
  and the sliced path. It holds the Jolt globals and roots the assets it loaded for its lifetime,
  because the shapes it carries between steps survive neither being released.
- `UCk_JoltCook_EditorSubsystem_UE` — the PRIMARY cook vehicle (world already booted by the
  editor): `Cook_CurrentWorld()`, `Cook_CurrentWorld_Incremental()`, `Cook_CurrentWorld_DryRun()`,
  `Validate_CurrentWorld()`, `Cook_MeshShapes()` (all blocking), plus `Request_CookMeshShapes()`
  and `Request_CookStaticWorld()` (sliced, progress notification — what the Tools menu invokes).
  BlueprintCallable for editor-utility widgets. Also owns the save hooks.
- `UCk_JoltCook_UserSettings_UE` — per-user auto-cook toggles (Editor Preferences -> CkFoundation
  -> Jolt Cook).
- `UCk_JoltCook_Commandlet` — headless cross-map + mesh-shape cook
  (`-run=Ck_JoltCook_Commandlet -Map=… | -AllMaps | -PackagingMaps | -MeshShapes [-DryRun]` —
  `-PackagingMaps -Incremental` selects only the project's ordered `MapsToCook` persistent entry
  roots; `-ForceRebuild` and the legacy no-mode invocation union those roots with UWorld metadata
  under `DirectoriesToAlwaysCook`. All forms skip packaging/Jolt/cooked-output exclusions and reject
  `bCookAll` and `-Map`/`-AllMaps` combinations before any cook;
  the FULL class token
  is required, `-run=CkJoltCook` resolves to no class; on unique-build-environment projects invoke
  the project's own `<Target>-Cmd.exe`, not the engine's `UnrealEditor-Cmd.exe`), CK_ENSUREs the
  DirectoriesToAlwaysCook ini entry. Boots worlds itself; if that
  proves flaky on WP/landscape maps, the documented pivot is a UWorldPartitionBuilder subclass.

The commandlet's `-Incremental` switch checks existing actor hashes and writes only dirty cells;
unchanged maps write neither cells nor their index. `-ForceRebuild` forces full map and mesh-shape
rebakes. The switches are mutually exclusive. Full fallback remains necessary for missing indexes,
contract drift, and World Partition maps, and keeps the same packaging exclusions. Per-map Display
logs report progress, cells written, current actors, and full fallback. These checks still load maps
and existing cells; incremental mode does not imply a metadata-only scan.

## Auto-cook on save

`UCk_JoltCook_EditorSubsystem_UE` hooks `FEditorDelegates::PostSaveWorldWithContext` (→ incremental
map cook) and `UPackage::PackageSavedWithContextEvent` (→ per-mesh shape cook), debounces them
through a timer, and drains the queue sliced across frames behind a status-bar progress
notification (the convention in `Source/EDITOR_MODULES.md` rule 5). The mesh sweep slices per mesh; the map cook slices per cooked
cell, per swept actor, and per rewritten cell. Only the FULL map cook still blocks — its World
Partition walk owns its own loop — and it is reached only on a first cook or a contract change.

Four things are load-bearing:

- **A saved SUBLEVEL cooks the PERSISTENT map.** `PostSaveWorldWithContext` hands you the sublevel's
  own `UWorld`, but the runtime resolves cooked data by `PersistentLevel->GetOutermost()`
  (`CkJoltStaticWorld_Subsystem.cpp:934`). Cooking the saved world directly would write a
  `CkJoltData/<Sublevel>/` tree that nothing ever loads.
- **Auto-cook refuses to run in commandlets, unattended/automation editors, and PIE.** The AutoTest
  populator auto-saves its level, so without that gate every test lane would cook content mid-suite
  and charge the cost to whichever test was running.
- **A cooked mesh shape must invalidate the runtime memo.** `mesh_shape_utils` caches resolved
  shapes AND negative lookups for process lifetime; `Cook_SingleMeshShape` calls
  `Invalidate_CacheForMesh` so the editor that just cooked the shape stops serving the pre-cook
  answer. Without it the fix only lands after a restart.
- **Jolt globals are held across the whole drain.** `Request_GlobalJoltInit/Shutdown` is refcounted
  and `Shutdown` at refcount 0 does `UnregisterTypes()` + deletes the Factory — letting that happen
  between frames would strand the `JPH::Ref`s the runtime shape cache holds.

Which maps are eligible is a SHARED decision and stays in the project settings
(`_CookExcludedMapPathPrefixes`, honored by both `Cook_AllMaps` and the on-save cook); whether YOU
auto-cook is per-user.

## Incremental map cook — what it does and where it declines

Diff the world's `ComputeSourceHash` per actor against the index, rewrite only the affected cells,
carry everything else over untouched.

- An actor that **moved across a cell boundary** dirties BOTH the cell it left and the one it
  joined. Dirtying only the new one leaves a ghost body at the old position.
- An actor **absent from the world** is deleted only when **its level is loaded** — its level
  package comes from the cooked group's `_SourceActorPath`. This is the whole point: a *full* cook
  run with sublevels unloaded rewrites the index from the loaded levels alone, dropping the rest,
  and a missing `ActorLookup` entry is not an error at runtime — it is "never baked", i.e. no
  collision, silently. The incremental path can only add and update.
- A dirty cell that still holds actors from an **unloaded** level restores those actors' shapes from
  the old blob and carries them into the new one. Without that the truncation just becomes
  cell-scoped.

It **falls back to a full `Cook_World`** — and says so in `_Outcome` and the log — when there is no
existing index, when the cook/Jolt version or bake-filter hash drifted (the cooked data no longer
speaks the same contract), or on a **World Partition** world.

> **World Partition is not supported incrementally.** `ForEachActorWithLoading` releases each actor
> once its batch finishes, so the actors sharing a dirty cell with a changed one cannot be revisited
> to re-extract them. Supporting it means retaining extracted bodies for the whole walk (no cheaper
> than a full cook) or a resumable streaming pass — neither is built.

## Anti-patterns

- Don't add a second extraction path — `ck::jolt::bake` is shared with the runtime deliberately.
- Don't hand-author cooked assets; only the cooker writes them (versioning + hashes).
- Orphaned cell assets from prior cooks are LOGGED, not auto-deleted (v1) — clean by hand.
- Don't call `UPackage::SavePackage` directly here — `ck::jolt::cook::Save_CookedAsset` exists
  because the raw call is FATAL-by-default against the read-only uassets an LFS-locking workspace
  produces, and it only shows up on the SECOND cook.
- Don't put the full mesh sweep on a blocking path in new code; `Request_CookMeshShapes` exists so
  the editor stays usable. `Cook_MeshShapes` stays blocking for commandlets and utility callers.

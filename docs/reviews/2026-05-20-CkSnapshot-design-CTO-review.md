# CkSnapshot — CTO Design Review

> **Workflow:** Review the brief below, then fill in the **CTO Review Response** section at the bottom of this file. Commit your changes — the design author (Saad) will pick up your notes from there.

> **Pre-implementation review.** No implementation plan has been authored yet. The artifact under review is the **design spec** — the architectural commitments before plan-writing burns effort. If you flag a blocker now, we revise the spec; if you green-light, we proceed to `superpowers:writing-plans` to author the implementation plan, then come back to you for a separate plan review.

---

## Reviewer brief

### Your role

Senior reviewer / architect. You are reviewing a **new tier-2 CkFoundation module** (`CkSnapshot`) that adds save/load to the entire ECS, plus the per-feature opt-in markings on **`CkFloatAttribute`** and **`CkInventory`** that constitute the V1 vertical slice.

Specifically:

1. Catch architectural issues that would be expensive to fix mid-implementation.
2. Catch convention/idiom mismatches against the existing CkFoundation codebase that would cause review churn later.
3. **Scrutinize the AngelScript-touching surface in particular** — the design claims AS gets persistence "for free" via three existing mechanisms (`utils_*` → C++ fragments, `FInstancedStruct` dynamic-fragment carrier, `UObject::Serialize` on `UCk_EntityScript_UE` subclasses). Saad asked this be flagged explicitly so you scrutinize whether the claim is correct and whether a fourth AS state-bearing surface has been missed.
4. Verify the V1 scope is right-sized (CkFloatAttribute + CkInventory + framework only — everything else deferred).
5. Either green-light, or list specific blocking concerns.

You're expected to **read code in the repo** — don't review the spec in isolation. Spot-check the patterns it relies on against:

- `Plugins/CkFoundation/Source/CkDynamic/Public/CkDynamic/CkDynamic_Fragment_Data.h` — the `FCk_Fragment_DynamicFragment_Data` carrier we propose to mark snapshotable.
- `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Fragment_Data.h` — the EntityScript fragment we propose to snapshot via `UObject::Serialize(ArIsSaveGame=true)`.
- `Plugins/CkFoundation/Source/CkAttribute/CLAUDE.md` — the homogeneous-feature reference; V1 must persist Min/Max/Current + modifier sub-entities (revocable + non-revocable) without confusing the per-call-revocable coalescing model.
- `Plugins/CkFoundation/Source/CkInventory/CLAUDE.md` — V1 must persist both Spatial (placement + rotation) and DataOnly (bound mode + bound limit), plus the item entities and their traits.
- `Plugins/CkFoundation/Source/CkThirdParty/Public/CkThirdParty/entt-3.15.0/src/entt/entity/snapshot.hpp` — the EnTT machinery the design integrates with. Confirm we're using it as intended.

### What's being built

> Saad's exact framing at the start of the session: *"I would like to add save/load support for our CkFoundation framework. Take the CkInventory or CkFloatAttribute (or any attribute) as the first feature to figure this out on. Before I share my thoughts, I would like you to propose what we can do to implement save/load where the burden on the dev is as little as possible and things happen 'automagically' if at all possible."*

The design is a whole-world snapshot model (every server-side ECS entity that opts in is captured) with per-fragment opt-in (`using IsSnapshotable = void;`) and per-field opt-in (UPROPERTY `meta=(SaveGame)`). EnTT 3.15's `snapshot.hpp` drives entity-topology iteration; UE's `FObjectAndNameAsStringProxyArchive` (the same one USaveGame uses internally) handles per-fragment payload bytes. Server-only — clients re-derive via the standard replication path that already exists in CkFoundation.

The V1 deliverable is the framework + tagging for `CkFloatAttribute` and `CkInventory` + 8 autotests + 1 gym. Other features get tagged later as separate per-feature CTO-reviewed tasks.

### Design spec location

[2026-05-20-CkSnapshot-design.md](../../../docs/superpowers/specs/2026-05-20-CkSnapshot-design.md) (relative path from this review file).

Absolute: `D:/Repos/CkPlugins/docs/superpowers/specs/2026-05-20-CkSnapshot-design.md`.

The spec is the load-bearing document — read it in full. This brief tells you what to scrutinize, not what's in it.

### Critical context — read before reviewing

You **must** read these (linked or referenced from the spec but worth pre-loading):

- `Plugins/CkFoundation/CLAUDE.md` — architecture principles (composition over inheritance, event-driven over timer-based, requests deferred, authority matters).
- `Plugins/CkFoundation/Source/CLAUDE.md` — full C++ rules (function formatting, ECS patterns, `CK_PROPERTY`, request structs, component lifetimes).
- `Plugins/CkFoundation/Script/CLAUDE.md` — AngelScript conventions (the design claims it needs only a new "Persistence" section to document the three surfaces).
- `Plugins/CkFoundation/Source/CkDynamic/CLAUDE.md` — the carrier we're proposing to mark snapshotable.
- The design spec linked above.

You **should** spot-check:

- `Plugins/CkFoundation/docs/reviews/2026-05-08-CkEqs-CTO-review.md` and `2026-05-08-CkNavigation-CTO-review.md` for the bar previous CTO reviews have set on the project.

### Design decisions already locked in (do NOT relitigate unless you see a real problem)

These were debated and settled during the brainstorming session before the spec was written. The spec captures them in the "Decisions locked-in" table; reproduced here for convenience:

1. **Whole-world snapshot model** — not per-owner, not per-feature-opt-in. Chosen for fit with Rewind99's sim shape (VHS store sim, ~110–130 NPCs, 4-player co-op).
2. **Opt-in via fragment-level marker** (`using IsSnapshotable = void;`) — not "tag every UPROPERTY", not "implement a SaveLoad interface per feature".
3. **Per-field opt-in via UPROPERTY `meta=(SaveGame)`** inside marked fragments — UE's existing convention; not a CkFoundation-specific mechanism.
4. **Level respawns actors; ECS re-binds via `FFragment_SaveKey` GUID** (the "Option A" choice from brainstorming) — not "save file respawns actors too", not "pluggable per-entity".
5. **Server-only save/load, "anytime, flush first" timing** — clients re-derive via replication; convergent flush of the server processor graph before snapshot.
6. **V1 feature coverage = CkFloatAttribute + CkInventory together** (not one then the other).
7. **Module name `CkSnapshot`** (rejected `CkSave`, considered `CkPersistence` and `CkArchive`).

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- Is `CkSnapshot` correctly placed at **tier 2** (`CkEcs`/`CkCore`/`CkLog`/`CkThirdParty` only)? The spec claims feature modules don't grow a dep edge to `CkSnapshot` because the marker is a free trait in `CkEcs/Concepts/` — does this story hold up, or does the `CK_REGISTER_SNAPSHOTABLE` macro inevitably force feature cpps to link `CkSnapshot`?
- Is the **two-layer USaveGame** design (wrapper USaveGame around opaque byte buffer) the right call? The rationale: USaveGame requires fixed UPROPERTY layout, our savable-fragment set is dynamic. The cost: we own our own header version-check. Acceptable trade?
- The **`FFragment_SaveKey` split-storage** (entity fragment + bridge UPROPERTY): is rendezvous-via-resolver the right pattern, or should we collapse into one storage location?
- Does the **convergent-flush** strategy compose with how CkFoundation processors already enqueue follow-up requests in handlers? Hard cap at 8 iterations — too low for any feature you can predict?

#### B. Convention compliance

- The proposed module layout (`Public/CkSnapshot/{Marker, SaveKey, Archive, Subsystem, SaveGame, Snapshot}/`) — does it match CkFoundation's tier-2 module shape, or have we drifted?
- `using IsSnapshotable = void;` — does this mirror existing tag-type-alias conventions (`MarkedDirtyBy` on processors) closely enough? Any naming we'd want different?
- Naming: `FFragment_SaveKey`, `UCk_Snapshot_Subsystem`, `UCk_Snapshot_SaveGame`, `FCk_Snapshot_LoadReport`, `FCk_Delegate_OnSaveComplete` — all per the conventions in `Plugins/CkFoundation/CLAUDE.md`?
- Public API surface (5 UFUNCTIONs + 4 signals on the subsystem) — does this fit the CkFoundation "thin Utils, deferred via signals" idiom? Any of the 5 should be deferred / re-shaped?

#### C. Version-specific API specifics

- **UE 5.5.** `FObjectAndNameAsStringProxyArchive` with `ArIsSaveGame=true` — is `SerializeItem` the right call site, or does UE 5.5 prefer a different entry point for round-tripping a USTRUCT through a proxy archive?
- **EnTT 3.15.** The spec describes `entt::snapshot` driving iteration over our registered savable types. Verify the snapshot/snapshot_loader contract has not shifted in 3.15 (the codebase notes 3.15.0, but `CkThirdParty` also ships 3.16 — confirm which is the build target).
- `FInstancedStruct` serialization via `SerializeItem` — does UE 5.5 round-trip the type-then-data layout cleanly, or do we need a custom hook?
- **`UObject::Serialize(FArchive&)` with `ArIsSaveGame=true`** for EntityScript instances — is this the right call site? Or do we need `SerializeScriptProperties`?

#### D. AngelScript surface (THE part to scrutinize most carefully)

This is the part Saad asked be flagged. The spec's claim is:

> "AS gets snapshotting for free in all three cases, because the C++-side carriers do the work and the UE-side reflection (`FInstancedStruct` + `UObject::Serialize`) is already AS-aware. No `IsSnapshotable` line in AS, no AS-specific registration, no AS-side archive code."

Three surfaces enumerated (see spec Section 1):

1. AS calling into C++ fragments via `utils_*` — covered by the C++-side marker. ✓ should be uncontroversial.
2. AS-declared dynamic fragments via `UCkDynamic_HandleDefinition` + `UCk_Utils_DynamicFragment_UE::Add_Fragment` — covered by marking `FCk_Fragment_DynamicFragment_Data` snapshotable + relying on `FInstancedStruct`'s built-in type-aware serialize. **Is this universally correct, or are there dynamic-fragment usage patterns that break this assumption?**
3. AS-authored `UCk_EntityScript_UE` subclasses — covered by `UObject::Serialize(ArIsSaveGame=true)` walking `UPROPERTY(SaveGame)` fields. **Does AngelScript-side `UPROPERTY(SaveGame)` meta actually make it into the compiled UClass the same way C++ `UPROPERTY(meta=(SaveGame))` does?** This is the load-bearing question for path #3.

**Specific things to verify or push back on:**

- Is there a **fourth AS state-bearing surface** we missed? E.g., AS-global state stored in script subsystems, AS-side caches, AS-bound multicast delegate subscriber lists, AS-instantiated UDataAssets via `asset Foo of UType {}`.
- The **AS class-rename caveat** (spec Section 1 sub-bullet at end of subsection): "If an AS class is renamed or deleted between save and load, that entity's script/fragment can't be restored. We handle the same way UE SaveGame already does: log loudly, skip the affected entity, surface in LoadReport." Is "log + skip" acceptable, or should we refuse load on any unresolved AS class?
- **`FCk_Fragment_DynamicFragment_Data` blanket-snapshotable** — does the framework's "all dynamic fragments persist by default" stance match how dynamic fragments are intended to be used, or should opt-in be per-payload-type?

#### E. Test coverage

- The 8 autotests + 1 gym + 1 legacy-save guard test (spec Section 6) — does this cover the contract well?
- Specific gaps to flag:
  - No test for the **cross-multi-frame async save callback** path (the spec implies this works because `SaveGameToSlotAsync` is async; no test exercises the callback delivery itself).
  - No test for **format-version bump migration** (no `Test_Snapshot_FormatVersionMigration_v1_to_v2`). Justifiable for V1 since we ship at format version 1 and migrations only matter on bump?
  - **`Test_Snapshot_ConvergentFlush_Cap`** (#8) — should it also assert the recovery path (after the failure, does the world remain in a consistent state for a subsequent save)?

#### F. Risks the spec calls out — sized correctly?

The spec acknowledges:

- **Pending client RPCs dropped at save time** — server doesn't see them. Marked acceptable; player gets a "saving..." dialog.
- **Convergent flush as framework health invariant** — features must not have processors that pathologically enqueue feedback loops.
- **AS class / UScriptStruct rename** requires `CoreRedirects` entry, enforced socially via CLAUDE.md.
- **Duplicate-GUID from actor duplication in editor** — detected at BeginPlay, one regenerated.
- **Transient runtime UObject refs** (`NewObject<>` with no stable path) must not have `meta=(SaveGame)`.

**Are any of these sized too small? Any sized too big (over-engineered)?**

#### G. Forward-compat with deferred work

V1 explicitly defers:

1. Per-fragment custom `Migrate(int32 FromVersion, FArchive&)` callbacks.
2. Runtime-spawned-but-savable actors.
3. Cross-feature entity references via raw handle.
4. Client-side snapshot generation.
5. Buffer compression.

**Do any of these deferrals contain hidden assumptions that V1 architecture would need to revise to add later?** Specifically: does the V1 wire format + SaveKey resolver design accommodate adding (1)/(2)/(3) as additive extensions, or would adding them later require re-versioning?

### Output format — fill in the CTO Review Response section below

Be direct. If the design is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers, not vague concerns.

---

## CTO Review Response

### Verdict

**CHANGES REQUESTED.**

The shape of the design is right (whole-world, fragment-marker opt-in, SaveKey rendezvous, server-only with replication as the client-restore path). The Section-1 AS analysis is *almost* right — path #2 (dynamic-fragment carrier) holds up, but paths #1 and #3 do not survive contact with how the live ECS fragments are actually defined in this repo. The spec is built on a serialization mechanism that doesn't compose with the runtime fragment surface it claims to cover.

The fix is local to Section 3 (the archive adapter) — once a non-USTRUCT path is specified, the rest of the architecture (SaveKey, USaveGame wrapper, convergent flush, LoadReport, orchestration) can stand. So this is a "patch the contract" not a "throw it out."

### Blocking issues

1. **`T::StaticStruct()->SerializeItem(...)` does not compile against the live fragments the spec needs to persist.** (Section 3 — *Archive adapter*.) The chosen mechanism only works for `UScriptStruct`-backed types. Every state-bearing live ECS fragment I sampled is a non-USTRUCT C++ struct in `namespace ck` with `CK_GENERATED_BODY` (which is just `using ThisType = X; CK_ENABLE_CUSTOM_VALIDATION()` per [CkMacros.h:67](../Source/CkCore/Public/CkCore/Macros/CkMacros.h)) — UHT does not generate `StaticStruct()` for these:
   - `ck::TFragment_Attribute<H, T, Component>` — `_Base/_Final` as raw `float/uint8`, no UPROPERTY. This is the V1 first-target (`FFragment_FloatAttribute_Current/Min/Max`).
   - `ck::TFragment_RecordOfEntities<H>` — `TArray<EntityType> _RecordEntries`, mutable, no UPROPERTY. This is `FFragment_RecordOfInventoryItems`, which Section 6 explicitly commits to persisting.
   - `ck::FFragment_InventoryItem` — `TWeakObjectPtr<const UCk_InventoryItem_Definition>`, no UPROPERTY. Item Definition is V1 in-scope.
   - `ck::FFragment_EntityScript_Current` — `TStrongObjectPtr<UCk_EntityScript_UE>`, no UPROPERTY. This is the path-#3 carrier.
   - `ck::FFragment_Inventory_PreviousItems`, the `TFragment_Inventory_Requests<Shape>` family, `FFragment_Inventory_Spatial_SyncReplication`, the EntityHolder fragments, etc.

   The `requires FragmentIsSnapshotable<T>` concept only checks for `typename T::IsSnapshotable;` — it does not enforce USTRUCT-ness, so callers will mark these fragments snapshotable in good faith and hit a compile error in the writer. Section 3 needs a parallel mechanism: either a per-family hand-rolled serialize trait (`SerializeSnapshot(FArchive&)` instance method, or a `TSnapshotTraits<T>` specialization), or a different reflection layer (e.g. entt::meta + a CkFoundation-side property descriptor table). Pick one and re-state the "one-line opt-in" promise honestly: it can hold for USTRUCT data fragments; it cannot hold for templated `ck::TFragment_*<...>` families without additional plumbing.

2. **Path #3 (EntityScript instance) is under-specified.** (Section 1 — *AngelScript surface*, point 3.) `FFragment_EntityScript_Current` holds `TStrongObjectPtr<UCk_EntityScript_UE> _Script` — `TStrongObjectPtr` is a non-reflected GC-rooted smart pointer (unlike `TObjectPtr`), so it can't be `UPROPERTY` and `ArIsSaveGame=true` tagged-property serialization can't see it. Also the spec says "spawn a fresh instance of the restored `TSubclassOf<>`" but is silent on **where the `UClass*` path itself is captured** — the fragment has no `TSubclassOf` field today; the only place the class lives is `_Script.Get()->GetClass()` on the live instance. The save side must explicitly write the class path; the load side must read it back and `NewObject<>` with that class before calling `Serialize`. Spec needs to state this concretely or you'll discover it mid-implementation.

3. **Spawn-params for EntityScripts are not addressed.** `FCk_Request_EntityScript_SpawnEntity` carries an `FInstancedStruct _SpawnParams` consumed in `Construct(InHandle, InSpawnParams)`. Many AS scripts use spawn params to seed runtime state into their own fields. On load the spec spawns a fresh instance and calls `Serialize` — but it does **not** re-run `Construct`, and spawn params are not stored anywhere persistent. Either (a) require AS authors to mirror all spawn-param-derived state into `UPROPERTY(SaveGame)` fields on the script (then document this contract loudly), or (b) persist the spawn-params `FInstancedStruct` alongside the class path. Pick one — silence here will bite during V1 EntityScript testing.

4. **`UPROPERTY(SaveGame)` on AngelScript-side properties is asserted, not verified.** Grepping `Plugins/CkFoundation` for `UPROPERTY.*SaveGame` returned **zero hits** — neither in C++ nor in any `.as` file. The whole "AS gets persistence for free via UPROPERTY meta" claim depends on the AS frontend translating `UPROPERTY(SaveGame) int _Score;` into a UE-reflected `CPF_SaveGame` flag on the generated `FProperty`. Before V1 ships, this needs an actual compile-and-Serialize smoke test (one AS class with one SaveGame field, round-trip through `FObjectAndNameAsStringProxyArchive`, assert the value survives). If the AS frontend silently drops the meta, path #3 is dead even after fix (2) lands. Add this to the V1 test plan **before** authoring the implementation plan.

### Non-blocking suggestions

1. **EnTT version.** Spec says 3.15.0; `Plugins/CkFoundation/Source/CkThirdParty/Public/CkThirdParty/` ships only `entt-3.16.0`. `Plugins/CkFoundation/CLAUDE.md:3` and `:376` also say 3.15 — the docs are stale in two places, not just the spec. Update the spec to 3.16 and verify `entt::snapshot` / `snapshot_loader` API against the shipped header, not docs from memory. Out-of-scope for this design but file a follow-up to fix the CLAUDE.md references.

2. **`FCk_Snapshot_Header` USTRUCT shape is referenced but never defined.** Section 3 USaveGame snippet and Section 4 capture step both stamp the header; Section 5 keys version checks off `_Header.FormatVersion`; `Get_SaveSlotHeader(...)` returns it BP-side. The implementation plan should commit to the field list (FormatVersion : uint16, EngineVersion, PluginBuildHash : FGuid, TimestampUTC, WorldName, ManifestSummary). Worth a half-page in Section 5 before plan-writing.

3. **Convergent-flush iteration cap = 8 is a magic number.** Probably fine as a starting value, but expose it as a CVar (`ck.Snapshot.ConvergentFlush.MaxIterations`) and report per-iteration dirty counts in the `Failed_NotQuiescent` log. Saves a redeploy when a feature trips it.

4. **Orphan-sweep 2-second timer should be project-overridable.** Spec already mentions "configurable post-load gate" — make it a `UCk_Snapshot_Settings : UDeveloperSettings` field, not a hard-coded literal, and have the gate accept either a duration or a named signal handle (the "OnAllPlayersReady" hook is the right escape hatch for Rewind99's 4-player co-op rejoin case).

5. **CkSnapshot at tier 2 forces a tier-1 touch.** Placing the `FragmentIsSnapshotable` concept under `CkEcs/Concepts/` is fine (you said so), but flag this as a CkEcs edit in the implementation plan so it doesn't surprise downstream consumers. Adding one header to CkEcs is a non-event; just don't bury it.

6. **`UObject::Serialize(FArchive&)` is the right call site for path #3** — not `SerializeScriptProperties`. The former dispatches to the latter internally, and your archive has `ArIsSaveGame=true` set already, so the tagged-property gating kicks in correctly. Worth a one-line note in the spec to pre-empt the question, but no change needed.

7. **`FInstancedStruct` round-tripping under `ArIsSaveGame=true`** works the way you describe (writes `UScriptStruct*` path, then serializes the inner data with the same `ArIsSaveGame` flag, which composes with per-field `meta=(SaveGame)` on the payload USTRUCT). I've used this in production. Good call.

8. **Adding the "Persistence" section to `Plugins/CkFoundation/Script/CLAUDE.md`** is the right place — that file is the authoritative AS doc. Make sure it documents the path-#3 caveat about spawn-params and the path-#3 class-path requirement once blocker (2)/(3) land.

### Convention compliance spot-checks performed

- `Plugins/CkFoundation/Source/CkDynamic/Public/CkDynamic/CkDynamic_Fragment_Data.h` — confirmed `FCk_Fragment_DynamicFragment_Data` IS a USTRUCT with `GENERATED_BODY()`, holds `FInstancedStruct _StructData`. Path #2 is sound.
- `Plugins/CkFoundation/Source/CkDynamic/Public/CkDynamic/CkDynamic_Fragment.h` — confirmed `ck::FFragment_DynamicFragment_Data` is a `using` alias to the global USTRUCT, not a separate type.
- `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Fragment.h` — confirmed `FFragment_EntityScript_Current` is non-USTRUCT, `TStrongObjectPtr` for the script instance. Drives blocker (2).
- `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Fragment_Data.h` — confirmed `FCk_Request_EntityScript_SpawnEntity` carries `FInstancedStruct _SpawnParams`. Drives blocker (3).
- `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript.h` — confirmed `UCk_EntityScript_UE` UCLASS, `_AssociatedEntity` is `UPROPERTY(Transient)` (correctly excluded from any future SaveGame round-trip).
- `Plugins/CkFoundation/Source/CkAttribute/Public/CkAttribute/CkAttribute_Fragment.h` — confirmed `TFragment_Attribute<H, T, Component>` is non-USTRUCT, raw `_Base`/`_Final` fields. Drives blocker (1).
- `Plugins/CkFoundation/Source/CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h` — confirmed the V1 `FFragment_FloatAttribute_Current/Min/Max` are typedefs of the non-USTRUCT template.
- `Plugins/CkFoundation/Source/CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h` — confirmed Params USTRUCTs are well-formed and would round-trip cleanly (no `SaveGame` meta on any field yet — that's what V1 tagging adds).
- `Plugins/CkFoundation/Source/CkInventory/Public/CkInventory/Item/CkItem_Fragment.h` — confirmed `FFragment_InventoryItem` is non-USTRUCT, `TWeakObjectPtr` for definition.
- `Plugins/CkFoundation/Source/CkInventory/Public/CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.h` — confirmed `TFragment_Inventory_Requests<FCk_Handle_Inventory_Spatial>` specialization is non-USTRUCT; Params alias `FFragment_Inventory_Spatial_Params = FCk_Fragment_Inventory_Spatial_ParamsData` IS USTRUCT.
- `Plugins/CkFoundation/Source/CkInventory/Public/CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment_Data.h` — confirmed Params USTRUCT shape; `FCk_InventoryItem_Spatial_ReplicatedEntry` carries `FIntPoint` + `ECk_CardinalRotation` per spec.
- `Plugins/CkFoundation/Source/CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h` — confirmed `TFragment_RecordOfEntities<T>` is non-USTRUCT, `TArray<EntityType>` as the mutable backing store. `RecordOfInventoryItems` inherits this.
- `Plugins/CkFoundation/Source/CkInventory/Public/CkInventory/Inventory/CkInventory_Fragment.h` — confirmed inventory-side helpers (`FFragment_Inventory_PreviousItems`, the EntityHolder fragments, the Records) are all non-USTRUCT.
- `Plugins/CkFoundation/Source/CkCore/Public/CkCore/Macros/CkMacros.h:67` — confirmed `CK_GENERATED_BODY` is just `using ThisType = X; CK_ENABLE_CUSTOM_VALIDATION()` — does **not** synthesize `StaticStruct()`.
- `Plugins/CkFoundation/CLAUDE.md` — confirmed framework principles (composition, request-deferred, authority-checks) and fragment naming patterns. Spec layout matches `_Fragment.h` vs `_Fragment_Data.h` convention correctly.
- `Plugins/CkFoundation/Script/CLAUDE.md` — confirmed AS UPROPERTY usage patterns. Found **zero** usages of `(SaveGame)` meta anywhere in `Plugins/CkFoundation/Script/`. Drives blocker (4).
- Grepped `Plugins/CkFoundation` for `UPROPERTY.*SaveGame` — zero hits. The entire mechanism is greenfield in this codebase.
- `Plugins/CkFoundation/Source/CkThirdParty/Public/CkThirdParty/` — confirmed only `entt-3.16.0/` ships; no `entt-3.15.0/`. Drives suggestion (1).

### Design / architecture observations

The Section-1 "three AS surfaces, all free" claim **partially holds**:

- **Path #1 (AS via `utils_*` → C++ fragments).** ✓ Mechanically the AS side sees no change — the marker lives on the C++ struct. But this path inherits blocker (1): if the C++ fragment behind a `utils_*` call is one of the non-USTRUCT `ck::TFragment_*<...>` families (which is most of them for V1's CkFloatAttribute), the marker is necessary but not sufficient — you also need the non-USTRUCT serialize path resolved. AS authors won't see the failure mode; the C++ side will.

- **Path #2 (dynamic fragments via `FCk_Fragment_DynamicFragment_Data`).** ✓ Sound. Carrier is a real USTRUCT, `FInstancedStruct` is type-aware and round-trips correctly under `ArIsSaveGame=true`. Per-payload-type opt-in via `meta=(SaveGame)` on the payload USTRUCT's fields is the right granularity. I'd ship this path as-is. Re the spec's Open Question 2 ("blanket-snapshotable, or opt-in per payload?") — keep it blanket-snapshotable at the **carrier** level, and let payload USTRUCTs gate themselves at the field level via `meta=(SaveGame)`. Anything more granular adds AS-author surface area for no payoff.

- **Path #3 (AS `UCk_EntityScript_UE` subclasses).** ✗ The mechanism described doesn't compose with `FFragment_EntityScript_Current`'s actual shape (blocker 2), doesn't address class-path persistence (blocker 2), doesn't address spawn-params (blocker 3), and depends on unverified AS-frontend behaviour (blocker 4). All four are addressable, but the spec needs to spell out each.

**A fourth AS-touching surface the spec missed:**

- **Long-lived signal subscriptions made outside `Construct`/`BeginPlay`.** When an AS script binds to a `CkSignal` mid-game (e.g. an inventory bound a delegate to a player-action signal during gameplay), on load the entity is wiped + restored via fresh `NewObject<>` + `Serialize`. If the binding wasn't re-issued during `BeginPlay`, it's gone — no `UPROPERTY(SaveGame)` can rescue a delegate target. This is probably the *correct* behaviour (delegates are runtime-only state), but it needs to be documented as a contract in the `Script/CLAUDE.md` Persistence section: *"Signal bindings do not survive snapshot. Re-bind in `BeginPlay`, or accept loss across save/load."* Otherwise the first time an AS author saves mid-tutorial and the UI-bound delegate stops firing on load, the report will read "save corruption" when it's actually expected behaviour.

**On the "AS class rename" caveat (Open Question 3):** log + skip + LoadReport is the right default for Rewind99. A refuse-load-on-unresolved policy is a debug-time convenience, not a shipping-build policy — players would lose access to their saves whenever a content patch retired a script. Make the strict mode an opt-in setting on `UCk_Snapshot_Settings` (e.g. `bRefuseLoadOnUnresolvedTypes : bool`), default off. Same shape as `RefuseLoadsBelowBuildHash`.

**On the V1 cross-feature handle reference deferral (Open Question 6):** Inventory's intra-feature handle refs (`FFragment_Item_ParentInventory`, `FFragment_InventorySlot_ItemRef`, the EntityHolder chain) are not cross-feature in the sense the deferral targets — they're within `CkInventory`. CkFloatAttribute's modifier-subentity refs are within `CkAttribute`. Both V1 features are fine. The first place the deferral will bite is when something like a CkAbility references an FCk_Handle_FloatAttribute on a different feature's entity tree — log a follow-up for whatever feature first crosses the line.

**On `_AssociatedEntity` being `UPROPERTY(Transient)`** in `UCk_EntityScript_UE`: this is correct and important — `Serialize` will skip it under `ArIsSaveGame=true` because both `Transient` and the missing `SaveGame` flag exclude it. On load, the post-restore code path needs to re-set `_AssociatedEntity` to the rebound `FCk_Handle` before any AS code touches `DoGet_ScriptEntity()`. Worth a sentence in the spec's restore flow.

**Convergent-flush as framework-health invariant:** I agree with the spec's framing — fail loud rather than silent. The thing to watch is whether processors that genuinely need >1 tick (e.g. multi-phase inventory operations split across requests + sync-replication queues) accidentally trip the cap during normal flow, not just under pathological feedback loops. Recommend the V1 test plan add one autotest that saves *during* a multi-frame inventory operation (`Request_AddItem` mid-flight) and asserts either clean completion or a clear `Failed_NotQuiescent` with the correct dirty fragment named. Spec's `Test_Snapshot_ConvergentFlush_Cap` (#8) tests the failure path; we also need a "real workload doesn't trip the cap" assertion.

### Sign-off conditions (to flip to GREEN-LIGHT)

1. **Section 3 specifies how non-USTRUCT live fragments serialize.** Pick a mechanism (per-family trait specialization, or an explicit `SerializeSnapshot(FArchive&)` member method that the snapshotable concept can detect via `requires`, or hand-rolled entries in `CK_REGISTER_SNAPSHOTABLE(...)`), and re-state Section 1's "one-line opt-in" honestly: USTRUCT data fragments get the one-liner; templated `ck::TFragment_*<...>` families also need the serialize method/specialization. Don't promise free coverage for the templated families.

2. **Section 1 path #3 spells out:** (a) where the `UClass*` for the EntityScript class is written/read in the snapshot, (b) what happens to the spawn-params `FInstancedStruct` across save/load (persisted, dropped-with-contract, or replayed), (c) the `_AssociatedEntity` rebind step on the restore flow.

3. **Section 6 test plan adds an AS-frontend `UPROPERTY(SaveGame)` smoke test** as a precondition for V1 — one AS class with one `UPROPERTY(SaveGame)` field, save, mutate, load, assert value restored. Must be in V1, not deferred.

4. **Section 6 test plan adds a "normal workload doesn't trip convergent flush" autotest** alongside `Test_Snapshot_ConvergentFlush_Cap`.

5. **Spec updates the entt version from 3.15.0 to 3.16.0** and the implementation plan verifies the snapshot/snapshot_loader API against the shipped header. (File a separate trivial PR fixing `Plugins/CkFoundation/CLAUDE.md:3,376` too — out of scope here.)

6. **`FCk_Snapshot_Header` USTRUCT shape is defined** in Section 5 before plan-writing.

7. **`Script/CLAUDE.md` Persistence section, when authored, documents the signal-subscription-doesn't-survive contract** as the fourth AS-touching surface.

Once (1)–(6) land in the spec, this is GREEN-LIGHT for plan authorship. (7) lands during V1 implementation, not as a spec gate. Nothing here invalidates the design's core shape — server-only whole-world snapshot, fragment-marker opt-in, SaveKey rendezvous on level reload, replication-as-client-restore. That direction is right.

---

### Reviewer

- **Name:** CTO (Claude Opus 4.7)
- **Date:** 2026-05-21

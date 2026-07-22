# Gate 3 — CkPoi meta-feature rewrite

> **Status:** ✅ Done (2026-07-22 — actual: 1 session, as estimated)
> **Depends on:** Gate 2 ✅ (2026-07-21)
> **Estimate:** 1 session — re-date at entry; record actual at exit

## Goal

After this gate: `CkPoi` owns no bespoke fragment machinery — `Add` composes `ck::FTag_Poi` +
`CkEntityTag` category (+ optional `CkLabel`), and every former fragment field lives in its new home
(`CkEntityTag` / `CkLabel` / `CkPoiDisplayDefinition` / `CkVisibleRange`). All consumers (Compass,
Minimap, EcsDebugger inspector, MapDebugger) read the new homes with **unchanged observable
behavior**, and the full sweep is green vs the Gate 2 exit baseline.

**Gate 3 / Gate 4 boundary (locked this gate):** Gate 3 = same behavior, new data homes — projectors
keep their own distance/fade math, reading range CONFIG from `VisibleRange` params. Gate 4 = the
semantic upgrade (projectors feed `Update_Distance`, consume `FTag_VisibleRange_Hidden` +
`FTag_PoiDisplayDefinition_ParentHidden` exclusion, multi-consumer scenarios, Minimap delegate-bug
fix, Player-as-Poi gym). Rationale: the old fragments die in this gate, so the projector data-source
swap cannot be deferred; the VisibleRange *state-flow* integration can be, and splitting keeps this
gate mechanically verifiable against existing test assertions.

## Entry criteria (verified 2026-07-21)

- [x] HEADs: CkFoundation `c267d1bf9`, root `085dc34`, CkTests `363b2e3`, CkGameplayDebugger `494820d`.
- [x] Gate 2 exit carries over across the doc-commit squash: post-rewrite tree verified
      content-identical to the tested tree except .md files (`git diff backup/pre-docsquash-dev dev`
      = 2 doc files), so Gate 2's build + green runs remain valid evidence.
- [x] Baseline (captured at Gate 2 exit, logs `Exit_Gate2_*.log`): Poi-pattern 44/44,
      Compass 13/13, Minimap 14/15 (1 pre-existing red `Ck_AutoTest_Minimap_Add_CreatesChild`,
      delegate-signature bug, no Poi API involvement — confirmed again this gate).
- [x] Consumer inventory complete (accessor-name sweep, per Gate 2 lesson): 3 raw-fragment consumers
      (`CkCompass_Processor.cpp:278-354`, `CkMinimap_Processor.cpp:313-387`,
      `SCkMapDebuggerWindow.cpp:861-879`), 2 Utils-only consumers (`CkInspector_Poi.cpp`,
      `CkCompassUI_MarkerWidget.cpp:76`), handle-plumbing-only sites keep compiling
      (`FCk_Handle_Poi` survives). Zero external mutators, zero external signal binds, zero external
      `FCk_RepData_Poi` references. **CkFogOfWar confirmed NOT a Poi consumer** (comment-only hit) —
      closes the PROGRESS open item.
- [x] Test inventory complete: 25 hand-written AS files call the old API (7 CkPoi, 10+gym CkCompass,
      8+gym CkMinimap). `utils_poi::Create` already absent from the old API.

## Locked design (from PROMPT.md decisions #1, #2, #7 + this gate's research)

### New CkPoi surface (complete)

- `FCk_Fragment_Poi_ParamsData`: `_Category` (FGameplayTag, essential, `Categories = "Poi.Category"`)
  + `_Label` (FGameplayTag, optional). Nothing else.
- `Add(Handle, Params)` — ensures: valid handle, `NOT Has`, Transform present, valid Category. Then:
  `InHandle.Add<ck::FTag_Poi>()` + `UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(InHandle, Category)`
  + (if `_Label` valid) `UCk_Utils_GameplayLabel_UE::Add`. Returns `Cast(InHandle)`. Mirrors
  `CkProjectile_Utils.cpp:14-32`.
- `Has`/`Cast` keyed on `ck::FTag_Poi` (plain `CK_DEFINE_ECS_TAG`, in the new `CkPoi_Fragment.h`).
- Getters kept (thin, delegating): `Get_WorldLocation(Poi)` → entity Transform location (NO
  RelativeLocation — deleted); `Get_CategoryTags(Poi)` → EntityTag `Get_AllTagsAsContainer` filtered
  to `Poi.Category` descendants.
- Native tags in `CkPoi_Utils.cpp`: keep `Tag_Poi_CategoryName` ("Poi.Category"); ADD
  `Tag_Poi_DisabledName` ("Poi.Disabled") — the enable/disable convention constant.
- **Deleted outright:** `FCk_RepData_Poi` + the persistence registrar (`CkPoi_Fragment.cpp` —
  PROMPT decision #7; EntityTag's own `Register_SaveOnly<FCk_SaveData_EntityTags>` covers migrated
  state), `FFragment_Poi_Current`, `FFragment_Poi_Requests`, `FTag_Poi_NeedsSetup`,
  `FTag_Poi_Disabled`, all 4 `FCk_Request_Poi_*`, both delegates + signals, both processors (whole
  `CkPoi_Processor.h/.cpp` files), the request-callstack registration
  (`CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(..., poi, ...)`), getters
  `Get_Category/Get_DisplayName/Get_Priority/Get_Max|MinVisibleRange/Get_OffscreenPolicy/
  Get_DisplayAsset/Get_EnableDisable/Get_StateTags`, all `Request_*`/`BindTo_*`/`UnbindFrom_*`.
- **No `Create` reintroduced** — deliberately. It was removed pre-campaign (Add/Create reshape,
  root `31f3c54`); gyms/tests already compose the ping path caller-side (bare child + Transform +
  Add). Revisit only if Gate 5 gym work wants the convenience.
- `UCk_Poi_EntityScript` survives (the save-rebuild recipe): Construct = Transform::Add + Poi::Add,
  params reshaped with the new ParamsData. `FCk_Poi_SpawnParams` unchanged in shape.
- `CkPoi.Build.cs` after: Core/CoreUObject/Engine/GameplayTags + CkCore/CkEcs/CkEcsExt/
  **CkEntityTag/CkLabel**/CkLog — **drop `CkPoiDisplayDefinition`** (the Gate 2 temp dep, this gate's
  headline) and drop CkSettings if nothing uses it (verify before dropping).
- Enable/disable convention: disabled = EntityTag `Poi.Disabled` present. Adds/removes are DEFERRED
  one pump pass and COUNTED (N disables need N enables) — semantic change from the old idempotent
  bool, documented in the rewritten `CkPoi/CLAUDE.md`.

### Consumer rewires (behavior-preserving)

Common pattern for both projectors (`DoGatherAndProjectPois`):
1. Gather view: `View<ck::FTag_Poi, TExclude<destroy×4>>` (drop the two Poi fragments + drop
   `TExclude<FTag_Poi_Disabled>`).
2. In the worker (pure reads only — ParallelFor contract): skip if
   `UCk_Utils_EntityTag_UE::Has_UsingGameplayTag(Entity, Poi.Disabled)`.
3. Category filter: `CategoryFilter.Matches(UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(Entity))`
   — single-category behavior unchanged; multi-category = any-match (improvement, free).
4. Position: entity Transform position (RelativeLocation term deleted).
5. Range cull + fade: if `UCk_Utils_VisibleRange_UE::Has(Entity)`, read
   `Get_MinRange/Get_MaxRange/Get_FadeBandCm` and keep the EXISTING inline distance/cull/fade math;
   absent → unlimited (no cull, alpha 1). No `Update_Distance` flow this gate.
6. Priority/OffscreenPolicy: `TryGet_PoiDisplayDefinition_ByConsumer(Entity, <consumer tag>)` —
   valid → `Get_Priority`/`Get_OffscreenPolicy`; invalid → defaults `0`/`Hide` (identical to the old
   field defaults, so category-only Pois keep old behavior).
7. Entry `_Category` field stays `FGameplayTag`: fill with the FIRST `Poi.Category.*` tag from
   `Get_CategoryTags` (all current content is single-category; documented limitation until a
   multi-category consumer exists).

Per-module specifics:
- **CkCompass**: declare native `Tag_PoiConsumer_Compass` ("Poi.Consumer.Compass") in
  `CkCompass_Utils.cpp`. `CkCompassUI_MarkerWidget.cpp:76` swaps `UCk_Utils_Poi_UE::Get_DisplayAsset`
  for the ByConsumer(Compass) DD resolve (invalid → null, same as unset today). Build.cs: +CkEntityTag,
  +CkVisibleRange (CkPoiDisplayDefinition already present since Gate 2).
- **CkMinimap**: declare native `Tag_PoiConsumer_Minimap` ("Poi.Consumer.Minimap") in
  `CkMinimap_Utils.cpp`. Fog cull line unchanged. Build.cs: +CkEntityTag, +CkPoiDisplayDefinition,
  +CkVisibleRange.
- **CkEcsDebugger `CkInspector_Poi.cpp`**: Has→`FTag_Poi`; rows become: category tags (container),
  label (if any), disabled state (EntityTag `Poi.Disabled`), world location. Delete rows for the
  dead getters (priority/offscreen/range/display-name/state-tags — those belong to the
  PoiDisplayDefinition/VisibleRange/EntityTag inspectors).
- **CkMapDebugger `SCkMapDebuggerWindow.cpp:861-879`**: enumerate via `View<FTag_Poi>`; row fields
  reshaped to the surviving surface (same set as the inspector).

### What still speaks the old contract (class-3 disclosure)

- Old SAVES holding `FCk_RepData_Poi` payloads: the payload type's handler no longer exists — Poi
  enable/state from pre-Gate-3 saves silently does not restore (EntityTag state saved by NEW runs
  restores fine). Accepted by PROMPT decision #7.
- Downstream (BusterBlock) BP graphs pinned to deleted `[Ck][Poi]` nodes and placed
  `UCk_Poi_EntityScript` assets with values in deleted params fields (DisplayName/Priority/ranges/
  OffscreenPolicy/RelativeLocation/DisplayAsset) — break/drop at next asset open there. To surface at
  submodule consumption time, not here.
- `Script/Generated/utils_poi.as` regenerates at next editor boot (test runs do this).

## Work items (sequenced)

1. **[C++ / Opus agent]** CkPoi module rewrite per Locked design. Pattern source:
   `CkProjectile_Utils.cpp:14-53`.
2. **[C++ / same agent]** CkCompass + CkMinimap rewire per Consumer rewires.
3. **[C++ / parallel Opus agent]** CkGameplayDebugger: inspector + map-debugger window rewires.
4. **[Fable]** Line-audit both agents' output; build via toolbox (`--build`, no editor running);
   fix LNK/compile fallout (expected: none — accessor sweep was exhaustive this time).
5. **[AS / Opus agent, AFTER build green]** Rewrite 25 test/gym files: 7 CkPoi tests → new surface
   (EntityTag deferred semantics: `WaitOneFrame` after Add/Remove before asserting; enable/disable →
   EntityTag `Poi.Disabled`; state tags → plain EntityTag tags + `OnGameplayTagUpdated` w/
   RelevantTags instead of `OnPoiStateChanged`); Compass/Minimap tests + gyms: `ParamsData` ctor
   shrinks to (Category), `Set_Priority`/`Set_OffscreenPolicy` → `utils_poi_display_definition::Add`
   with the projector's consumer tag, `Set_MaxVisibleRange` → `utils_visible_range::Add`,
   `Request_EnableDisable` → EntityTag add/remove of `Poi.Disabled`.
6. **[Fable]** Audit tests; full sweep via toolbox (`--test --discover-fresh`, patterns Poi, Compass,
   Minimap, VisibleRange, PoiDisplayDefinition); diff vs baseline with per-test rename/rewrite map.
7. **[Fable]** Docs same-commit: rewrite `CkPoi/CLAUDE.md` (new shape), update `Source/CLAUDE.md`
   rows (CkPoi/CkCompass/CkMinimap deps), `CkPoiDisplayDefinition/CLAUDE.md` + `CkVisibleRange/
   CLAUDE.md` "Used by", PROGRESS.md + PLAN.md status, this file's Status header.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect | If instead I see | Response |
|---|---|---|---|
| Toolbox build after items 1-3 | Green, no LNK | LNK2019 on a Poi symbol | A consumer the accessor sweep missed — add the direct dep/rewire, record the miss in PROGRESS |
| Rewritten Poi suite | All green; count may differ from 7 (recorded per-test map) | Category assert fails right after Add | EntityTag deferred-add timing — the test needs `WaitOneFrame` before reading, not a code fix |
| Compass 13-test sweep | 13/13, same assertions modulo API swap | Priority/offscreen tests fail with defaults | DD resolve returning invalid — check consumer-tag mismatch (native tag vs test-resolved tag string) |
| Minimap sweep | 14/15 (same 1 pre-existing red) | New reds in Fog tests | Fog tests don't touch Poi — investigate as a REAL regression, stop and root-cause |
| Old-save Poi state | Not restored (documented) | n/a — not tested this gate | n/a |

## Exit criteria — ALL land in the SAME commit set as the last work item

- [x] Old machinery greps ZERO across CkFoundation/Source + CkGameplayDebugger/Source (run 2026-07-22, post-final-edit).
- [x] CkPoi.Build.cs has NO `CkPoiDisplayDefinition` dep (deps now Core/Ecs/EcsExt/EntityTag/Label/Log; CkSettings also dropped — verified unused).
- [x] Build green (`Build_Gate3_PoiRewrite_3.log`, attempt 3 — see LNK lesson in PROGRESS). Full sweep vs baseline: Poi 44/44 (= baseline 44/44; rename `SetStateTags_ReplacesAll`→`StateTags_ViaEntityTag` mapped 1:1), Compass 13/13 (=), Minimap 14/15 (= same pre-existing red `Minimap_Add_CreatesChild`), VisibleRange 4/4 (=). Logs: `Exit_Gate3_{Poi_3,Compass,Minimap,VisibleRange}.log`. Zero regressions.
- [x] Stock-ensure self-review grep clean on all changed files (2026-07-22).
- [ ] `[EDITOR-VERIFY]` (human): new `[Ck][Poi]` node set renders (Add/Has/Cast/Get_WorldLocation/Get_CategoryTags), old nodes gone, `Make FCk_Fragment_Poi_ParamsData` shows exactly Category+Label.
- [x] Docs updated per work item 7; PROGRESS dated entry with confirmed/inferred split.

**Post-gate corrections to this plan (recorded, not relitigated):**
- "Expected: none" on LNK fallout was wrong — one round: `CkCompass` needed a direct `CkRecord` dep
  (including `CkPoiDisplayDefinition_Utils.h` pulls `CkRecord_Fragment.h`, whose record-of-extensions
  machinery instantiates the CKRECORD_API `FCk_Handle_EntityExtension` copy ctor in consumer TUs).
  First fix attempt targeted `CkEntityExtension` — wrong module; the handle is DECLARED in CkRecord
  (`CkRecord_Fragment_Data.h:14`, circular-dep workaround). Minimap already had CkRecord.
- The test-class rename wedged the AS compile: committed generated `CkTestsAssets.as` held typed
  accessors for the old wrapper class (sourced from the still-placed map actor), and the populator
  that would fix the map only runs post-compile. Recovery: hand-prune the stale generated blocks
  once + `git rm` the orphaned external-actor package (the populator cannot remove an actor whose
  class no longer loads — it never appears in the level's actor list). Until the orphan was removed,
  its automation row PASSED VACUOUSLY (45th "green" test with no code behind it) — a stale-green
  shape worth remembering.

# Gate 2 — CkPoiDisplayDefinition

> **Status:** ✅ Done (2026-07-21)
> **Depends on:** Gate 1 ✅ (re-verified this session — see Entry criteria)
> **Estimate:** 1 session — actual: 1 session (2 delegated agents + Fable audits; 3 build
> attempts — two LNK2019 rounds from name-invisible consumers of the moved types, see work item 2)

## Goal

"After this gate: `CkPoiDisplayDefinition` exists as a compiling, tested module. An entity can
carry one display definition direct-attach (`Add`) or several as child entities under its
`RecordOfPoiDisplayDefinitions` (`Create`), each keyed by a `_Consumer` tag. When the owning
entity composes `CkVisibleRange` and goes hidden, every display-definition child gains the plain
tag `FTag_PoiDisplayDefinition_ParentHidden` (and loses it when the owner is shown again) — a
child created under an already-hidden owner starts with the tag pre-applied. The
`UCk_Poi_DisplayDefinition_PDA` asset and `ECk_Poi_OffscreenPolicy` enum live in this module now;
CkPoi/CkCompass/CkMinimap still compile and their tests still pass."

## Entry criteria (all satisfied 2026-07-21 before work began)

- [x] Gate 1 exit re-verified on current HEAD (CkFoundation `29821b936`, CkTests `cafbc4f`):
      full Fable audit of all Gate-1 files this session — counted-tag Add/Remove discipline,
      0↔>0 signal gating, deferred requests, `.Complete()` chrono seed all confirmed in code;
      green evidence re-read (`Saved/Logs/BuildTest_VisibleRange.log`: Total 4 / Passed 4 /
      Failed 0). Non-blocking audit notes recorded in PROGRESS.md's Gate-2 entry.
- [x] `REFACTOR_MultiProjectorPoi.md`, `PROMPT.md`, `CkVisibleRange/CLAUDE.md` (consumer-note
      paragraph), Gate_01 read in full this session.
- [x] Reference patterns spot-checked against current code, not memory:
      child-entity + record ritual `CkTimer_Utils.cpp:80-81` (+ `CkTimer_Fragment.h:84`),
      native cross-module signal bind `CkTween_Utils.cpp:769-770`
      (`ck::UUtils_Signal_OnTimerDone::Bind<&ThisType::OnTimerDone>(Handle, Policy, PostFire)`),
      record macro variants `CkRecord_Fragment.h:99-110` / `CkRecord_Utils.h:1065-1075`,
      CoreRedirects precedent `Config/DefaultCkFoundation.ini:1-6`.
- [x] Type-move blast radius measured: `UCk_Poi_DisplayDefinition_PDA` +
      `ECk_Poi_OffscreenPolicy` referenced by CkPoi (fragment/utils), CkCompass_Processor.cpp:332,
      CkMinimap_Processor.cpp:369 (enum only, via transitive include), AS tests/gyms (by name —
      module-move-transparent), generated `utils_poi.as` (regenerates). **Zero .uasset content
      references either type** (binary grep of CkFoundation/CkTests Content) → CoreRedirects are
      insurance for downstream hosts, not a hard need here.
- [x] Baseline captured at gate entry, 2026-07-21, before any new `.as` landed (logs
      `Saved/Logs/Baseline_Gate2_{Poi,Compass,Minimap}.log`): **Poi 40/40, Compass 13/13,
      Minimap 14/15**. The 1 red — `Ck_AutoTest_Minimap_Add_CreatesChild` — is PRE-EXISTING
      (file untouched since `f87dbb1`, fails identically in an isolated re-run on unmodified
      test env: AS "Specified function is not compatible with delegate function" at its line
      ~50 `FCk_Lambda_InHandle(this, n"OnEachMinimap")` bind + a `[Server] ! Has(InHandle)`
      ensure; kills the editor, toolbox respawns). Excluded from this gate's exit diff;
      follow-up recorded for Gate 4 (Minimap rewire). Note: the `Poi` pattern's 40 also
      matches `Point*`/`JsonPointer*` names — harmless, the exit sweep uses identical
      patterns. This gate is NOT purely additive (the type move touches CkPoi), so the
      Gates-1-2-need-no-baseline note in PROGRESS.md stops applying here.

## Locked design (from PROMPT.md #3/#5 — do not relitigate)

- Fragment: `_Consumer` (FGameplayTag, essential), `_DisplayAsset`
  (`TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA>`, still opaque to gameplay), `_Priority`
  (int32), `_OffscreenPolicy` (`ECk_Poi_OffscreenPolicy`). Static config — **no Current
  fragment, no requests, no signals of its own.**
- Cascade lives HERE, not in CkVisibleRange. Separate **plain** tag
  `FTag_PoiDisplayDefinition_ParentHidden` on children (single parent = single source, no
  counting). Consumers (Gate 4) exclude BOTH `FTag_VisibleRange_Hidden` and
  `FTag_PoiDisplayDefinition_ParentHidden`.
- Never add a third vote to `FTag_VisibleRange_Hidden` from this module
  (CkVisibleRange/CLAUDE.md consumer note — hard boundary).

**Mechanism refinement (recorded, not a relitigation — behavior identical):** the cascade is
implemented as a **native signal bind** (`ck::UUtils_Signal_OnVisibleRange_HiddenChanged::
Bind<&UCk_Utils_PoiDisplayDefinition_UE::DoOnOwnerHiddenChanged>(...)`, the CkTween↔CkTimer
precedent), bound once per owner at first `Create` — NOT a polling TProcessor. The module ships
**zero processors**. PROMPT.md #3's "no processors except the cascade" is satisfied vacuously;
the cascade handler is a static Utils member.

## Work items

1. **Module scaffold** — mimic `CkVisibleRange` (Gate 1's scaffold, freshest exemplar):
   `CkPoiDisplayDefinition.Build.cs` (CkModuleRules; deps `Core, CoreUObject, Engine` +
   `CkCore, CkEcs, CkEcsExt, CkLabel, CkLog, CkRecord, CkSettings, CkVisibleRange` — Label added
   vs the design-doc table because `Create` labels children with their Consumer tag per the
   composition ritual), `_Log.h/.cpp`, `_Module.h/.cpp`, uplugin entry (Runtime/Default,
   standard platform allowlist).
2. **Type move** — `CkPoi_DisplayDefinition.h` moves verbatim to
   `CkPoiDisplayDefinition/Public/CkPoiDisplayDefinition/` (filename, class name unchanged;
   `CKPOI_API` → `CKPOIDISPLAYDEFINITION_API`). `ECk_Poi_OffscreenPolicy` + its formatter move
   out of `CkPoi_Fragment_Data.h:19-28` into the new module's `_Fragment_Data.h` (name
   unchanged). CkPoi keeps its fields until Gate 3: `CkPoi_Fragment_Data.h` includes the new
   headers; `CkPoi.Build.cs` adds a `CkPoiDisplayDefinition` dep (**temporary — removed in
   Gate 3**, noted in tier table). CoreRedirects appended to `Config/DefaultCkFoundation.ini`:
   ClassRedirect `/Script/CkPoi.Ck_Poi_DisplayDefinition_PDA` → new module, EnumRedirect
   `/Script/CkPoi.ECk_Poi_OffscreenPolicy` → new module. CkMinimap needs zero edits (enum use is
   header-only). **CORRECTED during the gate (first build attempt):** CkCompass DOES need a direct
   `CkPoiDisplayDefinition` dep — `CkCompassUI_MarkerWidget.cpp::DoResolveDisplay` links PDA
   symbols (`Get_Icon`/`Get_Tint`/`Get_SizeHint`/`LoadSynchronous`) that the pre-plan name-grep
   missed because the file reaches the PDA via `auto` from `Get_DisplayAsset()` — the type name
   never appears literally. LNK2019 ×4 on first build; fixed by declaring the direct dep
   (correct IWYU hygiene regardless of transitivity).
3. **`CkPoiDisplayDefinition_Fragment_Data.h`** — `FCk_Handle_PoiDisplayDefinition` (typesafe),
   `FCk_Fragment_PoiDisplayDefinition_ParamsData` (fields per Locked design;
   `CK_DEFINE_CONSTRUCTORS(..., _Consumer)`). Shape: `CkVisibleRange_Fragment_Data.h`.
4. **`CkPoiDisplayDefinition_Fragment.h`** — Params alias;
   `FTag_PoiDisplayDefinition_ParentHidden` (plain `CK_DEFINE_ECS_TAG`);
   `FTag_PoiDisplayDefinition_CascadeBound` (plain — bind-once guard on the OWNER);
   `CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfPoiDisplayDefinitions,
   FCk_Handle_PoiDisplayDefinition)` + `CK_DEFINE_RECORD_OF_ENTITIES_UTILS` (policy classes are
   inert post-Model-A purge; TRANSIENT documents intent: children rebuild from Construct
   recipes). Pattern: `CkTimer_Fragment.h:84`.
5. **`CkPoiDisplayDefinition_Utils.h/.cpp`** — `UCk_Utils_PoiDisplayDefinition_UE`
   (ScriptMixin on the typesafe handle, Cast/Has plumbing per `CkVisibleRange_Utils`):
   - `Add(FCk_Handle&, Params)` — direct-attach; ensures: valid handle, NOT already has,
     valid `_Consumer` tag. Adds Params fragment only.
   - `Create(FCk_Handle& InLifetimeOwner, Params) -> FCk_Handle_PoiDisplayDefinition` — child
     entity ritual mimicking `CkTimer_Utils.cpp:40-81`: `Request_CreateEntity`, label child with
     `Get_Consumer()` (`ECk_Record_LabelRequirementPolicy::Optional` on connect), Params fragment
     on child, `RecordOfPoiDisplayDefinitions_Utils::AddIfMissing(Owner)` + `Request_Connect`.
     Then: (a) bind-once cascade — if owner NOT `Has<FTag_PoiDisplayDefinition_CascadeBound>`:
     native-bind `DoOnOwnerHiddenChanged` to `OnVisibleRange_HiddenChanged` on the owner
     (`ECk_Signal_BindingPolicy::IgnorePayloadInFlight` — seeding below reads ground truth, a
     replay would double-apply; `PostFireBehavior::DoNothing`), add the guard tag. Binding works
     whether or not the owner has composed VisibleRange yet (connection fragments are
     entity-scoped, independent of the broadcasting feature — verify at compile/test, branch
     row below). (b) seed — if owner currently has VisibleRange AND `Get_IsHidden`: add
     `FTag_PoiDisplayDefinition_ParentHidden` to the new child NOW (design-doc gotcha #1: a
     child created under a hidden parent must not be briefly visible).
   - `DoOnOwnerHiddenChanged(FCk_Handle_VisibleRange InOwner, bool InIsHidden)` (static,
     signature = signal payload) — walk `RecordOfPoiDisplayDefinitions` via
     `ForEach_ValidEntry`; Has-guarded `Add`/`Remove` of `FTag_PoiDisplayDefinition_ParentHidden`
     per child (idempotent against the seed path).
   - Getters: `Get_Consumer`, `Get_Priority`, `Get_OffscreenPolicy`, `Get_DisplayAsset`,
     `Get_IsParentHidden` (Has of the plain tag), `Get_IsEffectivelyHidden` (ParentHidden OR
     (has VisibleRange AND VisibleRange::Get_IsHidden)).
   - `TryGet_PoiDisplayDefinition_ByConsumer(FCk_Handle InOwner, FGameplayTag InConsumer)` —
     direct-attach fragment checked first, then record walk; first exact match; invalid handle
     if none (house `TryGet_*` contract). This is the read API Gate 4's projectors will use.
   - NO removal API this gate (matches CkVisibleRange; VisibleRange itself has no Remove, so
     the design-doc removal-cleanup gotcha #2 is structurally unreachable today — recorded as
     a Gate-4/5 follow-up if a Remove ever appears).
6. **`CLAUDE.md`** for the module — purpose, key API, the cascade contract (which tag means
   what, who sets it), anti-patterns (don't vote on `FTag_VisibleRange_Hidden`; don't give this
   module Poi/viewer knowledge; `Add` vs `Create` decision rule).
7. **AutoTests** — `Plugins/CkTests/Script/CkPoiDisplayDefinition/` (CkTests, NOT CkFoundation —
   Gate-1 lesson). Mirror the compass tests' tag-name mechanism (`CkAutoTest_Compass_
   ClampPolicy_PinsToEdge.as:43-55`) for `Poi.Consumer.*` test tags. One class per file:
   - `CkAutoTest_PoiDisplayDefinition_AddDirectAttach` — `Add` composes on the test entity;
     getters echo params; `Has` true; no child entity appears in any record.
   - `CkAutoTest_PoiDisplayDefinition_CreateMultipleOnOneOwner` — two `Create`s with different
     `_Consumer` tags on one owner; both children valid + connected;
     `TryGet_..._ByConsumer` resolves each to the right child (and returns invalid for an
     unused consumer tag).
   - `CkAutoTest_PoiDisplayDefinition_ParentHiddenCascades` — owner composes VisibleRange
     (interval 0, MaxRange 500); two children via `Create`, ONE of which also composes its own
     VisibleRange kept in-range; drive owner out of range → BOTH children
     `Get_IsParentHidden` true (own-range in-range on the second proves parent-wins, PROMPT
     success criterion #3); drive owner back in range → both cleared.
   - `CkAutoTest_PoiDisplayDefinition_CreateUnderHiddenParentSeedsVote` — drive owner hidden
     FIRST (settle a frame), then `Create` a child → child `Get_IsParentHidden` true within one
     frame (gotcha #1 discriminator: without the seed, the child stays visible until the NEXT
     hidden transition, which never comes).
8. **Doc rows** — `Source/CLAUDE.md`: T4 row `CkPoiDisplayDefinition |
   Core,Ecs,EcsExt,Label,Log,Record,Settings,VisibleRange`, CkPoi row gains
   `PoiDisplayDefinition` (annotated "temporary until Gate 3"), decision-tree row
   ("per-consumer Poi presentation config + parent→child visibility cascade").

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Baseline test-only runs (Poi/Compass/Minimap) pre-change | All green (counts recorded) | Pre-existing reds | Record them by name; they are NOT this gate's to fix; diff at exit excludes them |
| Toolbox `--build --test --discover-fresh --test-pattern PoiDisplayDefinition` | 4 new tests pass, module compiles clean | UHT/link errors on the type move | Check API-macro rename + include order first; the move is verbatim otherwise |
| Cascade test | Both children ParentHidden on owner-hidden, cleared on shown | Children never gain the tag | The bind-before-VisibleRange-composed assumption failed — fallback (prewritten): bind at Create only when owner already has VisibleRange, and ALSO bind lazily inside `Create` for later children; if that still can't cover "VisibleRange added after last Create", pivot to a minimal polling processor (cached bool on owner) and record the pivot in PROGRESS.md |
| Seeding test | Child created under hidden parent is ParentHidden within a frame | Child visible until next transition | Seed path (5b) not running or record-connect ordering issue — check `Request_Connect` deferral vs seed read |
| Test-only re-runs of `Poi`, `Compass`, `Minimap` patterns post-build | Counts identical to baseline | Regressions | The enum/PDA move is the only suspect — check generated `utils_poi.as` churn + AS enum resolution before touching anything else |

## Exit criteria — ALL items land in the SAME commit as the last work item (per repo)

- [x] All 8 work items complete; 4 new AutoTests pass via toolbox (fresh run with
      `--discover-fresh` on this gate's final binaries: `Total: 4, Passed: 4, Failed: 0`,
      `Saved/Logs/BuildTest_Gate2_PoiDisplayDefinition.log`, 2026-07-21). The
      bind-before-VisibleRange branch resolved empirically: CreateMultipleOnOneOwner passes on
      an owner that never composes VisibleRange — no fallback needed.
- [x] Exit sweep diffed against entry baseline — zero regressions:
      Poi 40/40 → 44/44 (+4 = exactly the new tests), Compass 13/13 → 13/13,
      Minimap 14/15 → 14/15 (same single pre-existing red, `Ck_AutoTest_Minimap_Add_CreatesChild`,
      same editor-death signature; excluded per entry criteria).
      Logs: `Saved/Logs/Exit_Gate2_{Poi,Compass,Minimap}.log`.
- [x] `ck-change-control` done-checklist run (Class 2 additive + Class 3 type-move aspects):
      C++ ✓ (full editor build green after final edit), tests-as-delta ✓ (above), AS ✓ (the four
      AutoTests ARE the AS runtime exercise — they call `utils_poi_display_definition::*` in
      headless PIE), stock-ensure grep clean, old-contract sweep: external `/Script/CkPoi.*`
      path references (downstream host content/BP) are covered by the CoreRedirects; AS/C++
      reference by unchanged type NAME.
      **[EDITOR-VERIFY] (human, outstanding):** BP node checklist for the new surface — right-click
      graph → `[Ck][PoiDisplayDefinition]` nodes render (Add / Create / TryGet / getters), enum
      params show dropdowns, `Make FCk_Fragment_PoiDisplayDefinition_ParamsData` exists,
      `<AsPoiDisplayDefinition>` autocast converts a generic handle wire.
- [x] `CkPoiDisplayDefinition/CLAUDE.md` written; `Source/CLAUDE.md` rows updated (new T4 row,
      CkPoi row + temp-dep note, CkCompass row + PoiDisplayDefinition)
- [x] PLAN.md status row AND this file's Status header updated — same commit
- [x] PROGRESS.md dated entry appended: estimate-vs-actual, confirmed-vs-inferred evidence,
      deviations from this work-item list

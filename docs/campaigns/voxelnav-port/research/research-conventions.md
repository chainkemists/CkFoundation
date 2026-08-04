# HEADLINE

CkFoundation has no live module scaffold — CkTemplate and CkEcsTemplate were deleted whole in ad045415b (2026-06-09, 31 files, 821 deletions), and the doctrine explicitly replaces them with "scaffold by mimicry": copy CkTimer's quartet and rename. The empirical new-module checklist is narrow and verified against the most recent module addition (CkDialog, 51df1630a): the commit touched exactly the new Source/<Module>/ directory + CkFoundation.uplugin + Source/CLAUDE.md — nothing else in the plugin, no registry, no codegen manifest. A new module owes AngelScript nothing at creation time beyond naming its BFL UCk_Utils_<Feature>_UE (the "_UE" suffix is load-bearing: it dodges the AS suffix-strip list) and tagging it UCLASS(Meta=(ScriptMixin="FCk_Handle_X")); CkAngelscriptGenerator emits Script/Generated/utils_<feature>.as automatically at editor boot. For naming, all of CkVolumeNavigation/CkNavVolume/CkFlightNav/CkAirNav/CkSpatialNav have concrete defects (length, UE-term collision, agent-class exclusion, or direct token collision with CkSpatialQuery); CkVoxelNav is the recommendation — 10 chars sits in the median band, "Voxel" is a token no existing module uses, it is agent-agnostic across flying and swimming, and it follows the CkAStar/CkGrid precedent of naming the representation.

## 1. Source/CLAUDE.md — tier table, authoring rules, and what it does NOT own

File: `E:\Repos\CkPlugins_Other\Plugins\CkFoundation\Source\CLAUDE.md` (569 lines, self-dated 2026-07-02, submodule HEAD 7330c1bab). VERIFIED by full read.

**Tier table (Source/CLAUDE.md:109-232).** Header rule verbatim: "Tiers are semantic bands; a module may sit higher than its minimal depth, but **deps must never point to a higher band**." Deps column is Ck-only, `Ck` prefix stripped, Public+Private combined; engine modules omitted.

- **T0 — roots, no Ck deps** (:118-123): CkBuildConfig (hosts `CkModuleRules`, not in uplugin), CkSettings (not in uplugin), CkThirdParty (vendored libs), CkIskmRendererVF (engine-only VF shim, `PostConfigInit`).
- **T1 — foundation** (:127-135): CkCVar, CkCore, CkEditorTools, CkLog, CkMemory, CkPerception, CkProfile.
- **T2 — ECS core + direct-attach primitives** (:140-150): CkEcs, CkAi, CkInput, CkLabel, CkLoadingScreen, CkProvider, CkRecord, CkResourceLoader, CkTagSet, CkVariables.
- **T3 — actor bridge** (:155-157): CkActor, CkEcsExt.
- **T4 — feature modules** (:161-223): everything else runtime. **This is the tier a Nav3D port lands in.**
- **T5 — editor** (:225-231): 25 UncookedOnly + 3 Editor. "runtime code must NEVER depend on these."

**Reference deps for a nav-adjacent T4 module** (:195, :210, :174, :163):
- `CkNavigation | Core,Ecs,EcsExt,Label,Log,Record,Settings`
- `CkSpatialQuery | Core,Ecs,EcsExt,Jolt,Label,Log,Physics,Provider,Record,Settings,Shapes,ThirdParty`
- `CkCrowd | Core,Ecs,EcsExt,Label,Log,Navigation,Physics,Pmg,Projectile,Record,Settings,Shapes,SpatialQuery`
- `CkAStar | Core,Ecs,EcsExt,Log`

**Module-authoring rules — the closest thing to a new-module checklist** (Source/CLAUDE.md:246-257), quoted near-verbatim:
1. "**Scaffold by mimicry, not from the stale replacer script:** copy the smallest complete feature quartet (`CkTimer` — the root doctrine's canonical exemplar) and rename."
2. Build.cs inherits `CkModuleRules` (`Source/CkBuildConfig/CkBuildConfig.Build.cs`): C++20, unity build, per-config define matrix, auto-detected `WITH_ANGELSCRIPT_CK`. "Only CkThirdParty and CkIskmRendererVF use plain `ModuleRules` — don't add a third without cause."
3. Add the module to `CkFoundation.uplugin` with the standard Win64/Mac/Linux allowlist. LoadingPhase is `Default` unless justified ("only 3 modules deviate today").
4. "Dependency discipline: depend only on same-or-lower tiers; runtime never on T5. Editor-only deps go inside `if (Target.bBuildEditor)` (see CkGrid → CkEntitySpawner)."
5. "Ship a `Claude.md` with the module (purpose, key API, anti-patterns) and add its row here."

**Explicit removal note** (:243-244): "CkTemplate and CkEcsTemplate were removed in commit `ad045415b` (2026-06-09). Do not re-add rows for them." And (:240-242): "`CkScripts/` is NOT a module (no Build.cs) — a support dir holding maintenance scripts (`CkLfsLocks`, `CkEcsTemplateReplacer.ps1`). The latter still references the deleted CkEcsTemplate scaffold and is **stale**."

**Function-formatting/naming is NOT in this file.** Source/CLAUDE.md:549-556 ("Owned elsewhere — do not look for it here") routes function formatting, code style, `CK_PROPERTY` encapsulation, request structs, signal macros, error handling to the root `Plugins/CkFoundation/CLAUDE.md`. The porter's actual style rules live there (§ "Code style"), summarized in the next section.

**Doc drift to flag for the planner (VERIFIED):** Source/CLAUDE.md:111 claims "All **75 non-editor modules**"; `CkFoundation.uplugin` actually declares **117 module entries** today (parsed via json). The tier table is missing rows for at least CkParticles, CkLagCompensation, CkSubsystemBrowser, CkGridEditor, CkPieLayoutEditor, CkPoi(partially), CkSpline-adjacent additions. Treat the tier table as authoritative for *policy*, stale for *census*.

## 1b. Style rules a porter must follow (root CLAUDE.md, § Code style)

Source: `E:\Repos\CkPlugins_Other\Plugins\CkFoundation\CLAUDE.md`. VERIFIED against CkTimer sources.

**Function shapes.** Trailing return types everywhere EXCEPT UFUNCTION declarations (UHT rejects them; concrete return type goes on its own line). Definitions split across lines. Confirmed exemplars read on disk:
- `Source/CkTimer/Public/CkTimer/CkTimer_Utils.h:49-71` — `UFUNCTION(BlueprintCallable, Category="Ck|Utils|Timer", DisplayName="[Ck][Timer] Add New Timer")` then `static FCk_Handle_Timer` on its own line, then `Add(` with params one-per-line.
- `Source/CkTimer/Public/CkTimer/CkTimer_Processor.cpp:21-28` — `auto` / `FProcessor_Timer_Setup::` / `ForEachEntity(` / params / `-> void` each on its own line, indented.

Allman braces, 4-space indent, CRLF, `#pragma once`; includes ordered own-header → Ck module paths → engine → `*.generated.h` last. `// ----…----` (116 dashes) separator lines between top-level declarations — present in every CkTimer file.

**Validation flow (non-negotiable #3).** Hoist a side-effect-free condition to a local, `CK_ENSURE_IF_NOT(cond, TEXT("...{}"), val) {}` with an EMPTY body, then a separate ordinary `if (NOT cond) { return {}; }`. Rationale: `CK_DISABLE_ENSURE_CHECKS` compiles the macro body out. `ck::IsValid` / `ck::Is_NOT_Valid` for all validity; `NOT` macro instead of `!`.

**Naming.** Members `_PascalCase` (no `m_`); params `In*`/`Out*`; locals PascalCase; no `b` bool prefix. `Get_` getters, `TryGet_` may-fail, `Request_` mutators, `Do*` private helpers, `INTERNAL__` BP plumbing. No UFUNCTION overloads — disambiguate with `_ByName`, `_ByTag`, `_Simple`, `AddOrReplace`. Enums over bool options.

**ECS two-tier naming table** (root CLAUDE.md, "ECS naming is two-tier"):
| Thing | Name | Lives in |
|---|---|---|
| Reflected config | `FCk_Fragment_[Feature]_ParamsData` (USTRUCT) | `X_Fragment_Data.h` |
| Runtime fragment | `ck::FFragment_[Feature]_[Type]` (plain C++) | `X_Fragment.h` |
| Bridge alias | `using FFragment_X_Params = FCk_Fragment_X_ParamsData;` | `X_Fragment.h` |
| Tag | `ck::FTag_[Feature]_[Purpose]` via `CK_DEFINE_ECS_TAG` | `X_Fragment.h` |
| Typesafe handle | `FCk_Handle_[Feature]` | `X_Fragment_Data.h` — **NEVER** `X_Fragment.h` |
| Request | `FCk_Request_[Feature]_[Action] : FCk_Request_Base` | `X_Fragment_Data.h` |
| Processor | `ck::FProcessor_[Feature]_[Phase]` | `X_Processor.h/.cpp` |
| Utils | `UCk_Utils_[Feature]_UE` | `X_Utils.h/.cpp` |

Processor phase vocabulary (observed census): `Setup`, `HandleRequests`, `EndPlay`, `Replicate`, `Update`, `ReplicateOnRestore`, `FireSignals`, `SyncReplication`, `RecomputeAll`, `MinMaxClamp`, `ComputeAll`, `Exit`, `Destructor`. **`Teardown` is explicitly NOT house vocabulary.**

**Unity-build trap (matters for a fresh module):** "No anonymous namespaces and no file-local `static` helpers — unity builds concatenate TUs and collide them. Use a filename-derived named namespace (`namespace ck_timer_processor`)." VERIFIED in `CkTimer_Fragment.cpp:19` → `namespace ck_timer_fragment`.

**Other hard rules:** `MoveTemp` never `std::move`; `auto` aggressively; `{}` construction except UFUNCTION parameter defaults which use `()`; typesafe conversion via `UCk_Utils_X_UE::CastChecked`/`::Cast`, **NEVER** `ck::StaticCast<FCk_Handle_X>` (Source/CLAUDE.md:276-327 has the full rule + table + the equally-banned base-reference aliasing); no *what*-comments; logging via `ck::<feature>::Verbose(TEXT("... [{}]"), Val)` fmt-style, never `%s`.

## 2. Template modules — GONE, do not look for them

**VERIFIED absent:** `ls Source/CkEcsTemplate` and `ls Source/CkTemplate` both return "No such file or directory" on the current worktree.

**Removal commit:** `ad045415b` — "chore: Remove deprecated CkTemplate and CkEcsTemplate modules", 31 files changed, 821 deletions. Reconstructed file lists from `git show --stat`:

`Source/CkEcsTemplate/` (real content, 821 of the deletions were mostly here):
- `CkEcsTemplate.Build.cs` (30 lines), `CkEcsTemplate.md` (41), `Claude.md` (13)
- `CkEcsTemplate_Log.{h,cpp}` (16/12), `CkEcsTemplate_Module.{h,cpp}` (13/15)
- `Public/CkEcsTemplate/CkEcsTemplate_Fragment.{h,cpp}` (88/9)
- `Public/CkEcsTemplate/CkEcsTemplate_Fragment_Data.{h,cpp}` (73/3)
- `Public/CkEcsTemplate/CkEcsTemplate_Processor.{h,cpp}` (107/109)
- `Public/CkEcsTemplate/CkEcsTemplate_Utils.{h,cpp}` (74/58)

So yes — it WAS a copy-me scaffold carrying the full quartet, and its shape is exactly CkTimer's shape.

`Source/CkTemplate/` was a **hollow** scaffold: all eight quartet files were **0 bytes** (`CkTemplate_Fragment.{h,cpp}`, `_Fragment_Data.{h,cpp}`, `_Processor.{h,cpp}`, `_Utils.{h,cpp}` all show `| 0`). Only its Build.cs (27), .md (26), Claude.md (31), _Log (16/12), _Module (13/15) had content. That hollowness is presumably why it was deleted.

**Successor mechanism (VERIFIED, two independent statements):** Source/CLAUDE.md:248 — "Scaffold by mimicry, not from the stale replacer script: copy the smallest complete feature quartet (`CkTimer`) and rename." Source/CLAUDE.md:51 also redirects the "entity presets / archetypes" use case: "CkTemplate/CkEcsTemplate were REMOVED (`ad045415b`); these are the successors (INFERRED)".

**Booby trap:** `Source/CkScripts/CkEcsTemplateReplacer.ps1` still exists and still references the deleted scaffold. Source/CLAUDE.md:242 marks it stale. **Do not run it.**

## 3. The canonical quartet — CkTimer file set and macro surface

**Full file set (VERIFIED via `find Source/CkTimer -type f`, 14 files):**
```
Source/CkTimer/CkTimer.Build.cs
Source/CkTimer/CkTimer_Log.{h,cpp}
Source/CkTimer/CkTimer_Module.{h,cpp}
Source/CkTimer/Claude.md
Source/CkTimer/Public/CkTimer/CkTimer_Fragment.{h,cpp}
Source/CkTimer/Public/CkTimer/CkTimer_Fragment_Data.{h,cpp}
Source/CkTimer/Public/CkTimer/CkTimer_Processor.{h,cpp}
Source/CkTimer/Public/CkTimer/CkTimer_Utils.{h,cpp}
```
**Layout facts a porter must copy:** there is **no `Private/` folder**. Both headers AND .cpp files live under `Public/<Module>/`. Module-level files (`.Build.cs`, `_Log`, `_Module`, `Claude.md`) sit flat at `Source/<Module>/`. Confirmed identical in CkVisibleRange (12 files, same shape) and CkNavigation.

**Multi-feature variant (relevant to a nav port):** `CkNavigation` subdivides `Public/CkNavigation/` into feature subfolders — `Nav/` (the quartet: `CkNav_Algorithm.{h,cpp}`, `CkNav_Fragment.{h,cpp}`, `CkNav_Fragment_Data.{h,cpp}`, `CkNav_Processor.{h,cpp}`), plus `NavAreaMarkup/`, `Settings/`, `Utils/`. Note the file prefix inside the subfolder is the **feature** name (`CkNav_*`), not the module name. This is the precedent to follow for a module with >1 feature.

**Build.cs shape (`Source/CkTimer/CkTimer.Build.cs`, VERIFIED in full):**
```csharp
using System.IO;
using UnrealBuildTool;

public class CkTimer : CkModuleRules
{
    public CkTimer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags",
            "CkCore", "CkEcs", "CkEcsExt", "CkLabel", "CkLog",
            "CkProfile", "CkRecord",
        });
    }
}
```
Everything is `PublicDependencyModuleNames`; no `PrivateDependencyModuleNames` block. `CkModuleRules` (`Source/CkBuildConfig/CkBuildConfig.Build.cs`, VERIFIED) already supplies: `bUseUnity=true`, `CppStandard=Cpp20`, `PCHUsage=UseExplicitOrSharedPCHs`, `SetupIrisSupport(Target)`, base deps `ApplicationCore/Core/CoreUObject/Engine`, `WITH_ANGELSCRIPT_CK` auto-detection, and the whole `CK_DISABLE_*` / `CK_BUILD_*` per-configuration define matrix.

**_Log pair (boilerplate, 1:1 renameable):**
```cpp
// CkTimer_Log.h
#include "CkCore/Log/CkLog.h"
CKTIMER_API DECLARE_LOG_CATEGORY_EXTERN(CkTimer, Log, All);
namespace ck::timer { CK_DEFINE_LOG_FUNCTIONS(CkTimer); }
// CkTimer_Log.cpp
DEFINE_LOG_CATEGORY(CkTimer);
namespace ck::timer { CK_REGISTER_LOG_FUNCTIONS(CkTimer); }
```
**_Module pair:** trivial `FCkTimerModule : IModuleInterface` with empty `StartupModule`/`ShutdownModule`, `#define LOCTEXT_NAMESPACE "FCkTimerModule"`, and `IMPLEMENT_MODULE(FCkTimerModule, CkTimer)`.

**Macro surface, per file (all VERIFIED by grep/read):**

`CkTimer_Fragment_Data.h` — `CK_DEFINE_CUSTOM_FORMATTER_ENUM` ×5 (one per UENUM, :28/40/51/62/73); `CK_GENERATED_BODY` ×4; `CK_PROPERTY` / `CK_PROPERTY_GET` ×15; `CK_DEFINE_CONSTRUCTORS` ×6; `CK_REQUEST_DEFINE_DEBUG_NAME` ×4 (one per request struct); the typesafe handle one-liner at :108-109:
```cpp
struct CKTIMER_API FCk_Handle_Timer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Timer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Timer);
```

`CkTimer_Fragment.h` — `CK_DEFINE_ECS_TAG(FTag_Timer_NeedsSetup / _NeedsUpdate / _Countdown)`; `using FFragment_Timer_Params = FCk_Fragment_Timer_ParamsData;`; fragment structs with `CK_GENERATED_BODY` + explicit `friend class FProcessor_*` list + private `_Members` + `CK_PROPERTY_GET`; a `_Requests` fragment whose `RequestType` is a `std::variant<...>` and `RequestList = TArray<RequestType>`; `CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOfTimers, FCk_Handle_Timer)`; `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerX, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time)` ×8; `CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Timer_Requests)`.

`CkTimer_Fragment.cpp` — `CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKTIMER_API, timer, ck::FFragment_Timer_Requests)`; **the persistence registration** as a file-scope `struct FRegistrar { FRegistrar() { FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_Timer>({ .Produce = ..., .HydrationApply = ... }); } }; const FRegistrar GCkTimerSaveDataRegistrar;` inside `namespace ck_timer_fragment`. Note `CK_REGISTER_SNAPSHOTABLE` is **REMOVED** (root CLAUDE.md macro table: "Model-A purge, 2026-07-13"); this designated-init handler is the replacement.

`CkTimer_Processor.h` — processors derive `ck_exp::TProcessor<Self, HandleType, ck::TReadOnly<...>, ck::TReadWrite<...>, Tag, TExclude<...>, CK_IGNORE_PENDING_KILL>`; each declares `using Group = FGroup_Gameplay_TimeDelta;`, optional `using RunAfter = TDepList<FProcessor_Timer_Setup>;`, `using MarkedDirtyBy = ...;`, `using TProcessor::TProcessor;`, then `ForEachEntity(TimeType InDeltaT, HandleType InX, ...) -> void`.

`CkTimer_Processor.cpp` — **`CK_REGISTER_PROCESSOR` at the very top of the .cpp, one line per processor, before any namespace**:
```cpp
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Update_Countdown);
```

`CkTimer_Utils.h` — `UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Timer_CategoryName)`; then
```cpp
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Timer"))
class CKTIMER_API UCk_Utils_Timer_UE : public UCk_Utils_Ecs_Base_UE
{ GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_Utils_Timer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Timer);
private:
    struct RecordOfTimers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfTimers> {};
public:
    friend class UCk_Utils_Ecs_Base_UE;
    ...
```

**ProcessorInjector — CURRENT STATE: RETIRED. VERIFIED.** `find Source -iname '*ProcessorInjector*'` → zero directories/files. `rg --no-ignore -c 'ProcessorInjector' Source` → **zero code hits**; only 5 stale markdown files mention it: `Source/CkAudio/Public/CkAudio-tasks.md` (7), `Source/CkAudio/Public/CkAudio-spec.md` (2), `Source/CkStateMachine/CkStateMachine_Architecture.md` (1), **`Source/CkNavigation/Plan/Gate_01_Pathfinding.md` (2)**, **`Source/CkNavigation/Plan/Gate_00_Foundation.md` (2)**. Root CLAUDE.md states it directly: "the old ProcessorInjector mechanism is retired; any doc mentioning it is stale." ⚠️ **The two hits inside CkNavigation/Plan/ matter for a Nav3D port** — if the porter mines CkNavigation's gate docs as a template, they will copy a retired mechanism. `CK_REGISTER_PROCESSOR` appears in 159 files repo-wide.

## 4. Third-party vendoring — CkThirdParty layout, and where libmorton / a ported nav-core belong

**Layout (VERIFIED).** `Source/CkThirdParty/` contains only 4 top-level entries: `CkThirdParty.build.cs` (note the **lowercase `b`** — unique in the repo, UBT tolerates it), `CkThirdParty_Module.{h,cpp}`, `Claude.md`, `Public/`. There is **no `Private/`**. All libs live one-folder-deep at `Public/CkThirdParty/<lib>/`, each an essentially **verbatim upstream tree** — ctti carries its `.buckconfig`, `BUCK`, `buckaroo.json`, `CMakeLists.txt`, `.travis.yml`, `conanfile.py`; cleantype carries `appveyor.yml`, `binder/`, `resources/*.png`. Nothing is restructured to Ck conventions.

**Vendored set (7):** `entt-3.16.0/`, `JoltPhysics/`, `fmt/`, `cleantype/`, `ctti/`, `delegate/`, `bitwise-enum/`.

**Licenses.** Each lib keeps its own upstream license at its own root; there is **no aggregated NOTICE/THIRDPARTY file**. VERIFIED 7-for-7: `bitwise-enum/LICENSE`, `cleantype/LICENSE.md`, `ctti/LICENSE.md`, `delegate/LICENSE`, `entt-3.16.0/LICENSE`, `fmt/LICENSE.rst`, `JoltPhysics/LICENSE`.

**Build.cs exposure (`CkThirdParty.build.cs`, VERIFIED in full).** Plain `ModuleRules` (NOT `CkModuleRules`) — one of only two sanctioned exceptions. Key lines:
```csharp
public class CkThirdParty : ModuleRules {
  PCHUsage = UseExplicitOrSharedPCHs;  CppStandard = Cpp20;
  PublicIncludePaths.AddRange(new[] {
    Path.Combine(ModuleDirectory, "Public/CkThirdParty/entt-3.16.0/src/"),
    Path.Combine(ModuleDirectory, "Public/CkThirdParty/ctti/include"),
    Path.Combine(ModuleDirectory, "Public/CkThirdParty/cleantype/src/include"),
    Path.Combine(ModuleDirectory, "Public/CkThirdParty/JoltPhysics"),
    Path.Combine(ModuleDirectory, "Public/CkThirdParty/delegate/include")});
  IWYUSupport = IWYUSupport.None;
  PublicDefinitions.Add("JPH_ENABLE_ASSERTS"); PublicDefinitions.Add("JPH_DEBUG_RENDERER");
  // Target.Type == Server → no shared-lib defines; else JPH_SHARED_LIBRARY + private JPH_BUILD_SHARED_LIBRARY
  bUseUnity = false;
}
```
Note `fmt/` and `bitwise-enum/` get **no** `PublicIncludePaths` entry — they are reached via relative include from the CkCore wrapper. So adding an include-path entry is per-lib judgement, not automatic.

**Usage rules (`Source/CkThirdParty/Claude.md`, VERIFIED):**
1. "Never include ThirdParty headers directly from game code. Include the CkCore wrappers (`Format/CkFormat.h`, `Entt/Entt.h`)."
4. Jolt: "the world is owned by `CkJolt`; the sanctioned direct `JPH::` consumers are `CkJolt`, `CkSpatialQuery` (Probe internals), and `CkEqs`. Don't add new direct Jolt includes outside those." — i.e. **direct use is permitted but must be an explicitly enumerated allowlist entry in this doc.**

**Where libmorton goes (RECOMMENDATION, grounded in the above):**
- Tree: `Source/CkThirdParty/Public/CkThirdParty/libmorton/` — verbatim upstream checkout, MIT `LICENSE` preserved at its root (matches all 7 existing libs).
- `CkThirdParty.build.cs`: add `Path.Combine(ModuleDirectory, "Public/CkThirdParty/libmorton/include")` to `PublicIncludePaths` (libmorton is header-only, so no `PublicAdditionalLibraries` / no binary staging; `bUseUnity=false` on this module already protects against header collisions).
- `Source/CkThirdParty/Claude.md`: add a row to the **Vendored libraries** table (`libmorton/ | libmorton | Morton/Z-order encode-decode for SVO node keys`), and add a **usage rule** naming the new nav module as the sole sanctioned direct consumer — exactly the Jolt precedent (rule 4). Without that row the next audit will flag it as an unsanctioned include.
- The new nav module's `.Build.cs` then adds `"CkThirdParty"` to `PublicDependencyModuleNames` (precedent: CkSpatialQuery, CkEqs, CkSnapshot, CkUI, CkPerception all do).

**Where a ported nav-core library goes — the split (INFERRED from the CkThirdParty/CkJolt precedent, but strongly evidenced):** CkThirdParty holds **verbatim upstream trees only** (every one of the 7 retains upstream build files it does not use). If the nav-core is *ported* — rewritten into `ck::` namespaces, Ck types, `CK_ENSURE_IF_NOT`, house formatting — it is **not third-party anymore** and belongs inside the new module at `Source/<Module>/Public/<Module>/<Feature>/`, mirroring how `CkNavigation` wraps engine Recast without vendoring anything, and how `CkJolt` owns the *wrapper* while the Jolt *tree* lives in CkThirdParty. Rule of thumb for the plan: **verbatim → CkThirdParty; rewritten → the module.** A hybrid (vendor verbatim, wrap in the module) is also precedented — that is exactly the Jolt/CkJolt shape.

## 5. NAMING — full module census, candidate evaluation, recommendation

**ALL 122 directories under `Source/` (VERIFIED via `ls -1`, includes 3 stray .md files at the end):**
```
CkAStar CkActor CkActorRelay CkAggro CkAi CkAngelscriptGenerator CkAnimation
CkAnimationEditor CkAssetExporter CkAttribute CkAttributeEditor CkAudio
CkAudioEditor CkBuildConfig CkCVar CkCVarEditor CkCamera CkChaos CkCompass
CkCompositeAlgos CkConsoleCommands CkCore CkCoreEditor CkCrowd CkCue CkCueEditor
CkDataViewer CkDialog CkDynamic CkDynamicEditor CkEcs CkEcsEditor CkEcsExt
CkEcsExtEditor CkEditorGraph CkEditorStyle CkEditorToolbar CkEditorTools
CkEntityCollection CkEntityExtension CkEntitySpawner CkEntitySpawnerEditor
CkEntityTag CkEqs CkFx CkGameSession CkGoap CkGraphics CkGrid CkGridEditor
CkInput CkInsightsAnalyzer CkInteraction CkInventory CkInventoryEditor
CkIskmRenderer CkIskmRendererVF CkIsmRenderer CkJolt CkJoltEditor CkK2Nodes
CkLabel CkLagCompensation CkLoadingScreen CkLog CkLogEditor CkMemory CkMessaging
CkMinimap CkNavigation CkObjective CkObjectiveEditor CkOverlapBody
CkOverlapBodyEditor CkParticles CkParticlesEditor CkPathNetwork
CkPathNetworkEditor CkPerception CkPhysics CkPieLayoutEditor CkPmg CkPoi
CkPoiDisplayDefinition CkProfile CkProjectile CkProvider CkRaySense CkRecord
CkRelationship CkRenderTarget CkResolver CkResourceLoader CkResourceLoaderEditor
CkScripts CkSettings CkShapes CkSnapshot CkSpatialQuery CkSpline CkStateMachine
CkSubstep CkSubsystemBrowser CkTagSet CkTargeting CkThirdParty CkTimer CkTween
CkUI CkUIEditor CkUnrealComponent CkUsf CkUsfEditor CkVariables CkVat CkVatEditor
CkVfx CkVfxEditor CkVisibleRange CkWatermark
(+ CLAUDE.md, DEBUG_CALLSTACK_PROGRESS.md, EDITOR_MODULES.md, USING_WITH_CLAUDE.md)
```
**Length distribution (non-editor, VERIFIED):** 4 chars (CkAi, CkFx, CkUI) → 22 (CkPoiDisplayDefinition). Dense band is **7–14**; median ≈ 10. Sample: CkTimer=7, CkDialog=8, CkPhysics=9, CkSnapshot=10, CkTargeting=11, CkNavigation=12, CkPathNetwork=13, CkSpatialQuery/CkVisibleRange=14.

**Token collision scan (VERIFIED, `ls Source | grep -iE 'voxel|octree|volum|space|air|fl(y|ight)|nav|3d|spatial'`):** only two hits — **CkNavigation** and **CkSpatialQuery**. `Voxel`, `Octree`, `Volume`, `Space`, `Air`, `Flight` are **all unused tokens today.**

**Log-namespace census (VERIFIED, 100+ namespaces from every `*_Log.h`).** House form is a lowercase concatenation, sometimes abbreviated: `ck::nav` (CkNavigation — note it already claimed the short `nav`), `ck::spatialquery`, `ck::pathnetwork`, `ck::visiblerange`, `ck::astar`, `ck::crowd`, `ck::sm` (CkStateMachine), `ck::lag_comp`. **`ck::nav` is taken** — a new nav module must pick a distinct one.

**Candidate evaluation:**
| Candidate | Len | Verdict |
|---|---|---|
| CkVolumeNavigation | 18 | ✗ Wordiest option; "Navigation" duplicates CkNavigation wholesale — two modules whose names differ only by a qualifier is the exact confusion to avoid. "Volume" also reads as UE `AVolume`/`ANavMeshBoundsVolume`. |
| CkNavVolume | 11 | ✗ Length fine, semantics bad. "Nav volume" is *already* an established UE term for a bounds-volume actor (`ANavMeshBoundsVolume`, nav modifier volumes). Readers will assume it's authoring tooling for CkNavigation, not a separate solver. Sorts adjacent to CkNavigation in `ls`, compounding it. |
| **CkVoxelNav** | **10** | **✓ Best.** Median length. "Voxel" is unclaimed and unambiguously names the SVO representation. Agent-agnostic. Precedent for naming the mechanism: CkAStar, CkGrid, CkIsmRenderer. |
| CkFlightNav | 11 | ✗ Excludes swimming agents, which are in scope. Name would be a lie on day one. |
| CkAirNav | 8 | ✗ Same defect, worse — "Air" also evokes aviation/atmosphere sim. |
| CkSpatialNav | 12 | ✗ **Hard collision.** "Spatial" is owned by CkSpatialQuery (Jolt broadphase/probes). Readers will assume a dependency or a split of that module. Rejects on the token scan alone. |
| CkOctreeNav | 11 | ~ Acceptable but brittle: names the *specific* structure, so a later swap to a hierarchical grid or flowfield makes the name lie. "Voxel" survives that swap; "Octree" does not. |
| CkNavSpace | 10 | ~ Length good, no collision, but "Space" is vague (space partitioning? outer space? namespace?) and it leads with "Nav", re-triggering the CkNavigation adjacency. |

**My own proposals:**
- **CkVolumeNav** (11) — shorter cousin of CkVolumeNavigation; "volumetric navigation" is the honest description and is agent-agnostic. Only defect is residual `AVolume` ambiguity. Solid #2.
- **CkVolumetricNav** (15) — maximally precise, no collision, agent-agnostic; costs 5 chars over CkVoxelNav and sits at the CkVisibleRange/CkSpatialQuery end of the length band (still legal — CkPoiDisplayDefinition is 22).
- **CkVoxelPath** (11) — pairs with CkAStar/CkPathNetwork by naming the *output* (paths) rather than the query surface. Weaker than CkVoxelNav because the module will also own volume generation/serialization, not just pathing.

**RECOMMENDATION: `CkVoxelNav`.**
Rationale, in priority order: (1) zero token collision — `Voxel` appears nowhere in the 122-dir census, so nothing in `ls` or in a grep reads as related to CkNavigation or CkSpatialQuery; (2) 10 chars lands exactly on the median of the 7–14 band; (3) `Voxel` is the single word that separates it from CkNavigation's surface/Recast domain in one glance, which `Volume`/`Space`/`Nav*` do not; (4) agent-agnostic — unlike Flight/Air it does not exclude swimming; (5) naming the representation is established house precedent (CkAStar names an algorithm, CkGrid a structure, CkIsmRenderer a backend); (6) it does not contain "Nav3D", satisfying the hard constraint.

Derived identifiers, following house convention: log category `CkVoxelNav`, namespace **`ck::voxelnav`** (free — modeled on `ck::spatialquery`/`ck::visiblerange`/`ck::pathnetwork`), API macro `CKVOXELNAV_API`, utils `UCk_Utils_VoxelNav_UE`, handle `FCk_Handle_VoxelNav`, processors `ck::FProcessor_VoxelNav_<Phase>`, module class `FCkVoxelNavModule`, feature-file prefix inside subfolders `CkVoxelNav_*` (or a shorter feature prefix per the CkNavigation `CkNav_*` precedent if it subdivides).

**Residual risk to state in the plan:** CkVoxelNav, CkNavigation, CkPathNetwork, CkAStar, CkCrowd and CkSpatialQuery now form a 6-module navigation cluster. Whatever name is picked, the new module's `Claude.md` must open with an explicit "vs CkNavigation / vs CkSpatialQuery" boundary paragraph — CkVisibleRange's doc does exactly this (Source/CLAUDE.md:222: "deliberately minimal — no Poi/consumer knowledge; see its Claude.md") and it is the reason that module's scope stayed clean.

## 6. Campaign / plan docs — where they live, what CkNavigation did

**Two coexisting conventions, both VERIFIED on disk.**

**(A) Module-local `Source/<Module>/Plan/` — what CkNavigation did.** Full listing of `Source/CkNavigation/`:
```
CkNavigation/CLAUDE.md                        <- the per-module doc (uppercase variant)
CkNavigation/PLAN.md                          <- executive index, YAML front-matter
CkNavigation/CONTINUATION_PROMPT_DiagnosticGym.md
CkNavigation/Plan/Gate_00_Foundation.md
CkNavigation/Plan/Gate_01_Pathfinding.md
CkNavigation/Plan/Gate_02_Locomotion.md
CkNavigation/Plan/Gate_03_Separation.md
CkNavigation/Plan/Gate_03_Separation_Addendum.md
CkNavigation/Plan/Gate_03_Separation_Hybrid_Plan.md
CkNavigation/Plan/Gate_04_Doorways_Replan.md
CkNavigation/Plan/Gate_05_PlayerProxy.md
CkNavigation/Plan/Gate_06_StressTuning.md
CkNavigation/Plan/Gate_07_RentalStore.md
CkNavigation/Plan/Gym_Authoring_Cheatsheet.md
CkNavigation/Plan/CkCrowdDebugger_Claude.md
CkNavigation/Plan/Debugger_Mockup/{index,01_main,02_idle,03_filter,04_health_fail}.html + style.css
```
`PLAN.md` head (VERIFIED) carries YAML front-matter `title / status: in_progress / window: 8 days / last_updated: 2026-04-29`, then "## Why this exists", "This file is the **executive index** ... each gate has its own file under [Plan/](Plan/). Update the status table here as gates land; do not bloat this file.", then a "## Scope decisions (locked)" decision table (Decision | Choice | Why) and a "## Module layout" ASCII tree. **This is the closest structural precedent for a Nav3D port** — same domain, same "new sibling module(s) in CkFoundation + one in CkGameplayDebugger" shape, and it notably plans an accompanying **debugger module with HTML UX mockups** before writing code.

Only one other module uses this: `Source/CkVat/Plan/{Gate_00_Foundation, Gate_01_Bake, Gate_02_Material, Gate_03_Playback}.md`. Both use `Gate_NN_Name.md`.

**(B) Plugin-root `docs/campaigns/<slug>/` — the newer, skill-blessed convention.** `Plugins/CkFoundation/docs/` contains `campaigns/`, `reviews/`, `specs/`. Existing campaigns: `iskm-editor-preview`, `object-pooling-core`, `request-completion-delegates`, `saveload-rebuild-hydrate`, `saveload-v3-ergonomics`, `saveload-v3-parity`, `vefects-porting`. File sets (VERIFIED):
- `request-completion-delegates/`: `PROMPT.md`, `PROGRESS.md`, `GATE_00_Infrastructure.md`, `FEATURE_CENSUS.md`, `VALIDATION.md`
- `saveload-rebuild-hydrate/`: `PROMPT.md`, `PROGRESS.md`, `PHASE_0.md` … `PHASE_5.md` (+ `PHASE_1_RESEARCH.md`, `PHASE_3A/3B`, `PHASE_4A/4B`), `CONTINUATION_PROMPT_*.md` ×6, `DIGEST.md`, `FINALIZE.md`, `VALIDATION.md`

So the newer set is `PROMPT.md` + `PROGRESS.md` + `PHASE_N.md`|`GATE_NN_*.md` + `VALIDATION.md`, with `CONTINUATION_PROMPT_*.md` spun off per session. Root CLAUDE.md confirms: "Long tasks use the phase-gate system (PROMPT.md / Gate_N.md / living PROGRESS.md) — templates and triggers in `ck-methodology` (the owner of the doc-set naming)."

**Also observed:** ad-hoc single-file design/continuation docs at module root, e.g. `Source/CkEcs/DESIGN_SubInstancedCadenceProcessors.md`, `Source/CkEcs/CONTINUATION_PROMPT_ReplicatedFragmentDispatch.md`, `Source/CkDynamic/CONTINUATION_PROMPT_DynamicFragmentSnapshot.md`, `Source/CkActorRelay/CONTINUATION_PROMPT_RelayLazySpawn.md`, `Source/CkPoi/REFACTOR_MultiProjectorPoi.md`.

**Recommendation for the plan:** put the campaign at `Plugins/CkFoundation/docs/campaigns/<module-slug>/` (the newer, skill-owned convention with 7 live examples vs 2 for the module-local form), and load the `ck-methodology` skill for the canonical templates — it is named as the owner of the doc-set naming. If the port is small enough to be one module's concern, `Source/<Module>/Plan/Gate_NN_*.md` is still legitimate precedent. **Do not** mine CkNavigation's gate docs for mechanism — two of them teach the retired ProcessorInjector (§3).

## 7. AngelScript — what a new module owes at creation time

Source: `Plugins/CkFoundation/Script/CLAUDE.md` (793 lines, written as an `.as`-style comment block, 23 numbered sections). Relevant extracts VERIFIED by read.

**Bottom line: a new module owes AS almost nothing structurally — bindings are automatic — but three naming/decl decisions at creation time are load-bearing and fail silently if wrong.**

**1. Bindings are auto-generated; do not hand-write `utils_*`.** Script/CLAUDE.md §5: "FCkAngelscriptWrapperGenerator emits **268 generated namespaces at editor boot** (`Script/Generated/utils_<feature>.as`, one per `UCk_Utils_<Feature>_UE`); hand-written `Script/CkUtils_*.as` files merge extra sugar into the same namespaces." So declaring `UCk_Utils_VoxelNav_UE` is *sufficient* to get `utils_voxel_nav::` in AS. `Script/Generated/` currently holds `_index.as` + ~260 `utils_*.as` files. **§22 item 4: "NEVER blanket-delete `Script/Generated/`."**

**2. The `_UE` suffix is mandatory, not decoration.** §16.1 (GOTCHA): the AS plugin auto-strips suffixes `Statics / Library / BlueprintLibrary / BlueprintFunctionLibrary / FunctionLibrary` and prefixes `UKismet / UBlueprint` from BFL class names when forming the AS namespace (`AngelscriptSettings.h:126-139`). A BFL named `UMyFeature_FunctionLibrary` silently becomes namespace `"UMyFeature_"` and every callsite fails with the misleading `"No matching signatures to ..."`. "This is why every Ck BFL ends `_UE` — stick to it for any new BFL exposed to AS." Escape hatch if ever needed: `UCLASS(meta = (ScriptName = "..."))`.

**3. `ScriptMixin` on the utils UCLASS.** VERIFIED at `CkTimer_Utils.h:33`: `UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Timer"))`. §5 explains the consequence: "a function whose first param type matches its class's ScriptMixin target binds as a **HANDLE MEMBER only** — the static form does not even resolve for it; the `utils_*` wrapper works uniformly." So the mixin declaration decides the AS call shape for the whole module.

**4. Request-completion delegate placement (root CLAUDE.md, hard AS constraint).** Every deferred `Request_*` ends with `const FCk_Delegate_Request_OnCompleted& InDelegate` carrying `meta = (AutoCreateRefTerm = "InDelegate")` and **no C++ default** (UHT cannot parse a delegate default). CkAngelscriptGenerator emits the `= FCk_Delegate_Request_OnCompleted()` default into the generated wrapper. "**The delegate is always the LAST parameter**" — AS permits defaults only on *trailing* parameters, so moving it earlier breaks AS and silently rebinds positional callers.

**5. No `Script/` folder is required.** VERIFIED: `Plugins/CkFoundation/Script/` has only **three** per-module subfolders — `CkChaos/` (1 file), `CkUsf/` (10 files), `CkVat/` (1 file) — all of which exist solely to host `asset ... of ...` **asset definitions** (`CkUsf_*Looks_Assets.as`, `CkVat_Looks_Assets.as`, `CkChaos_FracturedDestructible_Base.as`). CkTimer, CkNavigation, CkSpatialQuery, CkCrowd etc. ship **no** `Script/` folder at all. A new module adds one only if it needs authored AS assets or hand-written sugar; otherwise the flat `Script/CkUtils_<Feature>.as` sugar file is the alternative (26 exist, e.g. `CkUtils_Timer.as`).

**6. Typesafe handles: C++-declared handles need nothing extra.** The `DynamicHandleTypes.json` registry gotcha (§7 — "CRITICAL GOTCHA", causes project-wide `Identifier 'FCk_Handle_X' is not a data type` at engine startup) applies **only** to handles declared from AS via `asset <X>Handle of UCkDynamic_HandleDefinition`. A C++ module declaring `FCk_Handle_VoxelNav` via `CK_GENERATED_BODY_HANDLE_TYPESAFE` in its `_Fragment_Data.h` is not affected. Worth recording as a scope-limiting fact so the porter doesn't chase it.

**7. Three-environments non-negotiable.** Root CLAUDE.md #4: "Every public API must work — and be verified — in C++, Blueprint, AND AngelScript. 'Works in C++' is one third of done." Per Script/CLAUDE.md §5, AS callers must use `utils_voxel_nav::Foo(...)`, never `UCk_Utils_VoxelNav_UE::Foo(...)`.

Load the `ck-angelscript-interop` skill (CkFoundation-scoped variant available) before any binding work.

## 8. Empirical new-module checklist (derived from the most recent real addition)

**Evidence:** `CkDialog` is the newest runtime module (Source/CLAUDE.md:111, "CkDialog added 2026-07-23"). Its introducing commit is `51df1630a` — "feat(CkDialog): world-scoped dialogue-line registry queried by event tag via deferred emitters". `git show --name-only` filtered to paths **outside** `Source/CkDialog/` returns exactly **two** files:
```
CkFoundation.uplugin
Source/CLAUDE.md
```
That is the whole cross-cutting footprint. **VERIFIED:** no registry file, no codegen manifest, no `.ini`, no CkTests/CkGameplayDebugger edit, no `Script/` change was needed to land a new runtime module.

**Resulting checklist:**
1. `Source/<Module>/<Module>.Build.cs` — `public class <Module> : CkModuleRules`, all deps in `PublicDependencyModuleNames`, `Core/CoreUObject/Engine` first, then Ck deps alphabetical, blank-line separated. Tier discipline: same-or-lower band only; editor-only deps inside `if (Target.bBuildEditor)`.
2. `Source/<Module>/<Module>_Module.{h,cpp}` — empty `IModuleInterface` + `IMPLEMENT_MODULE(F<Module>Module, <Module>)`.
3. `Source/<Module>/<Module>_Log.{h,cpp}` — `DECLARE_LOG_CATEGORY_EXTERN` + `namespace ck::<ns> { CK_DEFINE_LOG_FUNCTIONS(<Module>); }` / `CK_REGISTER_LOG_FUNCTIONS`. Pick a namespace not in the ~100-entry census (§5).
4. `Source/<Module>/Public/<Module>/<Feature>_Fragment_Data.h` + `_Fragment.h/.cpp` + `_Processor.h/.cpp` + `_Utils.h/.cpp` — copied from CkTimer and renamed. **No `Private/` folder; .cpp files live under `Public/`.** Subdivide into feature subfolders (CkNavigation `Nav/`, `Settings/`, `Utils/`) if >1 feature.
5. `CK_REGISTER_PROCESSOR(ck::FProcessor_<Feature>_<Phase>);` at the top of the processor .cpp, one per processor, before any namespace. (Not ProcessorInjector — retired.)
6. `Source/<Module>/Claude.md` — purpose / depends-on / used-by / Public API / anti-patterns. Note the casing is inconsistent in the wild (`Claude.md` ~70, `CLAUDE.md` ~20); CkNavigation, CkAStar, CkAggro, CkCompass, CkGoap, CkEntityTag use uppercase.
7. `CkFoundation.uplugin` — append `{"Name": "<Module>", "Type": "Runtime", "LoadingPhase": "Default", "WhitelistPlatforms": ["Win64", "Mac", "Linux"]}` (exact shape VERIFIED for CkTimer/CkNavigation/CkDialog/CkVat/CkVisibleRange/CkSpatialQuery — all identical).
8. `Source/CLAUDE.md` — add the T4 row to the tier table AND a row to the "I need to..." decision table at :25-107.
9. If vendoring: `Source/CkThirdParty/` tree + `PublicIncludePaths` entry + `Claude.md` table row + usage-rule allowlist entry (§4).

**INFERRED, not verified:** a *consumer* (game project, CkTests, CkGameplayDebugger) additionally needs the module in its own `.Build.cs` deps; and a module that adds project settings would touch `Config/DefaultCkFoundation.ini`. CkDialog needed neither.

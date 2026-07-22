# CkCore

**Purpose:** Foundation module. Language-level helpers, Unreal Engine wrappers, data utilities, math, time, debug, and AngelScript glue. Every other CkFoundation module depends on CkCore.

**Depends on:** `CkBuildConfig`, `CkLog`, `CkSettings`, `CkThirdParty` (nothing else Ck-related).
**Used by:** effectively all ~85 other modules.

**This file is the entry point.** If you don't know where a utility lives, read the use-case table below first. Each of the 49 subfolders has a `README.md` — full details for the non-trivial ones (16), or a short stub for the rest (33).

---

## What CkCore is NOT

Before reaching for CkCore, check these:

| You want… | Go to… |
|---|---|
| ECS entities, handles, processors, fragments | `CkEcs` |
| Actor ↔ Entity bridging | `CkActor`, `CkEcsExt` |
| Save/load for entities | `CkRecord` |
| Logging | `CkLog` (CkCore wraps the public log macros but the log impl lives in CkLog) |
| Profiling macros | `CkProfile` |
| Memory tracking | `CkMemory` |
| Timers (entity-scoped) | `CkTimer` |
| Console variables | `CkCVar` |
| Data providers | `CkProvider` |
| Gameplay feature modules (attributes, animation, audio, VFX, inventory, …) | their own module |

CkCore is the layer beneath ECS. It does not know about entities, processors, or any CkFoundation-specific concept that isn't pure utility.

---

## Use-case lookup table

Scan the left column. Each row points at the folder that owns the utility and (when useful) the specific header and a one-liner.

### Language / core macros

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `CK_PROPERTY`, `CK_PROPERTY_GET`, `CK_DEFINE_CONSTRUCTORS`, `CK_GENERATED_BODY`, `NOT`, `COMMA`, `CK_PURE_VIRTUAL`, operator boilerplate | `Macros` | `CkMacros.h` | `CK_PROPERTY_GET(_Volume);` |
| Build-time feature flags (`CK_DISABLE_ENSURE_CHECKS`, `CKCORE_API` style) | `Build` | `CkBuild_Macros.h` | guards around ensure/debug |
| C++20 concepts used across CkFoundation | `Concepts` | `CkConcepts.h` | constraints for templates |
| `type_traits::AsArray`, `AsString`, `ExtractValueType`, `Const/NonConst` | `TypeTraits` | `CkTypeTraits.h` | `ck::type_traits::ExtractValueType<T>::type` |
| Variadic tuple payload for signal / delegate arguments | `Payload` | `CkPayload.h` | `ck::MakePayload(Arg1, Arg2)` |
| Step-pipeline return type (`Continue`/`Abort`) | `Technique` | `CkTechnique.h` | `ck::EStepResult::Continue` |
| Inline `std::visit` overload helper, `ToTransform`, dereference helper | `Algorithms` | `CkAlgorithms.h` | `ck::algo::Overload{[](A){}, [](B){}}` |
| Comparators for common Unreal types (GameplayTag exact match, etc.) | `Functional` | `CkFunctional.h` | `ck::comparators::GameplayTag_MatchesTagExact` |
| `TPtrWrapper<T>` (const-correctness wrapper around a pointer) | `Types` | `CkPtrWrapper.h` | member wrapping a `UObject*` |
| Type-erased struct dispatch | `StructTypeSelector` | `CkStructTypeSelector.h` | user-configurable struct picker |
| `FCk_SharedBool` and friends (wrap primitives in a `USTRUCT` for BP/AS) | `SharedValues` | `CkSharedValues.h` | shared flag passed by reference |

### Validation, ensures, error handling

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `ck::IsValid(X)`, `ck::Is_NOT_Valid(X)`, custom validators (`CK_DEFINE_CUSTOM_IS_VALID*`) | `Validation` | `CkIsValid.h`, `CkIsValid_Defaults.h` | `if (ck::IsValid(Handle)) { … }` |
| `CK_ENSURE`, `CK_ENSURE_IF_NOT`, `CK_TRIGGER_ENSURE`, `CK_INVALID_ENUM`, `CK_ENSURE_VALID_IF_NOT`, `CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT` | `Ensure` | `CkEnsure.h` | `CK_ENSURE_IF_NOT(ck::IsValid(H), TEXT("Bad handle")) { return; }` |

**Rule:** Use `ck::IsValid` (not UE's `IsValid`) for handles, custom types, and anything with a custom validator. Use `CK_ENSURE_IF_NOT` when a precondition fails at runtime and you need an early-out with a diagnostic.

### Formatting, logging adapters, debugging

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `ck::Format(TEXT("{}"), Value)`, `ck::Format_UE`, `ck::Format_ANSI` (fmt-based wide-char format) | `Format` | `CkFormat.h`, `CkFormat_Defaults.h` | `ck::Format(TEXT("Count={}"), N)` |
| Stack trace helpers, debug name verbosity enum, debug utils for printing | `Debug` | `CkDebug_Utils.h` | diagnostic formatting |
| Draw debug lines / progress bars / ASCII visualization | `Debug` | `CkDebugDraw_Utils.h`, `CkDebugDraw_Subsystem.h` | `UCk_Utils_DebugDraw_UE` |
| Log category includes (the CkCore-side wrapper of CkLog) | `Log` | `CkLog.h` | `ck::core::Verbose(TEXT("…"))` |

### Unreal Engine wrappers

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `AActor` helpers (spawn, find, cast safely) | `Actor` | `CkActor_Utils.h` | `UCk_Utils_Actor_UE::…` |
| `ACharacter` helpers | `Character` | `CkCharacter_Utils.h` | `UCk_Utils_Character_UE::…` |
| `UActorComponent` helpers | `Component` | `CkActorComponent_Utils.h` | `UCk_Utils_ActorComponent_UE::…` |
| Custom `UGameEngine`/`UGameInstance`/`AGameMode`/`AGameState` bases | `Engine` | `CkGameEngine.h`, `CkGameInstance.h`, `CkGameMode.h`, `CkGameState.h` | inherit in project settings |
| `UObject` helpers, `UCk_WorldContextObject` base | `Object` | `CkObject_Utils.h`, `CkWorldContextObject.h` | world-aware UObject |
| Pool/recycle UObjects; subsystem-own a UObject's lifetime (so a fragment can hold it weakly) | `ObjectPooling` | `CkObjectPooling_Subsystem.h`, `CkObject_Utils.h` | see `ObjectPooling/README.md` |
| Iris-compatible replicated `UObject` base | `ObjectReplication` | `CkReplicatedObject.h` | `UCk_ReplicatedObject_UE` |
| Scene / world helpers | `Scene`, `World` | `CkScene_Utils.h`, `CkWorld_Utils.h` | `UCk_Utils_World_UE::…` |
| Level streaming helpers | `LevelStreaming` | `CkLevelStreaming_Utils.h` | load/unload level streaming |
| `UGameWorldSubsystem` base class | `Subsystems` | `GameWorldSubsytem/…` | register a world subsystem |
| Game-mode-agnostic game state utils | `Game` | `CkGame_Utils.h` | `UCk_Utils_Game_UE::…` |
| Mesh helpers | `Mesh` | `CkMesh_Utils.h` | static/skeletal mesh ops |
| `FMessageDialog` wrappers | `MessageDialog` | `CkMessageDialog_Utils.h` | editor prompts |
| Editor-only utilities (guarded by `WITH_EDITOR`) | `EditorOnly` | `CkEditorOnly_Utils.h` | asset open, selection, etc. |

### Data

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `UDataTable` helpers (row lookup, struct extraction) | `DataTable` | `CkDataTable_Utils.h` | `UCk_Utils_DataTable_UE::…` |
| `UCurveTable` helpers | `CurveTable` | `CkCurveTable_Utils.h` | `UCk_Utils_CurveTable_UE::…` |
| `FRuntimeFloatCurve` / curve evaluation helpers | `Curve` | `CkCurve_Utils.h` | `UCk_Utils_Curve_UE::…` |
| Project/engine settings entry (`UCk_Core_Settings`) | `Settings` | `CkCore_Settings.h` | project defaults |
| File / asset discovery (scope, strategy, localized roots) | `IO` | `CkIO_Utils.h` | `ECk_AssetSearchScope::Game` |
| Deferred config (write key-value to `.ini` deferred) | `IO` | `CkDeferredConfig.h` | `UCk_DeferredConfig` |
| `FCk_Meter` (value range + normalized current) | `Meter` | `CkMeter.h`, `CkMeter_Utils.h` | health/mana meters |
| Reflection over `FProperty` (sanitized name, user-defined struct GUIDs, property-compat checks, placeholder-class detection) | `Reflection` | `CkReflection_Utils.h` | `UCk_Utils_Reflection_UE::Get_SanitizedUserDefinedPropertyName` |
| Generic type conversion template | `TypeConverter` | `CkTypeConverter.h` | convert between related types |
| `FCk_Condition` rule system (`Pass`/`Fail`) | `Logic` | `CkCondition.h` | data-driven conditions |

### GameplayTags

| Use case | Folder | Key file | Example |
|---|---|---|---|
| Tag requirement checks, intersection, container ops | `GameplayTag` | `CkGameplayTag_Utils.h` | `UCk_Utils_GameplayTag_UE::Get_DoContainersIntersect` |
| `FGameplayTagCountContainer` helpers, stack counts | `GameplayTag` | `CkGameplayTagStack.h` | count-based tag stacking |

### Math / geometry

`Math` has 8 sub-subfolders — treat each as its own topic.

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `Arithmetic`, `Comparison`, numeric helpers | `Math/Arithmetic`, `Math/Comparison` | per folder | type-agnostic arithmetic helpers |
| `FRuntimeFloatCurve` evaluation helpers | `Math/FloatCurve` | per folder | curve sampling |
| 2D/3D geometry primitives and intersection tests | `Math/Geometry` | `CkGeometry_Types.h`, `CkGeometry_Utils.h` | AABB/sphere/line tests |
| Numeric limit wrappers | `Math/NumericLimits` | per folder | typed min/max |
| Random / probability helpers (weights, dice, sampling) | `Math/Probability` | per folder | weighted pick |
| `FCk_ValueRange<T>` (typed `[Min, Max]` with clamp / normalize / interpolate) | `Math/ValueRange` | `CkValueRange.h` | attribute ranges |
| Vector helpers beyond UE's built-ins | `Math/Vector` | per folder | extended ops |
| Color utilities (hue/lightness/mix) | `Color` | `CkColor_Utils.h` | `UCk_Utils_Color_UE::…` |

### Time

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `FCk_Time`, `FCk_Time_Unreal`, world time retrieval (`Get_WorldTime`) | `Time` | `CkTime.h`, `CkTime_Utils.h` | see example below |
| `FCk_Chrono` countdown/accumulator (`Tick` + `ECk_Chrono_OverflowPolicy`, `Consume`, `Complete`, `Reset`) | `Chrono` | `CkChrono.h`, `CkChrono_Utils.h` | timer-like primitive without an entity; `Tick(dt, Wrap)` = recurring-interval gate (0 = every tick), `Clamp` (default) = one-shot latch |

```cpp
// Canonical world-time retrieval
const auto TimeParams  = FCk_Utils_Time_GetWorldTime_Params{World};
const auto TimeResult  = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
const auto CurrentTime = TimeResult.Get_WorldTime().Get_Time();
```

### Strings / enums

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `FString` / `FName` / `FText` helpers | `String` | `CkString_Utils.h` | case-insensitive compare, split |
| Fuzzy match for user-facing search | `String` | `CkFuzzyMatch_Utils.h` | `FCk_FuzzyMatch_Fragment_Data` |
| Common project-wide enums centralized here (`ECk_NormalizationPolicy`, etc.) | `Enums` | `CkEnums.h` | cross-module shared enums |
| Enum-to-string / formatter registration | `Enums` | see `CK_DEFINE_CUSTOM_FORMATTER_ENUM` | `CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Foo)` |

### AngelScript integration

| Use case | Folder | Key file | Example |
|---|---|---|---|
| AngelScript type-validation helpers, debugger entry points | `AngelScript` | `CkAngelScript_TypeValidation.h`, `CkAngelscriptDebugger.h` | gated on `WITH_ANGELSCRIPT_CK` |
| AngelScript-specific format override | `Format` | `CkFormat_AngelScript.h` | per-type AS formatter |
| AngelScript validity override | `Validation` | `CkIsValid_AngelScript.h` | per-type AS IsValid |
| AngelScript macro wiring | `Macros` | `CkMacros_AngelScript.h` | binding macros |

### EnTT (underlying ECS library)

| Use case | Folder | Key file | Example |
|---|---|---|---|
| Include wrapper for EnTT (used by `CkEcs` internals, not for game code) | `Entt` | `Entt.h` | internal use — don't include from game code |

### Data assets (base types used in CkCore)

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `UCk_GameplayTags` data asset (defines project gameplay tags in assets or AngelScript) | `Types/DataAsset` | look inside | `asset MyTags of UCk_GameplayTags { … }` |

---

## Decision tree: "I need to…"

```
…validate something                  → Validation/  (ck::IsValid)
…bail out with a diagnostic on error → Ensure/      (CK_ENSURE_IF_NOT)
…format a string                     → Format/      (ck::Format)
…define a getter/setter              → Macros/      (CK_PROPERTY)
…spawn/find/cast an Actor            → Actor/
…get world time                      → Time/        (Get_WorldTime)
…countdown/accumulate ticks          → Chrono/      (FCk_Chrono)
…check/intersect gameplay tags       → GameplayTag/
…evaluate a curve                    → Curve/ | CurveTable/ | DataTable/
…walk reflection over a UClass       → Reflection/
…work with FProperty names           → Reflection/
…define a condition / rule           → Logic/       (FCk_Condition)
…build a Min/Max value range         → Math/ValueRange
…build a normalized health meter     → Meter/       (FCk_Meter)
…wrap a pointer with const-correct.  → Types/       (TPtrWrapper)
…bundle args for a signal payload    → Payload/     (ck::MakePayload)
…write a chained step pipeline       → Technique/
…scan assets by scope/strategy       → IO/
…write a GameWorldSubsystem base     → Subsystems/
…define a Blueprint-visible shared value (bool/float/int) | SharedValues/
…draw debug lines / progress bars    → Debug/DebugDraw
…print a stack trace / debug name    → Debug/CkDebug_Utils
…define a replicated UObject         → ObjectReplication/
```

---

## Anti-patterns

1. **Don't reimplement helpers that already live here.** Before adding a string op, check `String/`. Before writing a curve evaluator, check `Curve/` and `CurveTable/`. Before writing yet another tag container helper, check `GameplayTag/`.
2. **Don't use UE's `IsValid()` for CkFoundation handles/types.** Use `ck::IsValid()` — it routes through `IsValid_Executor` and honors custom validators. UE's version only knows `UObject*`.
3. **Don't `ensure()` or `check()` directly.** Use `CK_ENSURE_IF_NOT(...)` — it integrates with the ensure subsystem (per-site silencing, script-vs-code break policy, formatted messages).
4. **Don't raw `FString::Printf` in CkFoundation code.** Use `ck::Format(TEXT("{}"), …)` — it works across UE's `FString`/`FName`/custom types and has AS support.
5. **Don't include `CkThirdParty` or EnTT directly from game code.** Go through CkCore's wrappers (`Format`, `Enums` for bitwise enum, `Entt/Entt.h` for the ECS library).
6. **Don't put ECS concepts here.** If it mentions entities, handles, processors, or fragments — it belongs in `CkEcs` / `CkEcsExt`, not `CkCore`.
7. **Don't sprinkle inline comments explaining bool args.** Extract to `constexpr auto DescriptiveName = true;` per the root `CLAUDE.md` style guide.

---

## Where to go next

- Each CkCore subfolder has a `README.md`. Central ones have a full API summary; smaller ones have a 3–5 line stub. Use the tables above as the primary index — the READMEs exist for drill-down.
- For ECS-specific patterns, see `CkEcs/Claude.md`.
- For the full module index and cross-module architecture, see the root `/Source/CLAUDE.md` (section "Module Index").

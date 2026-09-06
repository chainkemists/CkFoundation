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
| Behavior-selecting policy tags (`All`/`Any`, `TransientPackage`, `ReturnOptional`, `DontResetContainer`, `ForceErase`, `TMutability`) | `Policy` | `CkPolicy.h` | `ck::policy::DontResetContainer{}` |
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
| Hide all Ck runtime diagnostics for a capture | `Diagnostics` | `CkDiagnosticVisibility.h` | `-CkStreamerMode` / `ck.Debug.StreamerMode 1` |
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
| Build id (short git hash baked in at build time) | `BuildId` | `CkBuildId.h` | `ck::Get_BuildId()` |
| Client/server build-version reporting over the wire | `Net` | `CkNetVersionReport.h`, `CkNetVersionSubsystem.h` | `UCk_NetVersion_WorldSubsystem_UE` |

### Data

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `UDataTable` helpers (row lookup, struct extraction) | `DataTable` | `CkDataTable_Utils.h` | `UCk_Utils_DataTable_UE::…` |
| `UCurveTable` helpers | `CurveTable` | `CkCurveTable_Utils.h` | `UCk_Utils_CurveTable_UE::…` |
| `FRuntimeFloatCurve` / curve evaluation helpers | `Curve` | `CkCurve_Utils.h` | `UCk_Utils_Curve_UE::…` |
| Project/engine settings entry (`UCk_Core_Settings`) | `Settings` | `CkCore_Settings.h` | project defaults |
| File / asset discovery (scope, strategy, localized roots) | `IO` | `CkIO_Utils.h` | `ECk_AssetSearchScope::Game` |
| Config data asset whose asset refs must not resolve during AS `__InitDefaults` | `IO` | `CkDeferredConfig.h` | `UCk_DeferredConfig_UE` (recipe below) |
| `FCk_Meter` (value range + normalized current) | `Meter` | `CkMeter.h`, `CkMeter_Utils.h` | health/mana meters |
| Reflection over `FProperty` (sanitized name, user-defined struct GUIDs, property-compat checks, placeholder-class detection) | `Reflection` | `CkReflection_Utils.h` | `UCk_Utils_Reflection_UE::Get_SanitizedUserDefinedPropertyName` |
| Generic type conversion template | `TypeConverter` | `CkTypeConverter.h` | convert between related types |
| Declare/query asset references the package graph cannot see (script accessors, config paths) | `Reference` | `CkAssetReferenceProvider.h` | `FCk_AssetReferenceProviderRegistry::Get().Get_ExternalReferences(Path)` |
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
| Streak-bounded random draws — sampling without replacement ("shuffle bag") | `Math/Probability` | `CkShuffleBag.h` | `ck::TShuffleBag<T>` |
| `FCk_ValueRange<T>` (typed `[Min, Max]` with clamp / normalize / interpolate) | `Math/ValueRange` | `CkValueRange.h` | attribute ranges |
| Vector helpers beyond UE's built-ins | `Math/Vector` | per folder | extended ops |
| Color utilities (hue/lightness/mix), named palettes, stable per-hash colors | `Color` | `CkColor_Utils.h` | `UCk_Utils_LinearColor::Get_StableColorFromHash` |

### Time

| Use case | Folder | Key file | Example |
|---|---|---|---|
| `FCk_Time`, `FCk_Time_Unreal`, world time retrieval (`Get_WorldTime`) | `Time` | `CkTime.h`, `CkTime_Utils.h` | see example below |
| `FCk_Time` factories — `ck::time::Seconds` / `Milliseconds` / `Minutes` / `Hz` (consteval, positive-only; e.g. a processor's `static constexpr FCk_Time TickRate`) | `Time` | `CkTime.h` | `ck::time::Hz(4)` == `ck::time::Seconds(0.25)` |
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
…draw random outcomes without streaks → Math/Probability (ck::TShuffleBag)
…build a normalized health meter     → Meter/       (FCk_Meter)
…wrap a pointer with const-correct.  → Types/       (TPtrWrapper)
…bundle args for a signal payload    → Payload/     (ck::MakePayload)
…write a chained step pipeline       → Technique/
…scan assets by scope/strategy       → IO/
…ask who references an asset from    → Reference/  (FCk_AssetReferenceProviderRegistry)
  script/config (no package edge)
…write a GameWorldSubsystem base     → Subsystems/
…define a Blueprint-visible shared value (bool/float/int) | SharedValues/
…draw debug lines / progress bars    → Debug/DebugDraw
…hide runtime diagnostics for capture → Diagnostics/CkDiagnosticVisibility
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

## Implementation notes

Non-obvious *why*s behind CkCore internals. Read the relevant entry before "simplifying" the code it describes.

### AngelScript — `Is_AngelscriptDebugger_Paused`

`ck::Is_AngelscriptDebugger_Paused()` (`AngelScript/CkAngelscriptDebugger.h`) exists because a paused AS VS Code debugger walks property getters to populate its Variables panel; ensures fired from that inspection path historically crashed. While it is true, `Ensure_Impl`, `FCk_Handle::Get<>/Has<>` and `FCk_Registry::Get<>` all silently return defaults. The header mirrors the engine's `WITH_AS_DEBUGSERVER` condition via the always-defined core build macros, so it compiles in Test/Shipping without depending on include order.

### Build id and net version reporting

- `ck::Get_BuildId()` returns the short git hash baked in by `CkCore.Build.cs::GenerateBuildIdHeader` (`"unknown"` if git was unavailable). `Get_ReportedBuildId()` is what the server stamps into GameState and the client reports over RPC; it equals `Get_BuildId()` unless the non-shipping CVar `ck.Net.BuildIdOverride` is set. Comparisons keep using `Get_BuildId()`, so the override deliberately forces a client/server mismatch — the only way to exercise the watermark mismatch rows in PIE, where both sides share one build.
- `ACk_NetVersionReport_UE` carries the build id across the network without the project adopting any particular GameState/PlayerState base class. The server spawns one per player and `SetOwner()`s it to that player's controller, so it is scoped to that connection and the owning client can route a Server RPC on it. Both directions travel: `_ServerBuildId` (server → owning client; the client flags a wrong-version server) and `_ClientBuildId` (owning client → server via RPC; the host reads every report to flag wrong-version clients). `UCk_NetVersion_WorldSubsystem_UE` owns lifetime (spawn on login, destroy on logout; clients receive the actors by replication) and is the registry the watermark reads from on every net role.

### Color — palettes and stable hashing

`UCk_Utils_LinearColor` / `UCk_Utils_Color` ship four palette blocks: a greyscale ramp, Material Design red/blue/green ramps, and the CSS named web colors. `Get_StableColorFromHash` was promoted out of a private inline in `UCk_Utils_CrowdAgent_UE::Get_DebugColor` so any debug subsystem can produce matching per-entity colors; it is equivalent to `FLinearColor::MakeFromHSV8(uint8(Hash & 0xFF), Saturation, Value)`.

### Debug — stack-trace capture cost model

`Get_StackTrace_AddressesOnly` captures program-counter addresses with no symbol resolution (~1–5 µs) and is cheap enough for a hot path. `Get_StackTrace_ResolveAddresses` costs ~50–200 µs per frame — resolve lazily, never on the capture path. `Get_StackTrace_ResolveAddress` returns a pointer into a global thread-safe cache (TUniquePtr-backed, so the `FString` address is stable): the first call inserts an unresolved placeholder and a background `FCallstackResolverThread` fills it in.

### Ensure — the PIE-ID query is thread-gated

`UE::GetPlayInEditorID()` has a `checkf(Value != -2)` that is FATAL on the async loading thread: the engine keeps a per-thread PIE-ID slot (`CoreGlobals.cpp`) and loading-thread slots hold the `-2` sentinel until an `FPlayInEditorLoadingScope` forwards a value. Only the game-thread slot is provably never `-2`, and the non-asserting `PRIVATE_GetGPlayInEditorID()` is not `CORE_API`-exported — so the ensure path queries the asserting accessor only when `IsInGameThread() && NOT IsInAsyncLoadingThread()`, otherwise feeding `-1`, which flows through the same `-1 < 0` test to "Server" (exactly what UE itself reports for worker threads). It is used only for the `[Server]`/`[Client]` prefix. Related: `-unattended`/commandlet contexts bail before the modal dialog because `FSlateApplication::AddModalWindow` spins on `Sleep` with no UI to dismiss it — the same exit `ACk_AutoTestRunner` forces via `Set_EnsureDisplayPolicy(LogOnly)`.

### Format — the fmt plumbing

Under C++20 fmt verifies format strings at compile time and will not take a string literal through the C++20 interface: `fmt::format` wants an `fmt::wformat_string` parameterised by the same arg types it receives, and our args must first pass through `ArgsForward` (which resolves pointers). `ck::Format` / `Format_ANSI` therefore hand-build the `fmt::wformat_string` from `decltype(ArgsForward(...))` — that is the whole reason the one-line return expression looks so heavy; it is all forwarding and return-type derivation. `CkFormat.h`'s includes of `CkIsValid.h` and `CkDebug_Utils.h` look unused but are required when building the Format defaults — do not let an IWYU pass strip them.

### IO — deferred config recipe

Subclass `UCk_DeferredConfig_UE` instead of `UDataAsset` when a config references assets unsafe to load during AngelScript `__InitDefaults` (before `UTypedElementRegistry` is ready):

1. hold the reference as `TSoftObjectPtr`/`TSoftClassPtr` for any package containing `UActorComponent` exports (Niagara, Blueprints);
2. add a matching hard-ref `UPROPERTY(Transient)` for runtime access;
3. override `ResolveAssets()` to `System::LoadAsset_Blocking(MySoftRef)`;
4. in asset declarations use `assets::` (soft), not `assets::load::`.

The base calls `ResolveAssets()` on `FCoreDelegates::OnFEngineLoopInitComplete`; `EnsureResolved()` is an idempotent manual safety net.

### IO — `-CkDeferredCmdsFile=<path>`

`FCommandLine` stores the process command line in a fixed 16,383-char buffer (`CommandLine.h`, `MaxCommandLineSize`), so a long `-ExecCmds="Automation RunTests <list>"` kills the editor before it runs anything. The bypass: a launcher (UnrealToolbox `--test`) writes the command line(s) to a file and passes only the short flag; each non-empty line is queued into `GEngine->DeferredCommands`, the same sink `-ExecCmds` feeds via `ParseExecCommands::QueueDeferredCommands`, drained by the engine tick's exec pump. **Ordering caveat:** these queue AFTER any `-ExecCmds` commands (`UEngine::Init` runs before `OnFEngineLoopInitComplete`) — do not mix the two for order-sensitive command sets.

### IO — blocking-load safety and commandlet detection

- `GIsEngineSafeForBlockingLoads` flips on `OnFEngineLoopInitComplete`; before that, `System::LoadAsset_Blocking` can crash on packages containing `UActorComponent` exports. The registrar also checks `GEngine != nullptr && GIsRunning` up front, because a module loaded AFTER the delegate already broadcast (late plugin load, editor hot-reload, DLL reload) would subscribe to a delegate that never fires again and leave the flag false forever.
- `Get_IsRunningCommandlet()` deliberately does NOT rely on `IsRunningCommandlet()` alone: that reads `PRIVATE_GIsRunningCommandlet`, set during PreInit, and a handful of very-early AngelScript CDO constructions fire before that point (~130 first-pass blocking-load ensures once slipped through a cook). The command line is populated from process start, so it also sniffs `-run=` / `-TargetPlatform=`, and it is re-read on EVERY call (never cached) because the first early-init query can precede a fully populated command line. Its purpose is to let the generated `assets::load::*` helpers downgrade the "called before engine init" ensure — cook first-pass loads are expected and self-heal via `UCk_DeferredAssetInit_UE`, so failing a cook over them is wrong; the ensure stays loud in editor/PIE/game.
- Asset-registry searches set `bIncludeOnlyOnDiskAssets = NOT IsInGameThread()`: in-memory enumeration is game-thread-only (`EnumerateMemoryAssetsHelper` asserts off it), and `LoadAssetByName` from an AS class PostInit runs off-thread during threaded AS init — that is the call path that crashed. On the game thread the full search is kept so unsaved editor assets are still found.
- `Report_PrematureAssetLoad` replaced a per-call `ck::EnsureIfNot` in those generated accessors (each ensure captures three stack traces, ~15 ms — a measured ~2.7 s startup storm). It records a count plus the first message only; the DeferredAssetInit sweep emits one summary line and resets via `Reset_PrematureAssetLoadReport`. The loads are expected and healed, so a surprising count just flags a soft-ref candidate.
- The `CK_WIDE1`/`CK_WIDE2`/`WFILE` char-to-`wchar_t` trick is from <https://stackoverflow.com/a/14421702>.

### IO — deferred AngelScript asset init

`CkDeferredAssetInit_AngelScript.*` fixes `assets::load::` returning nullptr during AS `__InitDefaults`/literal-asset init, plus the hot-reload case where the AS plugin re-runs `__Init_<Name>` on the same cached instance (so `_Arr.Add(...)` in an asset body would double every reload).

- **Phase 1 (boot only)** RESETS each AS class's CDO to its constructed state, then re-runs its DefaultsFunction chain on it. The chain walk mirrors the engine's `ExecuteDefaultsFunctions` (`ASClass.cpp`) — collect child→parent up the super chain, execute in reverse, each function in its own context so a failure does not skip siblings. The reset (script destructor → `ConstructFunction`) is the same pairing `UASClass::ReconstructScriptObject` uses, though this is the first in-place reconstruction of a CDO anywhere — hot reload builds a new class and CDO instead.
  - **This reverses an earlier deliberate decision and its stated rationale**, which was: no pre-reset, because "scalar/object-ref defaults are idempotent, and clearing first risks abandoning state if a mid-chain statement aborts", with the **known limitation** that "`default _X.Add(...)` container patterns on CLASS defaults double here — prefer container assignment". That limitation was not enforceable and was violated 54 times across 23 files in the consuming project; it produced a shipping-severity defect (one `TryDamageEntity` applying damage twice on every pooled applicator after the first, because the doubled CDO list is copied onto recycled instances by `Request_ResetToArchetype`). The abandonment risk the old rationale named is real and is now **loud** rather than silent: a constructor that fails after the destructor ran leaves the CDO unusable and fires `CK_ENSURE_IF_NOT` naming the class. **[DECISION 2026-09-06 — taken, not deferred: keep the reset.** The deciding argument is that the file was already inconsistent with itself. Its own header comment names re-running a body without resetting as the hazard the module exists to fix, and **Phase 2 already resets each literal's cached instance from its CDO before calling `__Init_` for exactly this reason**. Phase 1 was the only place that re-ran an initialiser over already-initialised state. The alternative — revert, convert all 54 `default <container>.Add(...)` sites to assignment, and add a lint over `.as` — leaves the framework accepting a body it cannot execute twice and replaces a structural guarantee with a rule that has already failed once in the field. Merging the PR that carries this line is the ratification; **rejecting it means doing the revert AND the lint together**, never the revert alone.]
  - The reset is deliberately PARTIAL, and that is load-bearing: the generated destructor skips primitives, references and handles, and the generated constructor touches those only where they carry an explicit initialiser. Containers, structs and strings come back pristine; handles keep their prior value — which is why an actor CDO's DefaultComponents (written at their script `VariableOffset` BEFORE the constructor runs) survive. Do not "improve" this into a full zeroing reset.
- **Phase 2 (boot + hot-reload)** resets each literal's cached instance from its CDO, then calls `__Init_<Name>` directly, bypassing the getter's first-call cache. The preprocessor (`AngelscriptPreprocessor.cpp` ~4003) emits `__Asset_{Name}`, a caching `Get{Name}()` property and `void __Init_{Name}({Type})`; `Module->PostInitFunctions` holds the `Get<Name>` GETTER names and is used as a drift sanity check. Instance reset skips transient properties and BARE Instanced object refs (orphaning the subobject would break `default _Comp.Foo = ...`) but must reset Instanced CONTAINERS, whose contents asset bodies recreate via `NewObject`.
- **Scope:** AS classes come from `FAngelscriptManager::GetActiveModules() -> Module->Classes`, never `TObjectIterator<UClass>` (which would scan thousands of non-AS UClasses).
- **Surgical heal.** The full sweep re-runs ~1200 CDOs and every literal init just to heal a handful, and it measured AS-execution bound, not IO bound. So `Note_DeferredAssetLoad_FromActiveContext` (called from `ck::EnsureIfNot_PrematureAssetLoad`, ungated so it also runs in cook) walks the active AS call stack ONCE and records the exact CDOs (`GDeferredLoadCDOs`) and literal names (`GDeferredLiteralNames`) that deferred. It mirrors the engine's `GetASConstructionScriptObject` (`Bind_UObject.cpp`): a frame whose `this` class chain owns the executing DefaultsFunction is a CDO default; a frame running a `__Init_<Name>` global is a literal body. Both, either or neither may appear — "neither" is safe because the original sweep never healed those cases either. An unreadable active context sets `GAttributionUncertain`, forcing BOTH phases to the full sweep; `ck.DeferredAssetInit.ForceFullHeal` is the manual escape hatch. **We never under-heal.** Hot-reload always uses full heal (it happens engine-safe, so attribution never fires). `OnAngelscriptPostReload` uses the side-effect-free `Get_IsEngineSafeForBlockingLoads_Peek` so its guard does not trip the `WasBlockingLoadQueriedWhileUnsafe` short-circuit.
- **Disregard-for-GC retention (`!WITH_EDITOR` only).** AS `asset ... of ...` owners and AS CDOs are created during AS InitialCompile, BEFORE `FEngineLoop` closes the disregard-for-GC set, so they land in the permanent pool GC never traverses (it assumes disregard objects only reference other permanent objects). The heal sweep then attaches normal-pool objects under them (minted sub-objects, `assets::load::`'d cooked assets), so the first GC reclaims them out from under the untraversed owner → dangling pointer → crash. Fix: `AddToRoot` those targets so `IsRooted()` satisfies the verifier's accept-test (`GarbageCollectionVerification.cpp:110` — `IsRooted || IsDisregardForGC || OwnerIndex>0 || ClusterRoot`). It must go through the GC reference collector, NOT `FReferenceFinder`: the AS runtime GC-tracks script-class UObject members (including non-`UPROPERTY` resolved-hard-ref fields like `UStaticMesh StandBodyMeshAsset;`) only through the autogenerated schema the real GC and verifier use, which `FReferenceFinder`'s legacy token-stream walk misses. It runs pre-GC rather than once at boot because some refs resolve lazily as actors stream in, and it roots only, never unroots — `AddToRoot` is a boolean flag, not a refcount, so unrooting could clear a root another system set on a shared asset (cost: sub-objects orphaned by a packaged `-as-development-mode` hot-reload stay rooted; dev-only, shipping bakes AS). Editor is excluded because the bug does not manifest there (asset registry/Content Browser keep things alive), verifiers are off, and rooting cooked assets every GC could interfere with editor asset GC.

### Macros — attribution

The `NARG_`/`NARG_I_`/`ARG_N`/`RSEQ_N` variadic-argument-count macros are from <https://stackoverflow.com/a/26408195/368599>, modified for MSVC.

### Math

- `UCk_Utils_Geometry_UE::Project_Box_ToScreen` is derived from the Cog plugin's `FCogWindowHelper::ComputeBoundingBoxScreenPosition` (`CogWindowHelper.cpp`).
- `ECk_Plane_Axis::YZ` rotation: symmetric shapes (circles/spheres) are unaffected — they are rotationally symmetric about the plane normal. Asymmetric debug shapes (text/symbols/arrows) stand upright facing ±X instead of being rotated 90°, which the old `FQuat(RightVector, -90)` form did (it laid them on their side). CkCrowd only ever uses XY, so the change is contained.

### Object — `ck_asgcdiag` console commands (`Ck.Diag.*`)

Diagnostic-only scaffolding, no behavior change. It drove the root-cause investigation of why `asset ... of` sub-objects (item traits, `PlayerMappableKeySettings`) are reclaimed on the first GC sweep even though their rooted owner references them via a `UPROPERTY`. Two scopes: the literal-asset dump covers minted sub-objects under disregard owners in `/Script/AngelscriptAssets`; the CDO probe covers `/Script/Angelscript`, where each AS class CDO is a disregard object and `CkDeferredAssetInit` re-runs `default X = assets::load::...` on it, leaving a disregard CDO pointing at a normal-pool cooked asset — the refs `GarbageCollectionVerification.cpp:110` flags. `Ck.Diag.VerifyGCAssumptions` is the authoritative oracle (warnings flush before the Fatal at `GarbageCollectionVerification.cpp:155`, so the log holds the complete violation list).

### ObjectPooling

- **Release-quiesce contract.** Pre-pooling, an object whose entity died was GC'd and its pending world timers / latent actions silently never fired; a dead object's delegates also compared unequal to any live re-bind. Pooling keeps the instance alive (pinned or parked) with its pointer identity intact, so without an explicit quiesce a lingering timer fires post-release against dead associations, and bindings on longer-lived entities leak into the next vend (stale delivery + duplicate-signature rejection of the re-bind). Only TRACKED objects are quiesced — release on an untracked object must not side-effect timers we do not own. `TryRegisterReleaseQuiesceHook` is the bind-site half (e.g. the ECS signal `Bind` funnel registers the disconnect closure).
- **`Request_ResetToArchetype`.** `UObject::CopyScriptPropertiesFrom` is a whole-object `asIScriptObject` assignment that copies EVERY member declared on the script class, participant included — hence the snapshot/`ON_SCOPE_EXIT` restore of `FCk_Handle_ObjectPoolingParticipant` so binds survive recycling on AngelScript-declared poolables (the contract `CkObjectPoolingParticipant.h` promises). Script-only members exist as byte offsets on the fused UObject because the class generator creates `FProperty`s solely for `UPROPERTY`-exported members; a recycled instance would otherwise resume the previous life's state (e.g. a consumed one-shot cursor). The subobject re-instancing mirrors `FObjectInitializer::InstanceSubobjects`; the instancing graph REUSES a same-named per-instance subobject rather than re-creating it, so the reset recurses into each (instance, template) pair. Instanced subobjects inside Set/Map containers are not walked — none exist in the framework today; add the container walk if one ever does. It is public so the contract is directly testable.
- **`DoesSupportWorldType` override.** The editor ECS world (`UCk_EditorEcsWorld_Subsystem_UE`) vends EntityScripts/components through the pooled path too; without the subsystem in Editor worlds those instances fall back to caller-owned creates with only weak holders (no GC root) and editor GC collects them mid-preview.

### Reflection

- `Get_ExposedPropertiesOfClass` walks the class chain base→derived (build chain + `Algo::Reverse`, then `TFieldIterator` per class with `ExcludeSuper`) so output reads Parent → Child. A flat `Algo::Reverse` over an `IncludeSuper` iteration is WRONG — it also flips the within-class declaration order.
- `Get_PropertyDefaultValueLiteral` emits bare `nullptr` for UObject-ish properties, deliberately. An earlier fix emitted a typed-null cast `<Class>(nullptr)` to disambiguate positional-ctor overload resolution (the OpenSign-class deadlock: bare `nullptr` reports as `<null handle>` and AS can't bind it). That was REVERTED — AngelScript rejects `<UClass>(nullptr)` in struct field-default declarations ("Data type can't be '<Class>'"); UObject types cannot be constructed via type-constructor syntax at all, not just `AActor`/`UActorComponent`. One emit path serves both the field-default and positional-ctor-arg contexts, so the typed cast cannot be applied only in the safe context without splitting the literal representation. Proper fix = Fix #2 of `codegen-bug-positional-ctor-null-uobject.md` (field-assignment-style emit instead of positional ctor for structs with `UObject*` fields); until it lands, adding a `default Params.X = Y` override on an entity-script subclass whose Params struct holds a `UObject*` field can re-open the OpenSign deadlock.
- `ck_reflection_detail::Get_StructLiteral` is two-tier by design. Tier 1 (named constants / early exits): `FTransform` and `FGameplayTag` return `{}` for non-constant values rather than an expression, because their constructor argument order differs from `TFieldIterator` order — a general decomposition would emit a wrong expression; `FVector`/`FRotator` emit a named alias when one applies and otherwise fall through. Tier 2 (general decomposition): for a struct whose every non-parm `UPROPERTY` field is representable, emit `StructName(expr1, ...)`, or `StructName()` when all fields equal their `InitializeStruct` defaults. Any unrepresentable field makes the whole struct return `{}` — never a partial expression.
- `Get_StructFieldOverrides` descends into a nested struct only when that struct itself contains a `UObject*` field AND has diffs (building longer dotted paths), because its positional ctor would carry the same `<null handle>` hazard. A nested struct with diffs but no `UObject*` stops the recursion and emits its own positional-ctor expression at that path — safe because that ctor takes no `nullptr`.

### Reference — why the registry, and the one rule

`IAssetRegistry::GetReferencers` answers only from serialized package edges. An asset reached through an AngelScript
generated accessor (`assets::Get_Foo()`), a config-driven soft path, or a runtime-assembled string leaves no edge — so
a tool asking the graph alone reports it as unreferenced and offers it for deletion, and the engine's own delete dialog
derives its referencer list from the *same* graph and agrees with the mistake.

`FCk_AssetReferenceProviderRegistry` inverts that: a module CREATING such references registers a query
(`CkAngelscriptGenerator`'s asset subsystem does, under `"AngelScript"`), and a tool reasoning about reachability asks
the registry. Neither links the other — which is what lets an *Editor*-type codegen module inform a *DeveloperTool*
auditor without either becoming undeployable.

**The one rule:** `Get_HasAnyProvider()` is not the same question as an empty result. "Nobody could answer" and
"everybody answered no" are different statements, and a consumer that collapses them reports a project it never asked
about as clean. Ask it, and say which of the two you got.

Game thread only, unsynchronized by design (registration is module startup, queries are inside a scan); an
`IsInGameThread` ensure pins it.

### Validation — `CkUntracedStructSafety` and `FCk_Entity`

`FCk_Entity` is approved by reflected path (`/Script/CkEcs.Ck_Entity`): it holds only an `entt::entity` integer id, and its `int32` mirror fields exist solely for the editor debugger (compiled out under `WITH_EDITORONLY_DATA`). A cooked build therefore sees zero reflected fields, so the field-less-struct heuristic would otherwise reject every dynamic fragment / EntityScript spawn-param embedding an `FCk_Handle` (whose `_Entity` member is an `FCk_Entity`).

---

## Where to go next

- Each CkCore subfolder has a `README.md`. Central ones have a full API summary; smaller ones have a 3–5 line stub. Use the tables above as the primary index — the READMEs exist for drill-down.
- For ECS-specific patterns, see `CkEcs/Claude.md`.
- For the full module index and cross-module architecture, see the root `/Source/CLAUDE.md` (section "Module Index").

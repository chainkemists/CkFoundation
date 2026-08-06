# PHASE 0 — Module scaffold, schema, registry

**Goal:** `CkGameSettings` compiles as a registered module; the headless registry works end-to-end
in memory (register / typed get-set / change delegates / reset / collection PDA); named specs green.
**No persistence, no CVar apply, no widgets in this phase** — `Request_SetSettingValue_*` updates
the in-memory value and fires the change delegate only.

## Entry criteria

1. `Plugins/CkFoundation` on `dev`, clean tree (`git -C Plugins/CkFoundation status`). Create and
   switch to `feature/game-settings`.
2. Capture the baseline: from BB root, `CkAuto\UnrealToolbox.exe --test --no-live` (editor closed).
   Record total/pass/fail counts AND failing test names into `PROGRESS.md` §Baseline. If this was
   already done in a prior session, verify the recorded baseline exists and skip.
3. Load skills: `ck-change-control`, `ck-macros-and-codegen`, `ck-tests-authoring-and-running`,
   `ck-angelscript-interop`.

## Steps

### 0.1 Scaffold
- `Source/CkGameSettings/CkGameSettings.Build.cs` — mirror `CkLoadingScreen.Build.cs` exactly in
  shape (`class CkGameSettings : CkModuleRules`). Phase 0 deps ONLY:
  `Core, CoreUObject, Engine, GameplayTags, DeveloperSettings, CkCore, CkCVar, CkLog, CkSettings`.
  (CkInput/CkUI/UMG/CommonUI arrive in Phase 3 with an annotation comment; do NOT add them now.)
- `CkGameSettings_Module.{h,cpp}`, `CkGameSettings_Log.{h,cpp}` — copy CkLoadingScreen's, rename.
  Log namespace: `ck::game_settings` via `CK_DEFINE_LOG_FUNCTIONS`.
- Add the module to `CkFoundation.uplugin`: `"Type": "Runtime"`, `"LoadingPhase": "Default"`,
  standard Win64/Mac/Linux allowlist. Insert alphabetically among the existing entries.

### 0.2 `Public/CkGameSettings/CkGameSettings_Common.h` — the schema (pre-designed; fill bodies only)

```cpp
UENUM(BlueprintType)
enum class ECk_GameSettings_ValueType : uint8
{
    Bool = 0,
    Int32,
    Float,
    String
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_ValueType);

UENUM(BlueprintType)
enum class ECk_GameSettings_Scope : uint8
{
    Machine = 0,
    Player
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_Scope);

UENUM(BlueprintType)
enum class ECk_GameSettings_PersistencePolicy : uint8
{
    Provider = 0,
    External
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_PersistencePolicy);

UENUM(BlueprintType)
enum class ECk_GameSettings_ApplyBindingType : uint8
{
    None = 0,
    CVar,
    Handler
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_ApplyBindingType);
```

`FCk_GameSettings_SettingOption` — `_Label` (FText), `_Value` (FString); both essential via
`CK_DEFINE_CONSTRUCTORS`.

`FCk_GameSettings_EditConditions` — `_RequiredPlatformTraits` (FGameplayTagContainer),
`_DisabledReason` (FText). All optional (`CK_PROPERTY`), default ctor only.

`FCk_GameSettings_SettingDefinition` — USTRUCT(BlueprintType), `CK_GENERATED_BODY`, private
members with `UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))`:
- Essentials (in `CK_DEFINE_CONSTRUCTORS(FCk_GameSettings_SettingDefinition, _Key, _ValueType, _DefaultValue)`):
  `_Key` (FName), `_ValueType` (ECk_GameSettings_ValueType), `_DefaultValue` (FString).
- Optionals (fluent `CK_PROPERTY`): `_Scope` (default Machine), `_PersistencePolicy` (default
  Provider), `_ApplyBindingType` (default None), `_CVar` (FCk_CVarRef; meaningful only when
  binding type == CVar), `_DisplayName` (FText), `_Description` (FText), `_CategoryTags`
  (FGameplayTagContainer), `_MinValue`/`_MaxValue` (FString; empty = unbounded — numeric types
  only), `_Options` (TArray<FCk_GameSettings_SettingOption>), `_EditConditions`.

Request structs (plain USTRUCT(BlueprintType) — deliberately NOT `FCk_Request_Base`; there is no
`_Requests` fragment; the subsystem carve-out applies. Fence: do not derive):
```cpp
FCk_Request_GameSettings_SetValue_Bool    { _Key (FName), _Value (bool)    }  // essentials, CK_DEFINE_CONSTRUCTORS
FCk_Request_GameSettings_SetValue_Int32   { _Key, _Value (int32)   }
FCk_Request_GameSettings_SetValue_Float   { _Key, _Value (float)   }
FCk_Request_GameSettings_SetValue_String  { _Key, _Value (FString) }
FCk_Request_GameSettings_ResetToDefault   { _Key }
```

Delegates:
```cpp
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_GameSettings_OnSettingChanged,
    FName, InKey, const FString&, InNewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCk_Delegate_GameSettings_OnSettingChanged_MC,
    FName, InKey, const FString&, InNewValue);
```

Typed apply-handler delegates (registered per key; used from Phase 1 onward, declared now):
`FCk_Delegate_GameSettings_ApplyHandler_{Bool,Int32,Float,String}` — DYNAMIC one-param each,
mirroring `FCk_Delegate_CVar_OnChanged_*` (`CkCVar_Data.h:148-151`).

### 0.3 `Settings/CkGameSettings_Settings.{h,cpp}`
`UCk_GameSettings_ProjectSettings_UE : UCk_Plugin_ProjectSettings_UE`, `meta = (DisplayName = "Game Settings")` —
mirror `CkLoadingScreen_Settings.h` exactly in shape. Phase 0 properties:
- `_CollectionScanPaths` (TArray<FDirectoryPath>) — dirs scanned for collection PDAs (precedent:
  `CkInput_Settings.h` `_MappingContextScanPaths`).
- `_DeferredApplyTimeoutSeconds` (float, default 30) — used Phase 1; declare now.
Plus the plain static-accessor class `UCk_Utils_GameSettings_Settings_UE` (NOT a UCLASS — see the
LoadingScreen exemplar `CkLoadingScreen_Settings.h:106`).

### 0.4 `Collection/CkGameSettings_Collection.{h,cpp}`
`UCk_GameSettings_Collection_PDA : UCk_DataAsset_PDA` (base is in CkCore — locate with
`rg -n "class CKCORE_API UCk_DataAsset_PDA" Source/CkCore`; if the include path surprises you,
mirror whatever `UCk_Provider_PDA` includes). One property:
`_Settings` (TArray<FCk_GameSettings_SettingDefinition>), `CK_PROPERTY_GET`.

### 0.5 `Subsystem/CkGameSettings_Subsystem.{h,cpp}`
`UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_GameSettings")`
`UCk_GameSettings_Subsystem_UE : UGameInstanceSubsystem`. NOT tickable in Phase 0.
Public API (trailing-return style for non-UFUNCTION; UFUNCTION style per house rules — copy the
LoadingScreen subsystem's formatting):

- `Initialize` / `Deinitialize` / `ShouldCreateSubsystem` (`ShouldCreateSubsystem` returns false
  for dedicated servers: `IsRunningDedicatedServer()`; MUST still return true for commandlet/
  null-RHI so AutoTests run).
- `Request_RegisterSetting(const FCk_GameSettings_SettingDefinition& InDefinition) -> bool`
  (UFUNCTION). Validation boundary (ensure + reject, atomicity per definition): key None,
  duplicate key, default not parseable as `_ValueType`, `_ApplyBindingType == CVar` with unset
  `_CVar` name, min/max on non-numeric type.
- `Request_RegisterSettings(const TArray<FCk_GameSettings_SettingDefinition>&) -> bool` — ATOMIC:
  validate all first; one invalid → ensure + register none, return false.
- `Request_RegisterCollection(const UCk_GameSettings_Collection_PDA*) -> bool` — forwards to the atomic array path.
- Typed access (UFUNCTIONs; unknown key or type mismatch = validation boundary — ensure + return
  the supplied fallback):
  `Get_SettingValue_Bool(FName InKey, bool InFallback = false) -> bool` (and `_Int32`, `_Float`,
  `_String` — UFUNCTION defaults use `()` syntax rules).
- `Get_IsSettingRegistered(FName) -> bool`, `Get_SettingDefinition(FName, FCk_GameSettings_SettingDefinition& OutDef) -> bool`,
  `Get_AllSettingKeys() -> TArray<FName>`, `Get_SettingKeysByCategory(const FGameplayTagQuery&) -> TArray<FName>`.
- `Request_SetSettingValue_Bool(const FCk_Request_GameSettings_SetValue_Bool&) -> bool` (+ Int32/
  Float/String). Phase 0 semantics: validate (registered, type match, range/options clamp-or-reject
  — REJECT with ensure, don't silently clamp), store in-memory, fire change delegate iff value
  actually changed. Returns whether the value now holds (idempotent same-value set = true, no fire).
- `Request_ResetToDefault(const FCk_Request_GameSettings_ResetToDefault&) -> bool`,
  `Request_ResetAllToDefaults() -> int32` (count reset).
- `BindTo_OnSettingChanged(FName InKey, const FCk_Delegate_GameSettings_OnSettingChanged&)` /
  `UnbindFrom_OnSettingChanged(...)`; `InKey == NAME_None` = wildcard (all settings).
- Internal state: `TMap<FName, FCk_GameSettings_SettingDefinition> _Definitions;`
  `TMap<FName, FString> _CurrentValues;` ordered registration list `TArray<FName> _RegistrationOrder;`
  binding table keyed by FName (NAME_None bucket = wildcard).

### 0.6 `Subsystem/CkGameSettings_Utils.{h,cpp}`
`UCk_Utils_GameSettings_UE : UBlueprintFunctionLibrary` — one UFUNCTION per subsystem public
member, each taking `const UObject* InWorldContextObject` first
(`meta = (WorldContext = "InWorldContextObject")`), resolving
GameInstance → subsystem, ensure-if-missing. Category `"Ck|Utils|GameSettings"`,
DisplayName `"[Ck][GameSettings] <Name>"`. This BFL is the AS surface — NO `BlueprintInternalUseOnly`.

### 0.7 Tests — `Source/CkGameSettings/Public/CkGameSettings/CkGameSettings_Registry.spec.cpp`
(beside code; Compass precedent `CkCompass_Utils.spec.cpp`). Consult `ck-tests-authoring-and-running`
for the spec harness shape. If the subsystem can't be exercised without a GameInstance in a plain
spec, split: pure validation/parsing logic goes into free functions in a
`Private/CkGameSettings_Validation.{h,cpp}` tested directly, and subsystem-level tests move to a
CkTests AS AutoTest in Phase 1 — record the split in PROGRESS. Required names (rubric):

- `Ck.CkGameSettings.Registry.RegisterAndQuery`
- `Ck.CkGameSettings.Registry.DuplicateKeyRejected` (invalid-input test: rejection + zero mutation + no crash)
- `Ck.CkGameSettings.Registry.AtomicBatchRejectsAll`
- `Ck.CkGameSettings.Registry.TypedAccessAndMismatchRejected`
- `Ck.CkGameSettings.Registry.SetValueFiresChangeOnce` (same-value set does NOT fire)
- `Ck.CkGameSettings.Registry.ResetAllRestoresDefaults`
- `Ck.CkGameSettings.Registry.RangeViolationRejected`

### 0.8 Build + gate
```
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.CkGameSettings" --discover-fresh
```
(editor CLOSED — exit 77 means it wasn't.)

**Decision gate:** expected = build Succeeded, all 0.7 names green.
- Compile errors → fix within this phase (load `ck-debugging-playbook` if UHT/linker-shaped).
- Any 0.7 test red → fix; re-run.
- Exit 76 (AS compile failed) → you broke the generated AS surface; check the log for
  `Angelscript: Error` naming a generated file; likely a UFUNCTION signature violating PROMPT
  fences → fix the signature, not the generated file.
- Anything else (crash, hang, unexpected exit code) → STOP, record in PROGRESS blockers, end session.

Then the regression check: full `--test --no-live`; delta vs baseline must be zero (report as
"baseline N failing {names} → still N {names}").

## Fences (phase-specific)

- No persistence, no CVar writes, no `FCoreDelegates`, no ticking in this phase.
- Do not create `FCk_Handle_GameSettings` or any ECS artifact.
- Do not add UMG/CkUI/CkInput deps yet.
- `Get_SettingValue_*` fallback params: `()` not `{}` (UHT).
- The subsystem holds NO pointer caches to definitions across re-registration; values are
  canonical FString internally, converted at the typed boundary.

## Exit criteria (measurable)

1. `CkAuto\UnrealToolbox.exe --build` Succeeded with editor closed.
2. All seven `Ck.CkGameSettings.Registry.*` tests green via toolbox with `--discover-fresh`.
3. Full suite delta-zero vs recorded baseline.
4. `rg -n "GConfig" Source/CkGameSettings` → 0 hits. `rg -n "BlueprintInternalUseOnly" Source/CkGameSettings` → 0 hits.
5. Committed on `feature/game-settings` (module + uplugin change + tests; nothing else staged).
6. PROGRESS.md updated: phase status, test names green, any deviations recorded.

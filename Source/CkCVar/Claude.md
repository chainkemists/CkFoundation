# CkCVar

**Purpose:** A **Blueprint/AngelScript-facing** console-variable layer: `FCk_CVarRef` (a typed CVar reference: name + `ECk_CVarType`) plus the `UCk_Utils_CVar_UE` BPFL that the CkCVarEditor K2 nodes expand into, and a `DefaultConfig` registry (`UCk_CVar_Settings_UE`) that persists BP-declared CVars so they exist again on the next boot.

**C++ callers do not use this module** — they register a CVar with the engine's `FAutoConsoleVariableRef` over a file-scope static directly (exemplar: `CkCrowd/Public/CkCrowd/Settings/CkCrowd_DebugSettings.cpp:43`, `CVarDrawAgentBody` → `ck.Crowd.Debug.AgentBody`), which registers at TU load, before any CDO exists.

**Depends on:** `CkCore`, `CkLog` (+ engine `DeveloperSettings`).
**Used by:** `CkCVarEditor` (the K2 nodes and the `FCk_CVarRef` pin/details customization) and `CkAngelscriptGenerator` (emits the `cvar::` constants). No runtime Ck module depends on it.

---

## Key types (`CkCVar/CkCVar_Data.h`)

```cpp
UENUM(BlueprintType) enum class ECk_CVarType : uint8 { Int32, Float, Bool, String, Command };

UENUM(BlueprintType) enum class ECk_CVar_InitialCallbackPolicy : uint8
{
    DoNotFire,       // bind and wait for next change
    FireImmediately  // bind and fire with the current value right now
};

// Typed CVar reference. HasNativeMake → UCk_Utils_CVar_UE::Make_CVarRef.
USTRUCT(BlueprintType) struct FCk_CVarRef
{
    CK_PROPERTY_GET(_Name);   // FName
    CK_PROPERTY_GET(_Type);   // ECk_CVarType
    auto IsValid() const -> bool;   // _Name != NAME_None
};

// Persisted declaration written into the settings registry by the Register K2 node.
USTRUCT(BlueprintType) struct FCk_CVarDefinition
{
    CK_PROPERTY_GET(_Name); CK_PROPERTY_GET(_Type);
    CK_PROPERTY_GET(_DefaultValue); CK_PROPERTY_GET(_HelpText);   // both FString
};

// Opaque callback ticket; INDEX_NONE means "not bound".
USTRUCT(BlueprintType) struct FCk_CVarCallbackHandle { CK_PROPERTY_GET(_ID); };
```

Callback signatures (dynamic delegates, one per type):
`FCk_Delegate_CVar_OnChanged_Int32` / `_Float` / `_Bool` / `_String`, and the parameterless
`FCk_Delegate_CVar_OnCommand`. `FCk_CVar_DelegateSignatureHolder::GetSignatureFunctionForType` maps
an `ECk_CVarType` to the matching `UFunction*` — this is how the K2 nodes build a correctly typed
delegate pin.

---

## Key API — `UCk_Utils_CVar_UE` (`CkCVar/Utils/CkCVar_Utils.h`)

Only **three** members are meant to be called by hand. Everything else is `INTERNAL_*` with
`meta = (BlueprintInternalUseOnly = true)` — they are the expansion targets of the CkCVarEditor K2
nodes and are not reachable from a normal Blueprint graph.

| Symbol | Kind | Notes |
|---|---|---|
| `Make_CVarRef(FName InName, ECk_CVarType InType)` | `UFUNCTION(BlueprintPure)`, `NativeMakeFunc` | the make-node for `FCk_CVarRef` |
| `IsRegistered(FCk_CVarRef InRef)` | `UFUNCTION(BlueprintPure)` | `IConsoleManager::FindConsoleVariable != nullptr` |
| `DetectCVarType(FName InCVarName) -> TOptional<ECk_CVarType>` | plain C++ static (no UFUNCTION) | settings registry first, then probes `IConsoleManager`; used by the AS generator |

The internal surface, spelled exactly:

```cpp
// Register (find-or-create) + bind in one call; returns the unbind ticket:
INTERNAL_Register_Int32 / _Float / _Bool / _String   (FName, DefaultValue, const FString& InHelp,
                                                      const FCk_Delegate_CVar_OnChanged_*&,
                                                      ECk_CVar_InitialCallbackPolicy)
INTERNAL_Register_Command                            (FName, const FString& InHelp,
                                                      const FCk_Delegate_CVar_OnCommand&)

// Bind only, against an already-registered CVar:
INTERNAL_Bind_Int32 / _Float / _Bool / _String       (FCk_CVarRef, delegate, ECk_CVar_InitialCallbackPolicy)
INTERNAL_Bind_Command                                (FCk_CVarRef, FCk_Delegate_CVar_OnCommand)

INTERNAL_Unbind(FCk_CVarCallbackHandle)

INTERNAL_Get_Int32 / _Float / _Bool / _String        (FCk_CVarRef)   // BlueprintPure
INTERNAL_Set_Int32 / _Float / _Bool / _String        (FCk_CVarRef, Value)
```

Behaviour worth knowing: change callbacks are marshalled to the game thread via `AsyncTask` before
executing; `FireImmediately` fires synchronously at bind time. The `INTERNAL_Set_*` calls no-op when
the value is unchanged, otherwise `SetWithCurrentPriority` + `CallAllConsoleVariableSinks()`.
Failure to find a CVar logs `ck::cvar::Warning` and returns an invalid handle / zero value.

---

## Key API — `UCk_CVar_Settings_UE` (`CkCVar/Settings/CkCVar_Settings.h`)

`UDeveloperSettings`, `Config = CkCVar, DefaultConfig`, shown under **CkFoundation → CVar Registry**.
All members are plain C++ (no UFUNCTIONs):

```cpp
static auto Get() -> UCk_CVar_Settings_UE*;                       // GetMutableDefault
auto RegisterDefinition(const FCk_CVarDefinition&) -> void;       // upsert by name + TryUpdateDefaultConfigFile
auto UnregisterDefinition(FName) -> void;
auto GetType(FName) const -> TOptional<ECk_CVarType>;
auto GetAllRegisteredNames() const -> TArray<FName>;
auto Get_RegisteredCVars() const -> const TArray<FCk_CVarDefinition>&;
```

`FCkCVarModule::StartupModule` (`CkCVar_Module.cpp:12`) walks `Get_RegisteredCVars()` and re-registers
each persisted definition with `IConsoleManager` (skipping names that already exist), so a CVar
declared from a Blueprint survives a restart even before that graph runs.

---

## Blueprint surface — the CkCVarEditor K2 nodes

The nodes are the intended authoring surface; they pick the right `INTERNAL_*` overload from the
`FCk_CVarRef`'s `ECk_CVarType` and synthesize a matching delegate pin.

| Node class | Title | Expands to |
|---|---|---|
| `UCk_K2Node_CVar_Register` | `[Ck] CVar Register` | `INTERNAL_Register_*`, and also writes an `FCk_CVarDefinition` into the settings registry |
| `UCk_K2Node_CVar_Get` | `[Ck] CVar Get` | `INTERNAL_Get_*` |
| `UCk_K2Node_CVar_Set` | `[Ck] CVar Set` | `INTERNAL_Set_*` |
| `UCk_K2Node_CVar_Bind` | `[Ck] CVar Bind To Changed` | `INTERNAL_Bind_*` |
| `UCk_K2Node_CVar_Unbind` | `[Ck] CVar Unbind` | `INTERNAL_Unbind` |

Menu category: `Ck|Utils|CVar`. `FCk_CVarRef` properties and pins get a searchable CVar-name picker
built from `ck::layout::SCVarRef_Widget`, wired up by `ck::layout::FCVarRef_Details`
(details panel) and `ck::layout::SCVarRef_GraphPin` + `FCk_CVarRef_GraphPanelPinFactory` (graph pins).

---

## AngelScript surface

Generated by `CkAngelscriptGenerator`, regenerated at editor startup — do not edit:

- `Script/Generated/cvar.as` — a `cvar::` namespace of `const FCk_CVarRef` constants, one per CVar
  discovered from `IConsoleManager` plus the settings registry (Commands are skipped: no value to
  read or write).
- `Script/Generated/utils_c_var.as` — the `utils_c_var::` wrapper, which contains exactly
  `Make_CVarRef` and `IsRegistered`. The `INTERNAL_*` functions are not wrapped, so **AngelScript
  can name and test a CVar but cannot get, set, or bind one through this module.**

### `FCk_CVarRef` as a designer-editable reference

Use `FCk_CVarRef` in settings/fragment properties when a designer should pick *which* CVar to observe,
rather than hardcoding the name:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
FCk_CVarRef _SpeedCVar;
```

---

## When to use CVars vs. Settings

| Scenario | Use |
|---|---|
| Designer wants a slider in Project Settings | `CkSettings` (`UCk_Plugin_ProjectSettings_UE` with `ConsoleVariable` meta) |
| C++ wants a live-tweakable value during a session (no restart) | engine `FAutoConsoleVariableRef` over a file-scope static — NOT this module |
| Blueprint needs to declare a CVar, or react to a value change at runtime | the `[Ck] CVar *` K2 nodes |
| Value must persist across sessions (saved to `.ini`) | `CkSettings` |

CVars and settings are not mutually exclusive — the engine's `UDeveloperSettingsBackedByCVars` bridges them, and `CkCrowd_DebugSettings.cpp` shows the hand-rolled variant (CVar callback writes the setting, `PostInitProperties` hydrates the CVar).

---

## Anti-patterns

1. Don't reach for `UCk_Utils_CVar_UE` from C++. Its usable surface is `Make_CVarRef` / `IsRegistered` / `DetectCVarType`; everything else is `BlueprintInternalUseOnly` K2-node plumbing. Register with `FAutoConsoleVariableRef` instead.
2. Don't bind a callback without storing the `FCk_CVarCallbackHandle`. `INTERNAL_Unbind` is the only way to remove it, and it needs the ID.
3. Don't register the same CVar name twice with different types — registration is find-or-create and the existing CVar wins, so the second declaration's type is silently ignored while `INTERNAL_Get_*`/`Set_*` keep reading it through the wrong accessor.
4. Don't assume a callback runs inline on change — every bound callback is re-dispatched via `AsyncTask(ENamedThreads::GameThread, ...)`, so it lands a frame boundary later. Only the `FireImmediately` initial fire is synchronous.

---

## See also
- `CkSettings/Claude.md` — project settings backed by CVars.
- `CkCore/Build/README.md` — compile-time feature flags (different from runtime CVars).

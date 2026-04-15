# CkCVar

**Purpose:** Console variable registration, typed access, and callback binding. Provides `FCk_CVarRef` (a typed CVar handle) and `UCk_Utils_CVar_UE` (register/read/write/bind/unbind CVars from C++, BP, and AS without touching `IConsoleManager` directly).

**Depends on:** `CkCore`, `CkLog`.
**Used by:** `CkAngelscriptGenerator`, `CkEcsExt`, plus any module that needs runtime-mutable tuning values.

---

## Key types

```cpp
UENUM() enum class ECk_CVarType : uint8 { Int32, Float, Bool, String, Command };

UENUM() enum class ECk_CVar_InitialCallbackPolicy : uint8
{
    DoNotFire,       // bind and wait for next change
    FireImmediately  // bind and fire with the current value right now
};

USTRUCT(BlueprintType)
struct FCk_CVarRef   // typed CVar reference: name + ECk_CVarType
{
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Type);
};

USTRUCT() struct FCk_CVarCallbackHandle; // opaque handle to unbind a callback
```

---

## Common operations

### Register / find

```cpp
// Register (finds existing or creates new):
UCk_Utils_CVar_UE::Register_Float(TEXT("ck.mymodule.speed"), 1.0f, TEXT("Gameplay speed scale"));
UCk_Utils_CVar_UE::Register_Bool (TEXT("ck.mymodule.debug"), false, TEXT("Enable debug draw"));
```

### Read

```cpp
float Speed  = UCk_Utils_CVar_UE::Get_Float(TEXT("ck.mymodule.speed"));
bool  Debug  = UCk_Utils_CVar_UE::Get_Bool (TEXT("ck.mymodule.debug"));
```

### Write (programmatic change)

```cpp
UCk_Utils_CVar_UE::Set_Float(TEXT("ck.mymodule.speed"), 2.0f);
```

### Bind / unbind callback

```cpp
// Fires whenever the CVar changes (and optionally immediately):
auto CallbackHandle = UCk_Utils_CVar_UE::BindCallback_Float(
    TEXT("ck.mymodule.speed"),
    FOnCVarFloatChanged::CreateUObject(this, &UMyObject::OnSpeedChanged),
    ECk_CVar_InitialCallbackPolicy::FireImmediately);

// Unbind using the opaque handle:
UCk_Utils_CVar_UE::UnbindCallback(CallbackHandle);
```

### `FCk_CVarRef` as a designer-editable reference

Use `FCk_CVarRef` in settings/fragment properties when a designer should pick *which* CVar to observe, rather than hardcoding the name:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
FCk_CVarRef _SpeedCVar;
```

---

## When to use CVars vs. Settings

| Scenario | Use |
|---|---|
| Designer wants a slider in Project Settings | `CkSettings` (`UCk_Plugin_ProjectSettings_UE` with `ConsoleVariable` meta) |
| Developer wants a live-tweakable value during a session (no restart) | `CkCVar` |
| AS or Blueprint needs to react to a value change at runtime | `CkCVar` callback |
| Value must persist across sessions (saved to `.ini`) | `CkSettings` |

CVars and settings are not mutually exclusive — `UDeveloperSettingsBackedByCVars` bridges them (settings expose the CVar, CkCVar can still bind a callback to it).

---

## Anti-patterns

1. Don't call `IConsoleManager::Get().FindConsoleVariable` directly in feature code — `UCk_Utils_CVar_UE` wraps this with error logging and type safety.
2. Don't bind a callback without storing the `FCk_CVarCallbackHandle`. You'll leak the callback until the session ends.
3. Don't register the same CVar name twice from different modules — the registration is find-or-create, but if the second caller specifies a different type, behavior is undefined.

---

## See also
- `CkSettings/Claude.md` — project settings backed by CVars.
- `CkCore/Build/README.md` — compile-time feature flags (different from runtime CVars).

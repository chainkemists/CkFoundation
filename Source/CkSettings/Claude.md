# CkSettings

**Purpose:** Base classes for CkFoundation project settings and user settings. Provides `UCk_Plugin_ProjectSettings_UE` (project-wide, persisted to `CkFoundation.ini`) and a user-settings base. Every CkFoundation module that exposes designer-configurable project settings inherits from one of these.

**Depends on:** Nothing else Ck (no Ck deps — it's a near-root module).
**Used by:** Every module that has configurable project settings: `CkCore`, `CkEcs`, `CkLog`, `CkLabel`, `CkRecord`, `CkProvider`, `CkActorProxy`, `CkActorRelay`, `CkAudio`, etc.

---

## Key types

### `UCk_Plugin_ProjectSettings_UE` (`CkProjectSettings.h`)

```cpp
UCLASS(Abstract, DefaultConfig, Config = CkFoundation)
class UCk_Plugin_ProjectSettings_UE : public UDeveloperSettingsBackedByCVars
{
    explicit UCk_Plugin_ProjectSettings_UE(const FObjectInitializer&);
};
```

- Backed by `UDeveloperSettingsBackedByCVars` — properties can be exposed as console variables (set `meta = (ConsoleVariable = "ck.my.var")`).
- Config saves to `CkFoundation.ini` (game config).
- Access pattern: `GetDefault<UCk_MyModule_Settings>()`.
- `UCk_Utils_ProjectSettings_UE` (`CkProjectSettings_Utils.h`) provides discovery and validation helpers.

### User settings (`CkUserSettings.h`)

Separate class for per-user persistent settings (saved in user local config, not game config). Use for resolution, audio volume, keybinds — things that shouldn't be committed to source.

---

## Pattern: adding settings to a module

```cpp
// MyModule_Settings.h
UCLASS(DefaultConfig, Config = CkFoundation)
class MYMODULE_API UCk_MyModule_Settings : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

private:
    UPROPERTY(Config, EditAnywhere, meta = (AllowPrivateAccess,
              ConsoleVariable = "ck.mymodule.enable_feature"))
    bool _EnableFeature = true;

public:
    CK_PROPERTY_GET(_EnableFeature);

    // UDeveloperSettings overrides (category/section for the UI):
    FName GetCategoryName() const override { return TEXT("CkFoundation"); }
    FName GetSectionName() const override  { return TEXT("MyModule"); }
};

// Access at runtime:
const auto* Settings = GetDefault<UCk_MyModule_Settings>();
const auto Enabled = Settings->Get_EnableFeature();
```

---

## Anti-patterns

1. Don't use `GConfig->GetString` directly in CkFoundation modules. Route through a `UCk_Plugin_ProjectSettings_UE` subclass — it provides type safety, editor UI, and CVar backing.
2. Don't put user preferences in project settings. Anything that varies per-machine or per-user (audio levels, quality settings) belongs in the user settings base.
3. Don't access settings in a tight per-frame loop. Cache the pointer (`GetDefault<T>()`) at processor init.

---

## See also
- `CkCVar/Claude.md` — for runtime-mutable CVars not backed by settings objects.
- UE `UDeveloperSettingsBackedByCVars` documentation.

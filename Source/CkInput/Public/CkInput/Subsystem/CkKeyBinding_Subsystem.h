#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Subsystems/LocalPlayerSubsystem.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkKeyBinding_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/** Delegate fired when a specific mapping's key changes. */
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCk_OnMappingKeyChanged, FName, MappingName, FKey, OldKey, FKey, NewKey);

// --------------------------------------------------------------------------------------------------------------------

/** Opaque handle for a keybind listener. Stores everything needed to unbind. */
USTRUCT(BlueprintType, meta = (HasNativeBreak, HasNativeMake))
struct CKINPUT_API FCk_Handle_KeybindListener
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_KeybindListener);

private:
    UPROPERTY()
    FName _MappingName = NAME_None;

    UPROPERTY()
    EPlayerMappableKeySlot _Slot = EPlayerMappableKeySlot::First;

    UPROPERTY()
    FCk_OnMappingKeyChanged _OnChanged;

public:
    CK_PROPERTY_GET(_MappingName);
    CK_PROPERTY_GET(_Slot);
    CK_PROPERTY_GET(_OnChanged);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Handle_KeybindListener, _MappingName, _Slot, _OnChanged);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Local-player subsystem that watches Enhanced Input User Settings for key
 * mapping changes and dispatches per-mapping-name delegates.
 *
 * Binds to OnSettingsChanged once. Each widget binds to a specific mapping
 * name — the subsystem caches the current key and only fires the delegate
 * when that specific mapping actually changes.
 */
UCLASS(NotBlueprintable)
class CKINPUT_API UCk_KeyBinding_Subsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_KeyBinding_Subsystem);

public:
    // --- USubsystem ---
    virtual void Initialize(FSubsystemCollectionBase& InCollection) override;
    virtual void Deinitialize() override;

public:
    /** Start listening for changes on a specific mapping. */
    auto
    BindTo_MappingKeyChanged(
        const FCk_Handle_KeybindListener& InListener) -> void;

    /** Stop listening for changes on a specific mapping. */
    auto UnbindFrom_MappingKeyChanged(
        const FCk_Handle_KeybindListener& InListener) -> void;

private:
    UFUNCTION()
    void OnSettingsChanged(UEnhancedInputUserSettings* InSettings);

private:
    struct FWatcherEntry
    {
        FName MappingName = NAME_None;
        EPlayerMappableKeySlot Slot = EPlayerMappableKeySlot::First;
        FKey CachedKey = FKey{EKeys::Invalid};
        FCk_OnMappingKeyChanged OnChanged;
    };

    TArray<FWatcherEntry> _Watchers;
    bool _Bound = false;
};

// --------------------------------------------------------------------------------------------------------------------

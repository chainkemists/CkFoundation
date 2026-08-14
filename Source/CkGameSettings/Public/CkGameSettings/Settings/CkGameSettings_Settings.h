#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Engine/EngineTypes.h>

#include "CkGameSettings_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GameSettings_StorageProvider_UE;
class UCk_GameSettingsUI_RowWidgetBase;
class USoundMix;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Game Settings"))
class CKGAMESETTINGS_API UCk_GameSettings_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_ProjectSettings_UE);

public:
    explicit UCk_GameSettings_ProjectSettings_UE(
        const FObjectInitializer& InObjectInitializer);

private:
    /**
     * Directories scanned for UCk_GameSettings_Collection_PDA assets at subsystem init.
     * Every setting definition found in these collections is auto-registered.
     */
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Registration",
              meta = (AllowPrivateAccess = true, ContentDir))
    TArray<FDirectoryPath> _CollectionScanPaths;

    /**
     * How long a stored CVar-bound value may wait for its CVar to register before it is
     * dropped loudly (ensure naming the key). The stored value itself is never deleted.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Apply",
              meta = (AllowPrivateAccess = true, ForceUnits = s, ConsoleVariable = "ck.GameSettings.DeferredApplyTimeoutSecs"))
    float _DeferredApplyTimeoutSeconds = 30.0f;

    /** Storage provider instantiated by the subsystem at init. Unset or unloadable falls back to the ordered-ini provider. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Storage",
              meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UCk_GameSettings_StorageProvider_UE> _StorageProviderClass;

    /** Registers the Audio pack (category volume settings driving SoundMix class overrides) at subsystem init. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Packs|Audio",
              meta = (AllowPrivateAccess = true))
    bool _EnableAudioPack = false;

    /** The SoundMix every Audio-pack category volume is applied through. The plugin ships no assets — supply the game's own. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Packs|Audio",
              meta = (AllowPrivateAccess = true, EditCondition = "_EnableAudioPack"))
    TSoftObjectPtr<USoundMix> _AudioMix;

    /** One Float volume setting is registered per category. The plugin ships no SoundClass assets — supply the game's own. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Packs|Audio",
              meta = (AllowPrivateAccess = true, EditCondition = "_EnableAudioPack", TitleProperty = "_SettingKey"))
    TArray<FCk_GameSettings_AudioCategory> _AudioCategories;

    /** Registers the Video pack (UGameUserSettings bridge: window mode, resolution, vsync, fps cap, scalability) at subsystem init. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Packs|Video",
              meta = (AllowPrivateAccess = true))
    bool _EnableVideoPack = false;

    /** Stamped as the category on every video.* setting the pack registers. Unset, the pack's rows
     *  land on the settings screen's "General" tab. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Packs|Video",
              meta = (AllowPrivateAccess = true, EditCondition = "_EnableVideoPack"))
    FGameplayTag _VideoPackCategoryTag;

    /** Per-value-type row widget class for the settings screen. Unset types use the built-in rows. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "UI",
              meta = (AllowPrivateAccess = true))
    TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>> _RowClassOverrides;

    /** Row widget class for options-present settings. Unset uses the built-in Select row. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "UI",
              meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase> _SelectRowClassOverride;

public:
    CK_PROPERTY_GET(_CollectionScanPaths);
    CK_PROPERTY_GET(_DeferredApplyTimeoutSeconds);
    CK_PROPERTY_GET(_StorageProviderClass);
    CK_PROPERTY_GET(_EnableAudioPack);
    CK_PROPERTY_GET(_AudioMix);
    CK_PROPERTY_GET(_AudioCategories);
    CK_PROPERTY_GET(_EnableVideoPack);
    CK_PROPERTY_GET(_VideoPackCategoryTag);
    CK_PROPERTY_GET(_RowClassOverrides);
    CK_PROPERTY_GET(_SelectRowClassOverride);
};

// --------------------------------------------------------------------------------------------------------------------

class CKGAMESETTINGS_API UCk_Utils_GameSettings_Settings_UE
{
public:
    static auto
    Get_CollectionScanPaths() -> const TArray<FDirectoryPath>&;

    static auto
    Get_DeferredApplyTimeoutSeconds() -> float;

    static auto
    Get_StorageProviderClass() -> const TSoftClassPtr<UCk_GameSettings_StorageProvider_UE>&;

    static auto
    Get_EnableAudioPack() -> bool;

    static auto
    Get_AudioMix() -> const TSoftObjectPtr<USoundMix>&;

    static auto
    Get_AudioCategories() -> const TArray<FCk_GameSettings_AudioCategory>&;

    static auto
    Get_EnableVideoPack() -> bool;

    static auto
    Get_VideoPackCategoryTag() -> const FGameplayTag&;

    static auto
    Get_RowClassOverrides() -> const TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>&;

    static auto
    Get_SelectRowClassOverride() -> const TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>&;
};

// --------------------------------------------------------------------------------------------------------------------

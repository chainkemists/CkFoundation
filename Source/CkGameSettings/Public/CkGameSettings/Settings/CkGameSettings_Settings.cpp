#include "CkGameSettings_Settings.h"

#include "CkGameSettings/Storage/CkGameSettings_IniStorageProvider.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_GameSettings_ProjectSettings_UE::
    UCk_GameSettings_ProjectSettings_UE(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    _StorageProviderClass = UCk_GameSettings_IniStorageProvider_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_CollectionScanPaths()
    -> const TArray<FDirectoryPath>&
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_CollectionScanPaths();
}

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_DeferredApplyTimeoutSeconds()
    -> float
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_DeferredApplyTimeoutSeconds();
}

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_StorageProviderClass()
    -> const TSoftClassPtr<UCk_GameSettings_StorageProvider_UE>&
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_StorageProviderClass();
}

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_EnableAudioPack()
    -> bool
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_EnableAudioPack();
}

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_AudioMix()
    -> const TSoftObjectPtr<USoundMix>&
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_AudioMix();
}

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_AudioCategories()
    -> const TArray<FCk_GameSettings_AudioCategory>&
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_AudioCategories();
}

auto
    UCk_Utils_GameSettings_Settings_UE::
    Get_EnableVideoPack()
    -> bool
{
    return GetDefault<UCk_GameSettings_ProjectSettings_UE>()->Get_EnableVideoPack();
}

// --------------------------------------------------------------------------------------------------------------------

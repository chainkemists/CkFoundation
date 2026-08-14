#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Engine/EngineTypes.h>

#include "CkGameSettings_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GameSettings_StorageProvider_UE;

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

public:
    CK_PROPERTY_GET(_CollectionScanPaths);
    CK_PROPERTY_GET(_DeferredApplyTimeoutSeconds);
    CK_PROPERTY_GET(_StorageProviderClass);
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
};

// --------------------------------------------------------------------------------------------------------------------

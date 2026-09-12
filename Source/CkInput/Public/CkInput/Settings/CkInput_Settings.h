#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkInput_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Input"))
class CKINPUT_API UCk_Input_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Input_ProjectSettings_UE);

private:
    /**
     * Directories to scan for UInputMappingContext assets at startup.
     * All IMCs found in these paths will have their remappable keys registered
     * with the Enhanced Input User Settings, ensuring key rebinding works
     * for all actions regardless of which mapping contexts are currently active.
     */
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Key Binding|Registration",
              meta = (AllowPrivateAccess = true, ContentDir))
    TArray<FDirectoryPath> _MappingContextScanPaths;

    /**
     * When enabled, raw gameplay input is recorded only while this application is active, the console is
     * closed, and the keyboard user directly owns this game viewport. Disable to restore the permissive
     * any-Slate-user viewport-focus behavior used before gameplay ownership was enforced.
     */
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Raw Input|Focus",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _RequireGameplayInputOwnership = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_MappingContextScanPaths);
    CK_PROPERTY(_RequireGameplayInputOwnership);
};

// --------------------------------------------------------------------------------------------------------------------

class CKINPUT_API UCk_Utils_Input_Settings_UE
{
public:
    static auto
    Get_MappingContextScanPaths() -> const TArray<FDirectoryPath>&;

    static auto
    Get_RequireGameplayInputOwnership() -> ECk_EnableDisable;
};

// --------------------------------------------------------------------------------------------------------------------

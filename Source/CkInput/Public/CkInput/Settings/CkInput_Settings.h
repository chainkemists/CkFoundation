#pragma once

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

public:
    CK_PROPERTY_GET(_MappingContextScanPaths);
};

// --------------------------------------------------------------------------------------------------------------------

class CKINPUT_API UCk_Utils_Input_Settings_UE
{
public:
    static auto
    Get_MappingContextScanPaths() -> const TArray<FDirectoryPath>&;
};

// --------------------------------------------------------------------------------------------------------------------

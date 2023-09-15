#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "CkMacros/CkMacros.h"

#include "CkNetTimeSync_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "NetTimeSync"))
class CKNET_API UCk_NetTimeSync_UserSettings_UE : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    explicit UCk_NetTimeSync_UserSettings_UE(const FObjectInitializer& ObjectInitializer);

private:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", meta = (AllowPrivateAccess = true))
    bool _EnableNetTimeSynchronization = true;

public:
    CK_PROPERTY_GET(_EnableNetTimeSynchronization);
};

// --------------------------------------------------------------------------------------------------------------------

class CKNET_API UCk_Utils_NetTimeSync_UserSettings_UE
{
public:
    static auto Get_EnableNetTimeSynchronization() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

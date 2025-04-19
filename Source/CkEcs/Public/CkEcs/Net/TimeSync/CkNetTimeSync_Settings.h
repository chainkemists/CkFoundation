#pragma once

#include <CoreMinimal.h>

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkNetTimeSync_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "NetTimeSync"))
class CKNET_API UCk_NetTimeSync_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

private:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", meta = (AllowPrivateAccess = true))
    bool _EnableNetTimeSynchronization = true;

public:
    CK_PROPERTY_GET(_EnableNetTimeSynchronization);
};

// --------------------------------------------------------------------------------------------------------------------

class CKNET_API UCk_Utils_NetTimeSync_Settings_UE
{
public:
    static auto Get_EnableNetTimeSynchronization() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

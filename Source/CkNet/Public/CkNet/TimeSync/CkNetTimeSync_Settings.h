#pragma once

#include <CoreMinimal.h>

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h"

#include "CkNetTimeSync_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "NetTimeSync"))
class CKNET_API UCk_NetTimeSync_UserSettings_UE : public UCk_EditorPerProject_UserSettings_UE
{
    GENERATED_BODY()

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

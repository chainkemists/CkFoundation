#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkActorRelay_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Actor Relay"))
class CKACTORRELAY_API UCk_ActorRelay_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorRelay_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Execution",
        meta = (AllowPrivateAccess = true,
            ToolTip = "How long to wait (in seconds) before warning about pending channel requests that couldn't be fulfilled due to missing channels"))
    float _PendingChannelTimeoutSeconds = 10.0f;

public:
    CK_PROPERTY_GET(_PendingChannelTimeoutSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKACTORRELAY_API UCk_Utils_ActorRelay_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorRelay_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorRelay|Settings")
    static float
    Get_PendingChannelTimeoutSeconds();
};

// --------------------------------------------------------------------------------------------------------------------

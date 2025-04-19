#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Time/CkTime.h"

#include "CkNetTimeSync_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKNET_API FCk_Request_NetTimeSync_NewSync
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_NetTimeSync_NewSync);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<APlayerController> _PlayerController;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _RoundTripTime;

public:
    CK_PROPERTY(_PlayerController)
    CK_PROPERTY(_RoundTripTime)
};

// --------------------------------------------------------------------------------------------------------------------
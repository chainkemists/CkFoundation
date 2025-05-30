#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkBallistics_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Ballistics_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKBALLISTICS_API FCk_Handle_Ballistics : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Ballistics); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Ballistics);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKBALLISTICS_API FCk_Fragment_Ballistics_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Ballistics_ParamsData);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Units="m/s"))
    float _InitialSpeed = 650.0f;

    // in meters per second
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Units="m/s"))
    FVector _TerminalVelocity = FVector{0.0f, 0.0f, 90.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Units="m/s^2"))
    FVector _Gravity = FVector{0.0f, 0.0f, -9.8f};

public:
    CK_PROPERTY(_InitialSpeed);
    CK_PROPERTY(_TerminalVelocity);
    CK_PROPERTY(_Gravity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKBALLISTICS_API FCk_Request_Ballistics_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Ballistics_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Ballistics_ExampleRequest);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Ballistics_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------

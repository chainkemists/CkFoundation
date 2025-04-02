#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkRaySense_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_RaySense_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum ECk_RaySense_Async : uint8
{
    Synchronous,
    Asynchronous
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRAYSENSE_API FCk_Handle_RaySense : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RaySense); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RaySense);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_Fragment_RaySense_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_RaySense_ParamsData);

private:
    int _DummyVar = 0;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_Request_RaySense_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_RaySense_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_RaySense_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_RaySense_HitResult : public FCk_Request_Base
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_RaySense_HitResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FVector _ImpactPoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FVector _ImpactNormal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TObjectPtr<AActor> _MaybeHitActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TObjectPtr<UPrimitiveComponent> _MaybeHitComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FCk_Handle _MaybeHitHandle;

public:
    CK_PROPERTY_GET(_ImpactPoint);
    CK_PROPERTY_GET(_ImpactNormal);

    CK_PROPERTY(_MaybeHitActor);
    CK_PROPERTY(_MaybeHitComponent);
    CK_PROPERTY(_MaybeHitHandle);

    CK_DEFINE_CONSTRUCTORS(FCk_RaySense_HitResult, _ImpactPoint, _ImpactNormal);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_RaySense_LineTrace,
    FCk_Handle_RaySense, InHandle,
    FCk_RaySense_HitResult, InHitResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_RaySense_LineTrace_MC,
    FCk_Handle_RaySense, InHandle,
    FCk_RaySense_HitResult, InHitResult);

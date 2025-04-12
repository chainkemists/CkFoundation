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
enum class ECk_RaySense_Async : uint8
{
    Synchronous,
    Asynchronous UMETA(DisplayName = "Asynchronous (NOT yet Supported)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RaySense_Async);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RaySense_CollisionQuality : uint8
{
    Sweep,
    Discrete
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RaySense_CollisionQuality);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRAYSENSE_API FCk_Handle_RaySense : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RaySense); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RaySense);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_RaySense_DataToIgnore
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RaySense_DataToIgnore);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TArray<TObjectPtr<AActor>> _ActorsToIgnore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TArray<TObjectPtr<UPrimitiveComponent>> _ComponentsToIgnore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TArray<FCk_Handle> _EntitiesToIgnore;

public:
    CK_PROPERTY(_ActorsToIgnore);
    CK_PROPERTY(_ComponentsToIgnore);
    CK_PROPERTY(_EntitiesToIgnore);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRAYSENSE_API FCk_Fragment_RaySense_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_RaySense_ParamsData);

private:
    // Discrete does NOT work for RaySense that do not have a shape
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_RaySense_CollisionQuality _CollisionQuality = ECk_RaySense_CollisionQuality::Sweep;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TEnumAsByte<ECollisionChannel> _CollisionChannel = ECollisionChannel::ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_RaySense_Async _Async = ECk_RaySense_Async::Synchronous;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FCk_RaySense_DataToIgnore _DataToIgnore;

public:
    CK_PROPERTY_GET(_CollisionQuality);
    CK_PROPERTY_GET(_CollisionChannel);
    CK_PROPERTY(_Async);
    CK_PROPERTY(_DataToIgnore);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RaySense_ParamsData, _CollisionQuality, _CollisionChannel);
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

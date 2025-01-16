#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPhysics/Public/CkPhysics/CkPhysics_Common.h"

#include <GameplayTagContainer.h>

#include "CkProbe_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Probe_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

// TODO: move to a more appropriate location
UENUM(BlueprintType)
enum class ECk_MotionType : uint8
{
    Static = 0,
    Kinematic,
    Dynamic,

    Count UMETA(Hidden)
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MotionType);
ENUM_RANGE_BY_COUNT(ECk_MotionType, ECk_MotionType::Count);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSPATIALQUERY_API FCk_Handle_Probe : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Probe); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Probe);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_Probe_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Probe_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_ShapeType _Shape = ECk_ShapeType::Box;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_MotionType _MotionType = ECk_MotionType::Static;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _HalfExtents = FVector(1.0f, 1.0f, 1.0f);

public:
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_HalfExtents);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_MultipleProbe_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleProbe_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Probe_ParamsData> _ProbeParams;

public:
    CK_PROPERTY_GET(_ProbeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleProbe_ParamsData, _ProbeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Probe_BeginOverlap : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Probe_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Probe_BeginOverlap);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ContactPoints);
    CK_PROPERTY_GET(_ContactNormal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Probe_BeginOverlap, _OtherEntity, _ContactPoints, _ContactNormal);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Probe_EndOverlap : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Probe_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Probe_EndOverlap);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

public:
    CK_PROPERTY_GET(_OtherEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Probe_EndOverlap, _OtherEntity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_Payload_OnBeginOverlap
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_Payload_OnBeginOverlap);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ContactPoints);
    CK_PROPERTY_GET(_ContactNormal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_Payload_OnBeginOverlap, _OtherEntity, _ContactPoints, _ContactNormal);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnBeginOverlap,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnBeginOverlap, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnBeginOverlap_MC,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnBeginOverlap, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_Payload_OnEndOverlap
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_Payload_OnEndOverlap);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

public:
    CK_PROPERTY_GET(_OtherEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_Payload_OnEndOverlap, _OtherEntity);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnEndOverlap,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnEndOverlap, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnEndOverlap_MC,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnEndOverlap, InPayload);


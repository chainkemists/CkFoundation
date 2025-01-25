#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPhysics/Public/CkPhysics/CkPhysics_Common.h"

#include <GameplayTagContainer.h>

#include "CkProbe_Fragment_Data.generated.h"
// --------------------------------------------------------------------------------------------------------------------

CKSPATIALQUERY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Probe);

// --------------------------------------------------------------------------------------------------------------------

// TODO: move to a more appropriate location
UENUM(BlueprintType)
enum class ECk_MotionType : uint8
{
    Static = 0,
    Kinematic,
    Dynamic,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MotionType);

// --------------------------------------------------------------------------------------------------------------------

// TODO: move to a more appropriate location
UENUM(BlueprintType)
enum class ECk_MotionQuality : uint8
{
    // FAST - use this for most Probes
    Discrete UMETA(DisplayName = "Discrete"),
    // SLOWER - avoid using this unless continuous collision detection is needed
    LinearCast UMETA(DisplayName = "LinearCast (CCD)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MotionQuality);

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
    ECk_MotionType _MotionType = ECk_MotionType::Static;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_MotionQuality _MotionQuality = ECk_MotionQuality::Discrete;

public:
    CK_PROPERTY(_MotionType);
    CK_PROPERTY(_MotionQuality);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_DebugInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_DebugInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _DebugColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _DebugOverlapColor = FLinearColor::Yellow;

public:
    CK_PROPERTY(_LineThickness);
    CK_PROPERTY(_DebugColor);
    CK_PROPERTY(_DebugOverlapColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_OverlapInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_OverlapInfo);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal;

public:
    CK_PROPERTY(_OtherEntity);
    CK_PROPERTY_SET(_ContactPoints);
    CK_PROPERTY_SET(_ContactNormal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_OverlapInfo, _OtherEntity);
};

auto CKSPATIALQUERY_API GetTypeHash(const FCk_Probe_OverlapInfo& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Probe_BeginOverlap : public FCk_Request_Base
{
    GENERATED_BODY()

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

USTRUCT()
struct FCk_Request_Probe_OverlapPersisted : public FCk_Request_Probe_BeginOverlap
{
    GENERATED_BODY()

    using FCk_Request_Probe_BeginOverlap::FCk_Request_Probe_BeginOverlap;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Probe_EndOverlap : public FCk_Request_Base
{
    GENERATED_BODY()

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
struct CKSPATIALQUERY_API FCk_Probe_Payload_OnOverlapPersisted : public FCk_Probe_Payload_OnBeginOverlap
{
    GENERATED_BODY()

    using FCk_Probe_Payload_OnBeginOverlap::FCk_Probe_Payload_OnBeginOverlap;
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnOverlapPersisted,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnOverlapPersisted, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnOverlapPersisted_MC,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnOverlapPersisted, InPayload);

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

// --------------------------------------------------------------------------------------------------------------------

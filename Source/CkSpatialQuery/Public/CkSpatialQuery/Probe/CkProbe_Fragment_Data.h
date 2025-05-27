#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkPhysics/Public/CkPhysics/CkPhysics_Common.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

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
enum class ECk_BackFaceMode : uint8
{
    IgnoreBackFaces,
    CollideWithBackFaces,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BackFaceMode);

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

UENUM(BlueprintType)
enum class ECk_ProbeResponse_Policy : uint8
{
    Notify,
    Silent
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProbeResponse_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PhysicalMaterialSource : uint8
{
    Direct UMETA(DisplayName = "User Specified"),
    Trace UMETA(DisplayName = "Trace Between Positions (Not Supported Yet)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PhysicalMaterialSource);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSPATIALQUERY_API FCk_Handle_Probe : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Probe); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Probe);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_SurfaceInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_SurfaceInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_PhysicalMaterialSource _PhysicalMaterialSource = ECk_PhysicalMaterialSource::Direct;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, EditCondition = "_PhysicalMaterialSource == ECk_PhysicalMaterialSource::Direct"))
    TObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

public:
    CK_PROPERTY(_PhysicalMaterialSource);
    CK_PROPERTY(_PhysicalMaterial);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_Probe_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Probe_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTag _ProbeName = TAG_Probe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_ProbeResponse_Policy _ResponsePolicy = ECk_ProbeResponse_Policy::Notify;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Probe",
            EditCondition = "_ResponsePolicy == ECk_ProbeResponse_Policy::Notify"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_MotionType _MotionType = ECk_MotionType::Static;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_MotionQuality _MotionQuality = ECk_MotionQuality::Discrete;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Probe_SurfaceInfo _SurfaceInfo;

public:
    CK_PROPERTY_GET(_ProbeName);
    CK_PROPERTY(_ResponsePolicy);
    CK_PROPERTY(_Filter);
    CK_PROPERTY(_MotionType);
    CK_PROPERTY(_MotionQuality);
    CK_PROPERTY(_SurfaceInfo);
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
              meta = (AllowPrivateAccess = true, UIMin = 1.0f, ClampMin = 1.0f))
    float _LineThickness = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _OverlapColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _DisabledColor = FLinearColor::Gray;

public:
    CK_PROPERTY(_LineThickness);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_OverlapColor);
    CK_PROPERTY(_DisabledColor);
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
    FCk_Handle_Probe _OtherEntity;

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
    FCk_Handle_Probe _OtherEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ContactPoints);
    CK_PROPERTY_GET(_ContactNormal);
    CK_PROPERTY_GET(_PhysicalMaterial);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Probe_BeginOverlap, _OtherEntity, _ContactPoints, _ContactNormal, _PhysicalMaterial);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Request_Probe_OverlapUpdated : public FCk_Request_Probe_BeginOverlap
{
    GENERATED_BODY()

    using FCk_Request_Probe_BeginOverlap::FCk_Request_Probe_BeginOverlap;

public:
    explicit
    FCk_Request_Probe_OverlapUpdated(
        FCk_Request_Probe_BeginOverlap InOther);
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
    FCk_Handle_Probe _OtherEntity;

public:
    CK_PROPERTY_GET(_OtherEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Probe_EndOverlap, _OtherEntity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Probe_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Probe_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Probe_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_RayCast_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_RayCast_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Probe _Probe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _HitLocation;

    // not normalized for performance
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NormalDirLen;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartPos;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _EndPos;

public:
    CK_PROPERTY_GET(_Probe);
    CK_PROPERTY_GET(_HitLocation);
    CK_PROPERTY_GET(_NormalDirLen);
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_EndPos);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_RayCast_Result, _Probe, _HitLocation, _NormalDirLen, _StartPos, _EndPos);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_RayCast_Settings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_RayCast_Settings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeTriangles = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeConvex = ECk_BackFaceMode::IgnoreBackFaces;

public:
    CK_PROPERTY_GET(_Filter);
    CK_PROPERTY(_BackFaceModeTriangles);
    CK_PROPERTY(_BackFaceModeConvex);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_RayCast_Settings, _Filter);
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ContactPoints);
    CK_PROPERTY_GET(_ContactNormal);
    CK_PROPERTY_GET(_PhysicalMaterial);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_Payload_OnBeginOverlap, _OtherEntity, _ContactPoints, _ContactNormal, _PhysicalMaterial);
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
struct CKSPATIALQUERY_API FCk_Probe_Payload_OnOverlapUpdated : public FCk_Probe_Payload_OnBeginOverlap
{
    GENERATED_BODY()

    using FCk_Probe_Payload_OnBeginOverlap::FCk_Probe_Payload_OnBeginOverlap;
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnOverlapUpdated,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnOverlapUpdated, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnOverlapUpdated_MC,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnOverlapUpdated, InPayload);

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

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_Payload_OnEnableDisable
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_Payload_OnEnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_Payload_OnEnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnEnableDisable,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnEnableDisable, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnEnableDisable_MC,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnEnableDisable, InPayload);

// --------------------------------------------------------------------------------------------------------------------

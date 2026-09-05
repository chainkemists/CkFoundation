#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkJolt/CkJolt_Common.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"

#include "CkPhysics/Public/CkPhysics/CkPhysics_Common.h"

#include "CkShapes/CkShapes_Common.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

#include "CkProbe_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKSPATIALQUERY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Probe);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProbeTrace_Policy : uint8
{
    Single,
    Multi
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProbeTrace_Policy);

// --------------------------------------------------------------------------------------------------------------------

/// Whether a ProbeTrace also sees NON-probe Jolt bodies (baked static world, JoltBodies), and what
/// it does with them. Opt-in per call: the default keeps a trace probe-only, which every existing
/// caller (EQS line-of-sight, crowd counts, the claw-machine cabinet scan) depends on.
UENUM(BlueprintType)
enum class ECk_ProbeTrace_WorldHitPolicy : uint8
{
    // Non-probe bodies are invisible to the trace.
    Ignore,

    // The nearest passing world hit truncates: probes beyond it are not returned and not
    // overlap-fired; the world hit itself IS returned as the final element.
    Blocking,

    // World hits interleave with probe hits in fraction order; nothing is truncated.
    Reported
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProbeTrace_WorldHitPolicy);

// --------------------------------------------------------------------------------------------------------------------

/// Discriminates what a single trace result refers to. `_Probe` is populated for Probe hits ONLY;
/// on World hits it is deliberately INVALID.
UENUM(BlueprintType)
enum class ECk_ProbeTrace_HitKind : uint8
{
    Probe,
    World
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProbeTrace_HitKind);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProbeResponse_Policy : uint8
{
    // Receives physical Probe overlap callbacks when its filter admits the other Probe.
    Notify = 0,

    // Does not receive physical Probe overlap callbacks, but remains a possible target for an
    // admitting Notify Probe and for ProbeTrace queries.
    Silent = 1
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ProbeResponse_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Probe_ContactParticipation : uint8
{
    // The Probe is eligible for physical Probe contacts, subject to response and filter admission.
    PhysicalContacts = 0,

    // The Probe remains visible to ProbeTrace queries but rejects physical Probe contacts in both directions.
    QueryOnly = 1
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Probe_ContactParticipation);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Probe_PersistContacts : uint8
{
    Disabled,
    Enabled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Probe_PersistContacts);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Probe_ContextOverlapPolicy : uint8
{
    // Only overlap with probes that have a DIFFERENT context owner
    DifferentContextOnly,

    // Only overlap with probes that have the SAME context owner
    SameContextOnly,

    // Overlap with any probe regardless of context
    Any
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Probe_ContextOverlapPolicy);

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

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSPATIALQUERY_API FCk_Handle_ProbeTrace : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ProbeTrace); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ProbeTrace);

// --------------------------------------------------------------------------------------------------------------------

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
    TWeakObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

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

    // QueryOnly is an explicit query-target contract: it remains traceable but rejects every physical
    // Probe contact pair. It is deliberately separate from directional Notify/Silent callback policy.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Probe_ContactParticipation _ContactParticipation = ECk_Probe_ContactParticipation::PhysicalContacts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "Probe",
            EditCondition = "_ResponsePolicy == ECk_ProbeResponse_Policy::Notify"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Probe_ContextOverlapPolicy _ContextOverlapPolicy = ECk_Probe_ContextOverlapPolicy::DifferentContextOnly;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_MotionType _MotionType = ECk_MotionType::Static;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_MotionQuality _MotionQuality = ECk_MotionQuality::Discrete;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Probe_SurfaceInfo _SurfaceInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Probe_PersistContacts _PersistContacts = ECk_Probe_PersistContacts::Disabled;

public:
    CK_PROPERTY_GET(_ProbeName);
    CK_PROPERTY(_ResponsePolicy);
    CK_PROPERTY(_ContactParticipation);
    CK_PROPERTY(_Filter);
    CK_PROPERTY(_ContextOverlapPolicy);
    CK_PROPERTY(_StartingState);
    CK_PROPERTY(_MotionType);
    CK_PROPERTY(_MotionQuality);
    CK_PROPERTY(_SurfaceInfo);
    CK_PROPERTY(_PersistContacts);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Probe_ParamsData, _ProbeName);
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
    float _LineThickness = 1.0f;

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
    FCk_Handle _OtherEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY(_ContactPoints);
    CK_PROPERTY(_ContactNormal);

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
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Probe_BeginOverlap);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

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
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Probe_EndOverlap);

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
struct CKSPATIALQUERY_API FCk_Request_Probe_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Probe_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Probe_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

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
    FVector _HitLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NormalDirLen = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _EndPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_HitKind _HitKind = ECk_ProbeTrace_HitKind::Probe;

    // Probe hits: the probe entity. World hits: the body's attribution entity (JoltStaticActor or
    // the JoltBody's own entity), which MAY be invalid — a body with no entity is still a real hit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _HitEntity;

    // The TRUE surface normal at the hit. Distinct from _NormalDirLen, which is (StartPos - HitLocation).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _SurfaceNormal = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Fraction = 0.0f;

public:
    CK_PROPERTY_GET(_Probe);
    CK_PROPERTY_GET(_HitLocation);
    CK_PROPERTY_GET(_NormalDirLen);
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_EndPos);
    CK_PROPERTY(_HitKind);
    CK_PROPERTY(_HitEntity);
    CK_PROPERTY(_SurfaceNormal);
    CK_PROPERTY(_Fraction);

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
    FVector _StartPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FVector _EndPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeTriangles = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeConvex = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_WorldHitPolicy _WorldHitPolicy = ECk_ProbeTrace_WorldHitPolicy::Ignore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_WorldHitPolicy != ECk_ProbeTrace_WorldHitPolicy::Ignore"))
    FCk_Jolt_QueryFilter _WorldFilter;

    // Whether filter-matching PROBE hits ping Begin/EndOverlap into the probes they hit. World hits
    // never fire overlaps regardless. Silent exists because a query like a weapon aim sweep wants
    // the hit list without the side-effects - callers needing that were bypassing this API to get it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeResponse_Policy _OverlapNotifyPolicy = ECk_ProbeResponse_Policy::Notify;

    // Hits whose resolved entity (probe entity or world attribution entity) is listed are dropped
    // before ordering and blocking. Own-collision-pill exclusion for weapon traces.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _IgnoredEntities;

public:
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_EndPos);
    CK_PROPERTY_GET(_Filter);
    CK_PROPERTY(_BackFaceModeTriangles);
    CK_PROPERTY(_BackFaceModeConvex);
    CK_PROPERTY(_WorldHitPolicy);
    CK_PROPERTY(_WorldFilter);
    CK_PROPERTY(_OverlapNotifyPolicy);
    CK_PROPERTY(_IgnoredEntities);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_RayCast_Settings, _StartPos, _EndPos, _Filter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_ShapeCast_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ShapeCast_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Probe _Probe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _HitLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NormalDirLen = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _EndPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Fraction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_HitKind _HitKind = ECk_ProbeTrace_HitKind::Probe;

    // Probe hits: the probe entity. World hits: the body's attribution entity (JoltStaticActor or
    // the JoltBody's own entity), which MAY be invalid — a body with no entity is still a real hit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _HitEntity;

    // The TRUE surface normal at the hit. Distinct from _NormalDirLen, which is (StartPos - HitLocation).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _SurfaceNormal = FVector::UpVector;

public:
    CK_PROPERTY_GET(_Probe);
    CK_PROPERTY_GET(_HitLocation);
    CK_PROPERTY_GET(_NormalDirLen);
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_EndPos);
    CK_PROPERTY_GET(_Fraction);
    CK_PROPERTY(_HitKind);
    CK_PROPERTY(_HitEntity);
    CK_PROPERTY(_SurfaceNormal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ShapeCast_Result, _Probe, _HitLocation, _NormalDirLen, _StartPos, _EndPos, _Fraction);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_ShapeCast_Settings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ShapeCast_Settings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _EndPos = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeTriangles = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeConvex = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_WorldHitPolicy _WorldHitPolicy = ECk_ProbeTrace_WorldHitPolicy::Ignore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_WorldHitPolicy != ECk_ProbeTrace_WorldHitPolicy::Ignore"))
    FCk_Jolt_QueryFilter _WorldFilter;

    // Whether filter-matching PROBE hits ping Begin/EndOverlap into the probes they hit. World hits
    // never fire overlaps regardless. Silent exists because a query like a weapon aim sweep wants
    // the hit list without the side-effects - callers needing that were bypassing this API to get it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeResponse_Policy _OverlapNotifyPolicy = ECk_ProbeResponse_Policy::Notify;

    // Hits whose resolved entity (probe entity or world attribution entity) is listed are dropped
    // before ordering and blocking. Own-collision-pill exclusion for weapon traces.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _IgnoredEntities;

public:
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_EndPos);
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_Filter);
    CK_PROPERTY(_BackFaceModeTriangles);
    CK_PROPERTY(_BackFaceModeConvex);
    CK_PROPERTY(_WorldHitPolicy);
    CK_PROPERTY(_WorldFilter);
    CK_PROPERTY(_OverlapNotifyPolicy);
    CK_PROPERTY(_IgnoredEntities);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ShapeCast_Settings, _StartPos, _EndPos, _Shape, _Filter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_RayCastPersistent_Settings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_RayCastPersistent_Settings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FCk_Handle_Transform _StartPos;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FVector _DirectionAndLength = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeTriangles = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeConvex = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_Policy _TracePolicy = ECk_ProbeTrace_Policy::Single;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_WorldHitPolicy _WorldHitPolicy = ECk_ProbeTrace_WorldHitPolicy::Ignore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_WorldHitPolicy != ECk_ProbeTrace_WorldHitPolicy::Ignore"))
    FCk_Jolt_QueryFilter _WorldFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _IgnoredEntities;

public:
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_DirectionAndLength);
    CK_PROPERTY_GET(_Filter);
    CK_PROPERTY(_BackFaceModeTriangles);
    CK_PROPERTY(_BackFaceModeConvex);
    CK_PROPERTY(_TracePolicy);
    CK_PROPERTY(_WorldHitPolicy);
    CK_PROPERTY(_WorldFilter);
    CK_PROPERTY(_IgnoredEntities);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_RayCastPersistent_Settings, _StartPos, _DirectionAndLength, _Filter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_ShapeCastPersistent_Settings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_ShapeCastPersistent_Settings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _StartPos;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _DirectionAndLength = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AnyShape _Shape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Probe"))
    FGameplayTagContainer _Filter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeTriangles = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_BackFaceMode _BackFaceModeConvex = ECk_BackFaceMode::IgnoreBackFaces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_Policy _TracePolicy = ECk_ProbeTrace_Policy::Single;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ProbeTrace_WorldHitPolicy _WorldHitPolicy = ECk_ProbeTrace_WorldHitPolicy::Ignore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_WorldHitPolicy != ECk_ProbeTrace_WorldHitPolicy::Ignore"))
    FCk_Jolt_QueryFilter _WorldFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _IgnoredEntities;

public:
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_DirectionAndLength);
    CK_PROPERTY_GET(_Shape);
    CK_PROPERTY_GET(_Filter);
    CK_PROPERTY(_BackFaceModeTriangles);
    CK_PROPERTY(_BackFaceModeConvex);
    CK_PROPERTY(_TracePolicy);
    CK_PROPERTY(_WorldHitPolicy);
    CK_PROPERTY(_WorldFilter);
    CK_PROPERTY(_IgnoredEntities);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_ShapeCastPersistent_Settings, _StartPos, _DirectionAndLength, _Shape, _Filter);
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
    FVector _ContactNormal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

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

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ProbeTrace_OnBeginOverlap,
    FCk_Handle_ProbeTrace, InHandle,
    FCk_Probe_Payload_OnBeginOverlap, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Probe_Payload_OnOverlapUpdated
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Probe_Payload_OnOverlapUpdated);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _OtherEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FVector> _ContactPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ContactNormal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPhysicalMaterial> _PhysicalMaterial;

public:
    CK_PROPERTY_GET(_OtherEntity);
    CK_PROPERTY_GET(_ContactPoints);
    CK_PROPERTY_GET(_ContactNormal);
    CK_PROPERTY_GET(_PhysicalMaterial);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Probe_Payload_OnOverlapUpdated, _OtherEntity, _ContactPoints, _ContactNormal, _PhysicalMaterial);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Probe_OnOverlapUpdated,
    FCk_Handle_Probe, InHandle,
    FCk_Probe_Payload_OnOverlapUpdated, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ProbeTrace_OnOverlapUpdated,
    FCk_Handle_ProbeTrace, InHandle,
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

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ProbeTrace_OnEndOverlap,
    FCk_Handle_ProbeTrace, InHandle,
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
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

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

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_ProbeTrace_Payload_OnWorldHit
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ProbeTrace_Payload_OnWorldHit);

private:
    // MAY be invalid: a Jolt body with no owning entity is still a real world contact.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _WorldEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _HitLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _SurfaceNormal = FVector::UpVector;

public:
    CK_PROPERTY_GET(_WorldEntity);
    CK_PROPERTY_GET(_HitLocation);
    CK_PROPERTY_GET(_SurfaceNormal);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ProbeTrace_Payload_OnWorldHit, _WorldEntity, _HitLocation, _SurfaceNormal);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ProbeTrace_OnWorldHit,
    FCk_Handle_ProbeTrace, InHandle,
    FCk_ProbeTrace_Payload_OnWorldHit, InPayload);

// --------------------------------------------------------------------------------------------------------------------

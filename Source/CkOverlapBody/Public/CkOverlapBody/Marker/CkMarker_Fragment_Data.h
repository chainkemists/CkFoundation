#pragma once

#include "CkActor/ActorModifier/CkActorModifier_Fragment_Data.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include "CkNet/CkNet_Common.h"

#include "CkOverlapBody/CkOverlapBody_Common.h"

#include "CkPhysics/CkPhysics_Common.h"

#include <GameplayTagContainer.h>

#include "CkMarker_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECk_Marker_AttachmentPolicy : uint8
{
    None = 0 UMETA(Hidden),

    /* The bone's rotation will be used */
    UseBoneRotation = 1 << 0,

    /* The bone's position will be used */
    UseBonePosition = 1 << 1,
};

ENUM_CLASS_FLAGS(ECk_Marker_AttachmentPolicy)
ENABLE_ENUM_BITWISE_OPERATORS(ECk_Marker_AttachmentPolicy);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Marker_AttachmentPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKOVERLAPBODY_API FCk_Handle_Marker : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Marker); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Marker);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_BasicInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_BasicInfo);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle_Marker _Marker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Owner;

public:
    CK_PROPERTY_GET(_Marker);
    CK_PROPERTY_GET(_Owner);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_BasicInfo, _Marker, _Owner);
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Marker_BasicInfo& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_DebugInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_DebugInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FColor _DebugLineColor = FColor::White;

public:
    CK_PROPERTY(_LineThickness);
    CK_PROPERTY_GET(_DebugLineColor);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_DebugInfo, _DebugLineColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_PhysicsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_PhysicsInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_CollisionDetectionType _CollisionType = ECk_CollisionDetectionType::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_NavigationEffect _NavigationEffect = ECk_NavigationEffect::DoesNotAffectNavigation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComponentOverlapBehavior _OverlapBehavior = ECk_ComponentOverlapBehavior::OtherActorComponents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCollisionProfileName _CollisionProfileName  = FCollisionProfileName{TEXT("CkMarker")};

public:
    CK_PROPERTY(_CollisionType)
    CK_PROPERTY(_NavigationEffect);
    CK_PROPERTY(_OverlapBehavior);
    CK_PROPERTY_GET(_CollisionProfileName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_PhysicsInfo, _CollisionProfileName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_AttachmentInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_AttachmentInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ActorComponent_AttachmentPolicy _AttachmentType = ECk_ActorComponent_AttachmentPolicy::Attach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Bitmask, BitmaskEnum = "/Script/CkOverlapBody.ECk_Marker_AttachmentPolicy"))
    int32  _AttachmentPolicyFlags = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_AttachmentPolicyFlags != 0"))
    FName _BoneName;

public:
    auto Get_AttachmentPolicy() const -> ECk_Marker_AttachmentPolicy;

public:
    CK_PROPERTY(_AttachmentType)
    CK_PROPERTY(_BoneName);
    CK_PROPERTY_SET(_AttachmentPolicyFlags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_ShapeInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_ShapeInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ShapeDimensions _ShapeDimensions;

public:
    CK_PROPERTY_GET(_ShapeDimensions)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_ShapeInfo, _ShapeDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Request_Marker_EnableDisable
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Marker_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Marker_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Marker_OnEnableDisable,
    FCk_Marker_BasicInfo, InMarkerBasicInfo,
    ECk_EnableDisable, InEnableDisable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Marker_OnEnableDisable_MC,
    FCk_Marker_BasicInfo, InMarkerBasicInfo,
    ECk_EnableDisable, InEnableDisable);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Fragment_Marker_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Marker_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _MarkerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Marker_ShapeInfo _ShapeParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Marker_PhysicsInfo _PhysicsParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Marker_AttachmentInfo _AttachmentParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _RelativeTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Net_ReplicationType _ReplicationType = ECk_Net_ReplicationType::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _ShowDebug = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ShowDebug"))
    FCk_Marker_DebugInfo _DebugParams;

public:
    CK_PROPERTY_GET(_MarkerName);
    CK_PROPERTY_GET(_ShapeParams);
    CK_PROPERTY(_PhysicsParams);
    CK_PROPERTY(_AttachmentParams);
    CK_PROPERTY(_RelativeTransform);
    CK_PROPERTY(_ShowDebug);
    CK_PROPERTY(_DebugParams);
    CK_PROPERTY(_StartingState);
    CK_PROPERTY(_ReplicationType);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Marker_ParamsData, _MarkerName, _ShapeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Fragment_MultipleMarker_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleMarker_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_MarkerName"))
    TArray<FCk_Fragment_Marker_ParamsData> _MarkerParams;

public:
    CK_PROPERTY_GET(_MarkerParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleMarker_ParamsData, _MarkerParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKOVERLAPBODY_API UCk_Marker_ActorComponent_Box_UE
    : public UCk_OverlapBody_ActorComponent_Box_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Marker_ActorComponent_Box_UE);

public:
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    auto Get_Type() const -> ECk_OverlapBody_Type override;
    auto Get_OwningEntity() const -> const FCk_Handle& override;

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKOVERLAPBODY_API UCk_Marker_ActorComponent_Sphere_UE
    : public UCk_OverlapBody_ActorComponent_Sphere_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Marker_ActorComponent_Sphere_UE);

public:
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    auto Get_Type() const -> ECk_OverlapBody_Type override;
    auto Get_OwningEntity() const -> const FCk_Handle& override;

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKOVERLAPBODY_API UCk_Marker_ActorComponent_Capsule_UE
    : public UCk_OverlapBody_ActorComponent_Capsule_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Marker_ActorComponent_Capsule_UE);

public:
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    auto Get_Type() const -> ECk_OverlapBody_Type override;
    auto Get_OwningEntity() const -> const FCk_Handle& override;

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------

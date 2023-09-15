#pragma once

#include "CkOverlapBody/CkOverlapBody_Common.h"

#include "CkActor/ActorModifier/CkActorModifier_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"

#include "CkFormat/CkFormat.h"

#include "CkPhysics/CkPhysics_Common.h"

#include <GameplayTagContainer.h>
#include <Components/ShapeComponent.h>

#include "CkNet/CkNet_Common.h"
#include "CkMarker_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Marker_BoneTransform_UsagePolicy : uint8
{
    IgnoreBoneTransform,

    UseBoneTransform,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Marker_BoneTransform_UsagePolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Marker_BoneTransform_PositionPolicy : uint8
{
    /* The bone's position will not be used */
    None = 0,

    /* The bone's position will be used */
    UseBonePosition,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Marker_BoneTransform_PositionPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Marker_BoneTransform_RotationPolicy : uint8
{
    /* The bone's rotation will not be used */
    None = 0,

    /* The bone's rotation will be used */
    UseBoneRotation,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Marker_BoneTransform_RotationPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_BasicDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_BasicDetails);

public:
    FCk_Marker_BasicDetails() = default;
    FCk_Marker_BasicDetails(
        FGameplayTag InMarkerName,
        FCk_Handle InMarkerEntity,
        FCk_EntityOwningActor_BasicDetails InMarkerAttachedEntityAndActor);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _MarkerName;

    UPROPERTY(Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _MarkerEntity;

    // Represents the Entity/Actor that the Marker ActorComp is attached to. Different from the Marker Entity itself
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_EntityOwningActor_BasicDetails _MarkerAttachedEntityAndActor;

public:
    CK_PROPERTY_GET(_MarkerName);
    CK_PROPERTY_GET(_MarkerEntity);
    CK_PROPERTY_GET(_MarkerAttachedEntityAndActor);
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Marker_BasicDetails& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_DebugInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_DebugInfo);

public:
    FCk_Marker_DebugInfo() = default;
    FCk_Marker_DebugInfo(
        float InLineThickness,
        FColor InDebugLineColor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FColor _DebugLineColor = FColor::White;

public:
    CK_PROPERTY_GET(_LineThickness);
    CK_PROPERTY_GET(_DebugLineColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_PhysicsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_PhysicsInfo);

public:
    FCk_Marker_PhysicsInfo() = default;
    FCk_Marker_PhysicsInfo(
        ECk_CollisionDetectionType   InCollisionType,
        ECk_NavigationEffect         InNavigationEffect,
        ECk_ComponentOverlapBehavior InOverlapBehavior,
        FName                        InCollisionProfileName);

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
    FName _CollisionProfileName = TEXT("CkMarker");

public:
    CK_PROPERTY_GET(_CollisionType)
    CK_PROPERTY_GET(_NavigationEffect);
    CK_PROPERTY_GET(_OverlapBehavior);
    CK_PROPERTY_GET(_CollisionProfileName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_AttachmentInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_AttachmentInfo);

public:
    FCk_Marker_AttachmentInfo() = default;
    FCk_Marker_AttachmentInfo(
        ECk_ActorComponent_AttachmentPolicy     InAttachmentType,
        ECk_Marker_BoneTransform_UsagePolicy    InUseBoneTransformOrNot,
        FName                                   InBoneName,
        ECk_Marker_BoneTransform_PositionPolicy InUseBonePosition,
        ECk_Marker_BoneTransform_RotationPolicy InUseBoneRotation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ActorComponent_AttachmentPolicy _AttachmentType = ECk_ActorComponent_AttachmentPolicy::Attach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Marker_BoneTransform_UsagePolicy _UseBoneTransformOrNot = ECk_Marker_BoneTransform_UsagePolicy::IgnoreBoneTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_UseBoneTransformOrNot == ECk_Marker_BoneTransform_UsagePolicy::UseBoneTransform"))
    FName _BoneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_UseBoneTransformOrNot == ECk_Marker_BoneTransform_UsagePolicy::UseBoneTransform"))
    ECk_Marker_BoneTransform_PositionPolicy _UseBonePosition = ECk_Marker_BoneTransform_PositionPolicy::UseBonePosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_UseBoneTransformOrNot == ECk_Marker_BoneTransform_UsagePolicy::UseBoneTransform"))
    ECk_Marker_BoneTransform_RotationPolicy _UseBoneRotation = ECk_Marker_BoneTransform_RotationPolicy::UseBoneRotation;

public:
    CK_PROPERTY_GET(_AttachmentType)
    CK_PROPERTY_GET(_UseBoneTransformOrNot);
    CK_PROPERTY_GET(_BoneName);
    CK_PROPERTY_GET(_UseBonePosition);
    CK_PROPERTY_GET(_UseBoneRotation);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Marker_ShapeInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_ShapeInfo);

public:
    FCk_Marker_ShapeInfo() = default;
    explicit FCk_Marker_ShapeInfo(
        FCk_ShapeDimensions InShapeDimensions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ShapeDimensions _ShapeDimensions;

public:
    CK_PROPERTY_GET(_ShapeDimensions)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Request_Marker_EnableDisable
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Marker_EnableDisable);

public:
    FCk_Request_Marker_EnableDisable() = default;
    explicit FCk_Request_Marker_EnableDisable(ECk_EnableDisable InEnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable;

public:
    CK_PROPERTY_GET(_EnableDisable)
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Marker_OnEnableDisable,
    FCk_Handle, InHandle,
    FGameplayTag, InMarkerName,
    ECk_EnableDisable, InEnableDisable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_Marker_OnEnableDisable_MC,
    FCk_Handle, InHandle,
    FGameplayTag, InMarkerName,
    ECk_EnableDisable, InEnableDisable);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Fragment_Marker_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Marker_ParamsData);

public:
    FCk_Fragment_Marker_ParamsData() = default;
    FCk_Fragment_Marker_ParamsData(
        FGameplayTag              InMarkerName,
        FCk_Marker_ShapeInfo      InShapeParams,
        FCk_Marker_PhysicsInfo    InPhysicsParams,
        FCk_Marker_AttachmentInfo InAttachmentParams,
        FTransform                InRelativeTransform,
        ECk_EnableDisable         InStartingState,
        ECk_Net_ReplicationType   InReplicationType,
        bool                      InShowDebug,
        FCk_Marker_DebugInfo      InDebugParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, GameplayTagFilter = "Marker"))
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
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
    CK_PROPERTY_GET(_PhysicsParams);
    CK_PROPERTY_GET(_AttachmentParams);
    CK_PROPERTY_GET(_RelativeTransform);
    CK_PROPERTY_GET(_ShowDebug);
    CK_PROPERTY_GET(_DebugParams);
    CK_PROPERTY_GET(_StartingState);
    CK_PROPERTY_GET(_ReplicationType);
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
    auto Get_OwningEntity() const -> FCk_Handle override;

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
    auto Get_OwningEntity() const -> FCk_Handle override;

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
    auto Get_OwningEntity() const -> FCk_Handle override;

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkActor/ActorModifier/CkActorModifier_Fragment_Data.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Net/CkNet_Common.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

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

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKOVERLAPBODY_API FCk_Marker_BasicDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Marker_BasicDetails);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _MarkerName;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_Marker _MarkerEntity;

    // Represents the Entity/Actor that the Marker ActorComp is attached to. Different from the Marker Entity itself
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_EntityOwningActor_BasicDetails _MarkerAttachedEntityAndActor;

public:
    CK_PROPERTY_GET(_MarkerName);
    CK_PROPERTY_GET(_MarkerEntity);
    CK_PROPERTY_GET(_MarkerAttachedEntityAndActor);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_BasicDetails, _MarkerName, _MarkerEntity, _MarkerAttachedEntityAndActor);
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Marker_BasicDetails& InObj) -> uint32;

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
    CK_PROPERTY_GET(_LineThickness);
    CK_PROPERTY_GET(_DebugLineColor);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_DebugInfo, _LineThickness, _DebugLineColor);
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
    FCollisionProfileName _CollisionProfileName  = FCollisionProfileName(TEXT("CkMarker"));

public:
    CK_PROPERTY_GET(_CollisionType)
    CK_PROPERTY_GET(_NavigationEffect);
    CK_PROPERTY_GET(_OverlapBehavior);
    CK_PROPERTY_GET(_CollisionProfileName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Marker_PhysicsInfo, _CollisionType, _NavigationEffect, _OverlapBehavior, _CollisionProfileName);
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
struct CKOVERLAPBODY_API FCk_Request_Marker_Resize : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Marker_Resize);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Marker_Resize);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ShapeDimensions _NewMarkerDimensions;

public:
    CK_PROPERTY_GET(_NewMarkerDimensions)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Marker_Resize, _NewMarkerDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Request_Marker_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Marker_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Marker_EnableDisable);

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
              meta = (AllowPrivateAccess = true, Categories = "Marker"))
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
    CK_PROPERTY_GET(_PhysicsParams);
    CK_PROPERTY_GET(_AttachmentParams);
    CK_PROPERTY_GET(_RelativeTransform);
    CK_PROPERTY_GET(_ShowDebug);
    CK_PROPERTY_GET(_DebugParams);
    CK_PROPERTY_GET(_StartingState);
    CK_PROPERTY(_ReplicationType);
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
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName = "[Ck][Marker] Get Type")
    ECk_OverlapBody_Type Get_Type() const override;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName = "[Ck][Marker] Get Owning Entity")
    const FCk_Handle& Get_OwningEntity() const override;

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
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName = "[Ck][Marker] Get Type")
    ECk_OverlapBody_Type Get_Type() const override;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName = "[Ck][Marker] Get Owning Entity")
    const FCk_Handle& Get_OwningEntity() const override;

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
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName = "[Ck][Marker] Get Type")
    ECk_OverlapBody_Type Get_Type() const override;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName = "[Ck][Marker] Get Owning Entity")
    const FCk_Handle& Get_OwningEntity() const override;

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------
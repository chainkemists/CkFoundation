#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <Components/ShapeComponent.h>

#include "CkSensor_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECk_Sensor_AttachmentPolicy : uint8
{
    None = 0 UMETA(Hidden),

    /* The bone's rotation will be used */
    UseBoneRotation = 1 << 0,

    /* The bone's position will be used */
    UseBonePosition = 1 << 1,
};

ENUM_CLASS_FLAGS(ECk_Sensor_AttachmentPolicy)
ENABLE_ENUM_BITWISE_OPERATORS(ECk_Sensor_AttachmentPolicy);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Sensor_AttachmentPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKOVERLAPBODY_API FCk_Handle_Sensor : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Sensor); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Sensor);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_BeginOverlap_UnrealDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_BeginOverlap_UnrealDetails);

    FCk_Sensor_BeginOverlap_UnrealDetails() = default;
    FCk_Sensor_BeginOverlap_UnrealDetails(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPrimitiveComponent> _OverlappedComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _OtherActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPrimitiveComponent> _OtherComp;

public:
    CK_PROPERTY_GET(_OverlappedComponent)
    CK_PROPERTY_GET(_OtherActor)
    CK_PROPERTY_GET(_OtherComp)
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Sensor_BeginOverlap_UnrealDetails& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_EndOverlap_UnrealDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_EndOverlap_UnrealDetails);

public:
    FCk_Sensor_EndOverlap_UnrealDetails() = default;
    FCk_Sensor_EndOverlap_UnrealDetails(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPrimitiveComponent> _OverlappedComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _OtherActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UPrimitiveComponent> _OtherComp;

public:
    CK_PROPERTY_GET(_OverlappedComponent)
    CK_PROPERTY_GET(_OtherActor)
    CK_PROPERTY_GET(_OtherComp)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKOVERLAPBODY_API FCk_Sensor_BasicDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_BasicDetails);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _SensorName;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_Sensor _SensorEntity;

    // Represents the Entity/Actor that the Sensor ActorComp is attached to. Different from the Sensor Entity itself
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_EntityOwningActor_BasicDetails _SensorAttachedEntityAndActor;

public:
    CK_PROPERTY_GET(_SensorName);
    CK_PROPERTY_GET(_SensorEntity);
    CK_PROPERTY_GET(_SensorAttachedEntityAndActor);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_BasicDetails, _SensorName, _SensorEntity, _SensorAttachedEntityAndActor);
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Sensor_BasicDetails& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_MarkerOverlapInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_MarkerOverlapInfo);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Marker_BasicDetails _MarkerDetails;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BeginOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_MarkerDetails)
    CK_PROPERTY_GET(_OverlapDetails)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_MarkerOverlapInfo, _MarkerDetails, _OverlapDetails);
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Sensor_MarkerOverlapInfo& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_NonMarkerOverlapInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_NonMarkerOverlapInfo);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BeginOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_OverlapDetails)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_NonMarkerOverlapInfo, _OverlapDetails);
};

auto CKOVERLAPBODY_API GetTypeHash(const FCk_Sensor_NonMarkerOverlapInfo& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_MarkerOverlaps
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_MarkerOverlaps);

public:
    using SensorOverlapInfoType = FCk_Sensor_MarkerOverlapInfo;
    using SensorOverlapInfoList = TMap<FCk_Marker_BasicDetails, SensorOverlapInfoType>;

public:
    auto Add(
        const SensorOverlapInfoType& InOverlap) -> ThisType&;

    auto Remove(
        const SensorOverlapInfoType& InOverlap) -> ThisType&;

    auto RemoveOverlapWithMarker(
        const FCk_Marker_BasicDetails& InMarkerDetails) -> ThisType&;

    auto Get_HasOverlapWithMarker(
        const FCk_Marker_BasicDetails& InMarkerDetails) const -> bool;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TMap<FCk_Marker_BasicDetails, FCk_Sensor_MarkerOverlapInfo> _Overlaps;

public:
    CK_PROPERTY_GET(_Overlaps);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_MarkerOverlaps, _Overlaps);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_NonMarkerOverlaps
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_NonMarkerOverlaps);

public:
    using SensorOverlapInfoType = FCk_Sensor_NonMarkerOverlapInfo;
    using SensorOverlapInfoList = TSet<SensorOverlapInfoType>;

public:
    auto Add(
        const SensorOverlapInfoType& InOverlap) -> ThisType&;

    auto Remove(
        const SensorOverlapInfoType& InOverlap) -> ThisType&;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSet<FCk_Sensor_NonMarkerOverlapInfo> _Overlaps;

public:
    CK_PROPERTY_GET(_Overlaps);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_NonMarkerOverlaps, _Overlaps);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_DebugInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_DebugInfo);

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
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_DebugInfo, _LineThickness, _DebugLineColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_PhysicsInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_PhysicsInfo);

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
    FCollisionProfileName _CollisionProfileName  = FCollisionProfileName(TEXT("CkSensor"));

public:
    CK_PROPERTY_GET(_CollisionType)
    CK_PROPERTY_GET(_NavigationEffect);
    CK_PROPERTY_GET(_OverlapBehavior);
    CK_PROPERTY_GET(_CollisionProfileName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_PhysicsInfo, _CollisionType, _NavigationEffect, _OverlapBehavior, _CollisionProfileName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_AttachmentInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_AttachmentInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ActorComponent_AttachmentPolicy _AttachmentType = ECk_ActorComponent_AttachmentPolicy::Attach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Bitmask, BitmaskEnum = "/Script/CkOverlapBody.ECk_Sensor_AttachmentPolicy"))
    int32  _AttachmentPolicyFlags = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_AttachmentPolicyFlags != 0"))
    FName _BoneName;

public:
    auto Get_AttachmentPolicy() const -> ECk_Sensor_AttachmentPolicy;

public:
    CK_PROPERTY(_AttachmentType)
    CK_PROPERTY(_BoneName);
    CK_PROPERTY_SET(_AttachmentPolicyFlags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_ShapeInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_ShapeInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ShapeDimensions _ShapeDimensions;

public:
    CK_PROPERTY_GET(_ShapeDimensions)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_ShapeInfo, _ShapeDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_FilteringInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_FilteringInfo);

public:
    FCk_Sensor_FilteringInfo() = default;
    explicit FCk_Sensor_FilteringInfo(
        const TArray<FGameplayTag>& InMarkerNames);

private:
    // TODO: Turn into GameplayTagContainer
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Marker"))
    TArray<FGameplayTag> _MarkerNames;

public:
    CK_PROPERTY_GET(_MarkerNames)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Request_Sensor_Resize : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sensor_Resize);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sensor_Resize);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_ShapeDimensions _NewSensorDimensions;

public:
    CK_PROPERTY_GET(_NewSensorDimensions)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sensor_Resize, _NewSensorDimensions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Request_Sensor_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sensor_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sensor_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sensor_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Sensor_OnEnableDisable,
    FCk_Handle, InHandle,
    FGameplayTag, InSensorName,
    ECk_EnableDisable, InEnableDisable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_Sensor_OnEnableDisable_MC,
    FCk_Handle, InHandle,
    FGameplayTag, InSensorName,
    ECk_EnableDisable, InEnableDisable);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKOVERLAPBODY_API FCk_Request_Sensor_OnBeginOverlap : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sensor_OnBeginOverlap);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sensor_OnBeginOverlap);

private:
    FCk_Marker_BasicDetails _MarkerDetails;
    FCk_Sensor_BasicDetails _SensorDetails;
    FCk_Sensor_BeginOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_MarkerDetails)
    CK_PROPERTY_GET(_SensorDetails)
    CK_PROPERTY_GET(_OverlapDetails)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sensor_OnBeginOverlap, _MarkerDetails, _SensorDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKOVERLAPBODY_API FCk_Request_Sensor_OnBeginOverlap_NonMarker : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sensor_OnBeginOverlap_NonMarker);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sensor_OnBeginOverlap_NonMarker);

private:
    FCk_Sensor_BasicDetails _SensorDetails;
    FCk_Sensor_BeginOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_SensorDetails)
    CK_PROPERTY_GET(_OverlapDetails)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sensor_OnBeginOverlap_NonMarker, _SensorDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKOVERLAPBODY_API FCk_Request_Sensor_OnEndOverlap : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sensor_OnEndOverlap);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sensor_OnEndOverlap);

private:
    FCk_Marker_BasicDetails _MarkerDetails;
    FCk_Sensor_BasicDetails _SensorDetails;
    FCk_Sensor_EndOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_MarkerDetails)
    CK_PROPERTY_GET(_SensorDetails)
    CK_PROPERTY_GET(_OverlapDetails)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sensor_OnEndOverlap, _MarkerDetails, _SensorDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKOVERLAPBODY_API FCk_Request_Sensor_OnEndOverlap_NonMarker : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Sensor_OnEndOverlap_NonMarker);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Sensor_OnEndOverlap_NonMarker);

private:
    FCk_Sensor_BasicDetails _SensorDetails;
    FCk_Sensor_EndOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_SensorDetails)
    CK_PROPERTY_GET(_OverlapDetails)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Sensor_OnEndOverlap_NonMarker, _SensorDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_Payload_OnBeginOverlap
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_Payload_OnBeginOverlap);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BasicDetails _SensorDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Marker_BasicDetails _MarkerDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BeginOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_SensorDetails);
    CK_PROPERTY_GET(_MarkerDetails);
    CK_PROPERTY_GET(_OverlapDetails);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_Payload_OnBeginOverlap, _SensorDetails, _MarkerDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnBeginOverlap,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnBeginOverlap, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnBeginOverlap_MC,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnBeginOverlap, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_Payload_OnBeginOverlap_NonMarker
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_Payload_OnBeginOverlap_NonMarker);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BasicDetails _SensorDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BeginOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_SensorDetails);
    CK_PROPERTY_GET(_OverlapDetails);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_Payload_OnBeginOverlap_NonMarker, _SensorDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnBeginOverlap_NonMarker,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnBeginOverlap_NonMarker, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnBeginOverlap_NonMarker_MC,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnBeginOverlap_NonMarker, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_Payload_OnEndOverlap
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_Payload_OnEndOverlap);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BasicDetails _SensorDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Marker_BasicDetails _MarkerDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_EndOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_SensorDetails);
    CK_PROPERTY_GET(_MarkerDetails);
    CK_PROPERTY_GET(_OverlapDetails);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_Payload_OnEndOverlap, _SensorDetails, _MarkerDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnEndOverlap,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnEndOverlap, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnEndOverlap_MC,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnEndOverlap, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Sensor_Payload_OnEndOverlap_NonMarker
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sensor_Payload_OnEndOverlap_NonMarker);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_BasicDetails _SensorDetails;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_EndOverlap_UnrealDetails _OverlapDetails;

public:
    CK_PROPERTY_GET(_SensorDetails);
    CK_PROPERTY_GET(_OverlapDetails);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sensor_Payload_OnEndOverlap_NonMarker, _SensorDetails, _OverlapDetails);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnEndOverlap_NonMarker,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnEndOverlap_NonMarker, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Sensor_OnEndOverlap_NonMarker_MC,
    FCk_Handle, InHandle,
    FCk_Sensor_Payload_OnEndOverlap_NonMarker, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Fragment_Sensor_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Sensor_ParamsData);

public:
    FCk_Fragment_Sensor_ParamsData() = default;
    FCk_Fragment_Sensor_ParamsData(
        FGameplayTag              InSensorName,
        FCk_Sensor_FilteringInfo  InFilteringParams,
        FCk_Sensor_ShapeInfo      InShapeParams,
        FCk_Sensor_PhysicsInfo    InPhysicsParams,
        FCk_Sensor_AttachmentInfo InAttachmentParams,
        FTransform                InRelativeTransform,
        ECk_EnableDisable         InStartingState,
        ECk_Net_ReplicationType   InReplicationType,
        bool                      InShowDebug,
        FCk_Sensor_DebugInfo      InDebugParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Sensor"))
    FGameplayTag _SensorName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_FilteringInfo _FilteringParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_ShapeInfo _ShapeParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_PhysicsInfo _PhysicsParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Sensor_AttachmentInfo _AttachmentParams;

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
    FCk_Sensor_DebugInfo _DebugParams;

public:
    CK_PROPERTY_GET(_SensorName);
    CK_PROPERTY_GET(_FilteringParams);
    CK_PROPERTY_GET(_ShapeParams);
    CK_PROPERTY_GET(_PhysicsParams);
    CK_PROPERTY_GET(_AttachmentParams);
    CK_PROPERTY_GET(_RelativeTransform);
    CK_PROPERTY_GET(_StartingState);
    CK_PROPERTY(_ReplicationType);
    CK_PROPERTY_GET(_ShowDebug);
    CK_PROPERTY_GET(_DebugParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOVERLAPBODY_API FCk_Fragment_MultipleSensor_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleSensor_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Sensor_ParamsData> _SensorParams;

public:
    CK_PROPERTY_GET(_SensorParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleSensor_ParamsData, _SensorParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKOVERLAPBODY_API UCk_Sensor_ActorComponent_Box_UE
    : public UCk_OverlapBody_ActorComponent_Box_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Sensor_ActorComponent_Box_UE);

public:
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    auto Get_Type() const -> ECk_OverlapBody_Type override;
    auto Get_OwningEntity() const -> const FCk_Handle& override;

private:
    auto InitializeComponent() -> void override;

private:
    UFUNCTION()
    void OnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex,
        bool                 InFromSweep,
        const FHitResult&    InHitResult);

    UFUNCTION()
    void OnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex);

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKOVERLAPBODY_API UCk_Sensor_ActorComponent_Sphere_UE
    : public UCk_OverlapBody_ActorComponent_Sphere_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Sensor_ActorComponent_Sphere_UE);

public:
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    auto Get_Type() const -> ECk_OverlapBody_Type override;
    auto Get_OwningEntity() const -> const FCk_Handle& override;

private:
    auto InitializeComponent() -> void override;

private:
    UFUNCTION()
    void OnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex,
        bool                 InFromSweep,
        const FHitResult&    InHitResult);

    UFUNCTION()
    void OnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex);

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKOVERLAPBODY_API UCk_Sensor_ActorComponent_Capsule_UE
    : public UCk_OverlapBody_ActorComponent_Capsule_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Sensor_ActorComponent_Capsule_UE);

public:
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    auto Get_Type() const -> ECk_OverlapBody_Type override;
    auto Get_OwningEntity() const -> const FCk_Handle& override;

private:
    auto InitializeComponent() -> void override;

private:
    UFUNCTION()
    void OnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex,
        bool                 InFromSweep,
        const FHitResult&    InHitResult);

    UFUNCTION()
    void OnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex);

private:
    UPROPERTY(Transient)
    FCk_Handle _OwningEntity;
};

// --------------------------------------------------------------------------------------------------------------------
// IsValid & Formatter

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Sensor_BeginOverlap_UnrealDetails, [](const FCk_Sensor_BeginOverlap_UnrealDetails& InObj)
{
    return ck::Format
    (
        TEXT("Other Actor: [{}] | Other Comp: [{}] | Overlapped Component: [{}]"),
        InObj.Get_OtherActor(),
        InObj.Get_OtherComp(),
        InObj.Get_OverlappedComponent()
    );
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Sensor_EndOverlap_UnrealDetails, [](const FCk_Sensor_EndOverlap_UnrealDetails& InObj)
{
    return ck::Format
    (
        TEXT("Other Actor: [{}] | Other Comp: [{}] | Overlapped Component: [{}]"),
        InObj.Get_OtherActor(),
        InObj.Get_OtherComp(),
        InObj.Get_OverlappedComponent()
    );
});

// --------------------------------------------------------------------------------------------------------------------
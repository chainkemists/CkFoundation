#include "CkSensor_Fragment_Data.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Utils.h"
#include "CkOverlapBody/Sensor/CkSensor_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Sensor, TEXT("Sensor"));

// --------------------------------------------------------------------------------------------------------------------

namespace ck_sensor
{
    auto DoGet_OverlapBodyInfo(
        const UPrimitiveComponent* InComp)
        -> TTuple<ECk_OverlapBody_Type, const ICk_OverlapBody_Interface*>
    {
        if (const auto* ComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InComp);
            ck::IsValid(ComponentAsOverlapBody, ck::IsValid_Policy_NullptrOnly{}))
        {
            return MakeTuple(ComponentAsOverlapBody->Get_Type(), ComponentAsOverlapBody);
        }

        return MakeTuple(ECk_OverlapBody_Type::Other, nullptr);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto DoGet_Sensor_And_MarkerOrOverlapComp(
        const UPrimitiveComponent* InCompA,
        const UPrimitiveComponent* InCompB)
        -> TTuple<const UPrimitiveComponent*, const ICk_OverlapBody_Interface*, const UPrimitiveComponent*, const ICk_OverlapBody_Interface*>
    {
        const auto& [compA_OverlapBodyType, compA_AsOverlapBody] = DoGet_OverlapBodyInfo(InCompA);
        const auto& [compB_OverlapBodyType, compB_AsOverlapBody] = DoGet_OverlapBodyInfo(InCompB);

        CK_ENSURE_IF_NOT(compA_OverlapBodyType != compB_OverlapBodyType,
            TEXT("OverlappedComponent [{}] and OtherComp [{}] both have the same OverlapBody type [{}] during BeginOverlap/EndOverlap event!"),
            InCompA,
            InCompB,
            compA_OverlapBodyType)
        { return {}; }

        if (compA_OverlapBodyType == ECk_OverlapBody_Type::Sensor)
        {
            const auto& SensorComp          = InCompA;
            const auto& SensorOverlapBody   = compA_AsOverlapBody;
            const auto& MarkerOrOverlapComp = InCompB;
            const auto& MarkerOverlapBody   = compB_AsOverlapBody;

            if (ck::IsValid(MarkerOverlapBody, ck::IsValid_Policy_NullptrOnly{}))
            {
                CK_ENSURE
                (
                    compB_OverlapBodyType == ECk_OverlapBody_Type::Marker,
                    TEXT("Expected [{}] to be a Marker with the paired Sensor [{}]"),
                    MarkerOrOverlapComp,
                    SensorComp
                );
            }

            return MakeTuple(SensorComp, SensorOverlapBody, MarkerOrOverlapComp, MarkerOverlapBody);
        }

        if (compB_OverlapBodyType == ECk_OverlapBody_Type::Sensor)
        {
            const auto& SensorComp          = InCompB;
            const auto& SensorOverlapBody   = compB_AsOverlapBody;
            const auto& MarkerOrOverlapComp = InCompA;
            const auto& MarkerOverlapBody   = compA_AsOverlapBody;

            if (ck::IsValid(MarkerOverlapBody, ck::IsValid_Policy_NullptrOnly{}))
            {
                CK_ENSURE
                (
                    compA_OverlapBodyType == ECk_OverlapBody_Type::Marker,
                    TEXT("Expected [{}] to be a Marker with the paired Sensor [{}]"),
                    MarkerOrOverlapComp,
                    SensorComp
                );
            }

            return MakeTuple(SensorComp, SensorOverlapBody, MarkerOrOverlapComp, MarkerOverlapBody);
        }

        CK_TRIGGER_ENSURE(TEXT("BeginOverlap/EndOverlap event between CompA [{}] and CompB [{}] but none of them are Sensors"), InCompA, InCompB);
        return {};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto DoOnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex,
        bool                 InFromSweep,
        const FHitResult&    InHitResult)
    -> void
    {
        const auto& [sensorComp, sensorOverlapBody, markerOrOverlapComp, markerOverlapBody] = DoGet_Sensor_And_MarkerOrOverlapComp(InOverlappedComponent, InOtherComp);

        CK_ENSURE_IF_NOT(ck::IsValid(sensorComp) && ck::IsValid(markerOrOverlapComp),
            TEXT("Failed to get Marker/OverlapComponent and Sensor from BeginOverlap event on Actor [{}] with Actor [{}] and Component [{}] with OtherComp [{}]"),
            InOverlappedComponent->GetOwner(),
            InOtherComp->GetOwner(),
            InOverlappedComponent,
            InOtherComp)
        { return; }

        const auto& SensorCompAttachedActor = sensorComp->GetOwner();
        const auto& MarkerCompAttachedActor = markerOrOverlapComp->GetOwner();

        if (sensorComp->MoveIgnoreActors.Contains(MarkerCompAttachedActor))
        {
            CK_ENSURE_IF_NOT(SensorCompAttachedActor == MarkerCompAttachedActor,
                TEXT("Received BeginOverlap on Actor [{}] with Actor [{}] and Component [{}] with OtherComp [{}] "
                     "where OtherComp is set to be ignored."),
                SensorCompAttachedActor,
                MarkerCompAttachedActor,
                sensorComp,
                markerOrOverlapComp)
            { return; }
        }

    	CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(SensorCompAttachedActor),
    		TEXT("Sensor Owner Actor {} is NOT ECS ready! This means the sensor component was not added through this feature (maybe through cloning components)"),
    		SensorCompAttachedActor)
    	{ return; }

        const auto& SensorAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(SensorCompAttachedActor);
        auto SensorOwningEntity = UCk_Utils_Sensor_UE::CastChecked(sensorOverlapBody->Get_OwningEntity());
        const auto& SensorName = UCk_Utils_GameplayLabel_UE::Get_Label(SensorOwningEntity);

        if (ck::Is_NOT_Valid(markerOverlapBody, ck::IsValid_Policy_NullptrOnly{}))
        {
            UCk_Utils_Sensor_UE::Request_OnBeginOverlap_NonMarker
            (
                SensorOwningEntity,
                FCk_Request_Sensor_OnBeginOverlap_NonMarker
                {
                    FCk_Sensor_BasicDetails
                    {
                        SensorName,
                        SensorOwningEntity,
                        SensorAttachedEntityAndActor
                    },
                    FCk_Sensor_BeginOverlap_UnrealDetails
                    {
                        InOverlappedComponent,
                        InOtherActor,
                        InOtherComp
                    }
                }
            );

            return;
        }

        const auto& MarkerAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(MarkerCompAttachedActor);
        const auto& MarkerOwningEntity = UCk_Utils_Marker_UE::CastChecked(markerOverlapBody->Get_OwningEntity());
        const auto& MarkerName = UCk_Utils_GameplayLabel_UE::Get_Label(MarkerOwningEntity);

        UCk_Utils_Sensor_UE::Request_OnBeginOverlap
        (
            SensorOwningEntity,
            FCk_Request_Sensor_OnBeginOverlap
            {
                FCk_Marker_BasicDetails
                {
                    MarkerName,
                    MarkerOwningEntity,
                    MarkerAttachedEntityAndActor
                },
                FCk_Sensor_BasicDetails
                {
                    SensorName,
                    SensorOwningEntity,
                    SensorAttachedEntityAndActor
                },
                FCk_Sensor_BeginOverlap_UnrealDetails
                {
                    InOverlappedComponent,
                    InOtherActor,
                    InOtherComp
                }
            }
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto DoOnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor*              InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32                InOtherBodyIndex)
    -> void
    {
        const auto& [sensorComp, sensorOverlapBody, markerOrOverlapComp, markerOverlapBody] = DoGet_Sensor_And_MarkerOrOverlapComp(InOverlappedComponent, InOtherComp);

        CK_ENSURE_IF_NOT(ck::IsValid(sensorComp) && ck::IsValid(markerOrOverlapComp),
            TEXT("Failed to get Marker/OverlapComponent and Sensor from BeginOverlap event on Actor [{}] with Actor [{}] and Component [{}] with OtherComp [{}]"),
            InOverlappedComponent->GetOwner(),
            InOtherComp->GetOwner(),
            InOverlappedComponent,
            InOtherComp)
        { return; }

        const auto& SensorCompAttachedActor = sensorComp->GetOwner();
        const auto& MarkerCompAttachedActor = markerOrOverlapComp->GetOwner();

        if (sensorComp->MoveIgnoreActors.Contains(MarkerCompAttachedActor))
        {
            CK_ENSURE_IF_NOT(SensorCompAttachedActor == MarkerCompAttachedActor,
                TEXT("Received BeginOverlap on Actor [{}] with Actor [{}] and Component [{}] with OtherComp [{}] "
                     "where OtherComp is set to be ignored."),
                SensorCompAttachedActor,
                MarkerCompAttachedActor,
                sensorComp,
                markerOrOverlapComp)
            { return; }
        }

        const auto& SensorAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(SensorCompAttachedActor);
        auto SensorOwningEntity                  = UCk_Utils_Sensor_UE::CastChecked(sensorOverlapBody->Get_OwningEntity());
        const auto& SensorName                   = UCk_Utils_GameplayLabel_UE::Get_Label(SensorOwningEntity);

        if (ck::Is_NOT_Valid(markerOverlapBody, ck::IsValid_Policy_NullptrOnly{}))
        {
            UCk_Utils_Sensor_UE::Request_OnEndOverlap_NonMarker
            (
                SensorOwningEntity,
                FCk_Request_Sensor_OnEndOverlap_NonMarker
                {
                    FCk_Sensor_BasicDetails
                    {
                        SensorName,
                        SensorOwningEntity,
                        SensorAttachedEntityAndActor
                    },
                    FCk_Sensor_EndOverlap_UnrealDetails
                    {
                        InOverlappedComponent,
                        InOtherActor,
                        InOtherComp
                    }
                }
            );

            return;
        }

        const auto& MarkerAttachedEntityAndActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails_FromActor(MarkerCompAttachedActor);
        const auto& MarkerOwningEntity           = markerOverlapBody->Get_OwningEntity();
        const auto& MarkerName                   = UCk_Utils_GameplayLabel_UE::Get_Label(MarkerOwningEntity);

        UCk_Utils_Sensor_UE::Request_OnEndOverlap
        (
            SensorOwningEntity,
            FCk_Request_Sensor_OnEndOverlap
            {
                FCk_Marker_BasicDetails
                {
                    MarkerName,
                    UCk_Utils_Marker_UE::CastChecked(MarkerOwningEntity),
                    MarkerAttachedEntityAndActor
                },
                FCk_Sensor_BasicDetails
                {
                    SensorName,
                    SensorOwningEntity,
                    SensorAttachedEntityAndActor
                },
                FCk_Sensor_EndOverlap_UnrealDetails
                {
                    InOverlappedComponent,
                    InOtherActor,
                    InOtherComp
                }
            }
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Sensor_BeginOverlap_UnrealDetails::
    FCk_Sensor_BeginOverlap_UnrealDetails(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp)
    : _OverlappedComponent(InOverlappedComponent)
    , _OtherActor(InOtherActor)
    , _OtherComp(InOtherComp)
{
}

auto
    FCk_Sensor_BeginOverlap_UnrealDetails::
    operator==(const ThisType& InOther) const
    -> bool
{
    return (Get_OtherActor() == InOther.Get_OtherActor() && Get_OtherComp() == InOther.Get_OtherComp()           && Get_OverlappedComponent() == InOther.Get_OverlappedComponent()) ||
           (Get_OtherActor() == InOther.Get_OtherActor() && Get_OtherComp() == InOther.Get_OverlappedComponent() && Get_OverlappedComponent() == InOther.Get_OtherComp());
}

auto
    GetTypeHash(
        const FCk_Sensor_BeginOverlap_UnrealDetails& InUnrealDetails)
    -> uint32
{
    return GetTypeHash(InUnrealDetails.Get_OtherActor()) +
           GetTypeHash(InUnrealDetails.Get_OtherComp()) +
           GetTypeHash(InUnrealDetails.Get_OverlappedComponent());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Sensor_EndOverlap_UnrealDetails::
    FCk_Sensor_EndOverlap_UnrealDetails(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp)
    : _OverlappedComponent(InOverlappedComponent)
    , _OtherActor(InOtherActor)
    , _OtherComp(InOtherComp)

{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Sensor_BasicDetails::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_SensorName() == InOther.Get_SensorName() &&
           Get_SensorEntity() == InOther.Get_SensorEntity();
}

auto
    GetTypeHash(
        const FCk_Sensor_BasicDetails& InBasicDetails)
    -> uint32
{
    return GetTypeHash(InBasicDetails.Get_SensorName()) +
           GetTypeHash(InBasicDetails.Get_SensorEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Sensor_MarkerOverlapInfo::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_MarkerDetails() == InOther.Get_MarkerDetails();
}

auto
    GetTypeHash(
        const FCk_Sensor_MarkerOverlapInfo& InOverlapInfo)
    -> uint32
{
    return GetTypeHash(InOverlapInfo.Get_MarkerDetails());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Sensor_NonMarkerOverlapInfo::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_OverlapDetails() == InOther.Get_OverlapDetails();
}

auto
    GetTypeHash(
        const FCk_Sensor_NonMarkerOverlapInfo& InOverlapInfo)
    -> uint32
{
    return GetTypeHash(InOverlapInfo.Get_OverlapDetails());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Sensor_MarkerOverlaps::
    Process_Add(
        const SensorOverlapInfoType& InOverlap)
    -> ThisType&
{
    _Overlaps.Add(InOverlap.Get_MarkerDetails(), InOverlap);

    return *this;
}

auto
    FCk_Sensor_MarkerOverlaps::
    Process_Remove(
        const SensorOverlapInfoType& InOverlap)
    -> ThisType&
{
    _Overlaps.Remove(InOverlap.Get_MarkerDetails());

    return *this;
}

auto
    FCk_Sensor_MarkerOverlaps::
    Process_RemoveOverlapWithMarker(
        const FCk_Marker_BasicDetails& InMarkerDetails)
    -> ThisType&
{
    _Overlaps.Remove(InMarkerDetails);

    return *this;
}

auto
    FCk_Sensor_MarkerOverlaps::
    Get_HasOverlapWithMarker(
        const FCk_Marker_BasicDetails& InMarkerDetails) const
    -> bool
{
    return _Overlaps.Contains(InMarkerDetails);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Sensor_NonMarkerOverlaps::
    Process_Add(
        const SensorOverlapInfoType& InOverlap)
    -> ThisType&
{
    _Overlaps.Add(InOverlap);

    return *this;
}

auto
    FCk_Sensor_NonMarkerOverlaps::
    Process_Remove(
        const SensorOverlapInfoType& InOverlap)
    -> ThisType&
{
    _Overlaps.Remove(InOverlap);

    return *this;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Sensor_AttachmentInfo::
    Get_AttachmentPolicy() const
    -> ECk_Sensor_AttachmentPolicy
{
    return static_cast<ECk_Sensor_AttachmentPolicy>(_AttachmentPolicyFlags);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Sensor_FilteringInfo::
    FCk_Sensor_FilteringInfo(
    const TArray<FGameplayTag>& InMarkerNames)
    : _MarkerNames(InMarkerNames)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Sensor_ParamsData::
    FCk_Fragment_Sensor_ParamsData(
        FGameplayTag InSensorName,
        FCk_Sensor_FilteringInfo InFilteringParams,
        FCk_Sensor_ShapeInfo InShapeParams,
        FCk_Sensor_PhysicsInfo InPhysicsParams,
        FCk_Sensor_AttachmentInfo InAttachmentParams,
        FTransform InRelativeTransform,
        ECk_EnableDisable InStartingState,
        ECk_Net_ReplicationType InReplicationType,
        bool InShowDebug,
        FCk_Sensor_DebugInfo InDebugParams)
    : _SensorName(InSensorName)
    , _FilteringParams(InFilteringParams)
    , _ShapeParams(InShapeParams)
    , _PhysicsParams(InPhysicsParams)
    , _AttachmentParams(InAttachmentParams)
    , _RelativeTransform(InRelativeTransform)
    , _StartingState(InStartingState)
    , _ReplicationType(InReplicationType)
    , _ShowDebug(InShowDebug)
    , _DebugParams(InDebugParams)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Sensor_ActorComponent_Box_UE::
    Get_Type() const
    -> ECk_OverlapBody_Type
{
    return ECk_OverlapBody_Type::Sensor;
}

auto
    UCk_Sensor_ActorComponent_Box_UE::
    Get_OwningEntity() const
    -> const FCk_Handle&
{
    return _OwningEntity;
}

auto
    UCk_Sensor_ActorComponent_Box_UE::
    InitializeComponent()
    -> void
{
    Super::InitializeComponent();

    // If this component is ever cloned/duplicated, InitializeComponent could run on the cloned object if RegisterComponentWithWorld is called
    // In this case, the delegates are already bound, because the duplication process also copies the bindings from the original component
    if (NOT this->OnComponentBeginOverlap.IsAlreadyBound(this, &ThisType::OnBeginOverlap))
    {
        this->OnComponentBeginOverlap.AddDynamic(this, &ThisType::OnBeginOverlap);
    }

    if (NOT this->OnComponentEndOverlap.IsAlreadyBound(this, &ThisType::OnEndOverlap))
    {
        this->OnComponentEndOverlap.AddDynamic(this, &ThisType::OnEndOverlap);
    }
}

auto
    UCk_Sensor_ActorComponent_Box_UE::
    OnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32 InOtherBodyIndex,
        bool InFromSweep,
        const FHitResult& InHitResult)
    -> void
{
    ck_sensor::DoOnBeginOverlap
    (
        InOverlappedComponent,
        InOtherActor,
        InOtherComp,
        InOtherBodyIndex,
        InFromSweep,
        InHitResult
    );
}

auto
    UCk_Sensor_ActorComponent_Box_UE::
    OnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32 InOtherBodyIndex)
    -> void
{
    ck_sensor::DoOnEndOverlap
    (
        InOverlappedComponent,
        InOtherActor,
        InOtherComp,
        InOtherBodyIndex
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Sensor_ActorComponent_Sphere_UE::
    Get_Type() const
    -> ECk_OverlapBody_Type
{
    return ECk_OverlapBody_Type::Sensor;
}

auto
    UCk_Sensor_ActorComponent_Sphere_UE::
    Get_OwningEntity() const
    -> const FCk_Handle&
{
    return _OwningEntity;
}

auto
    UCk_Sensor_ActorComponent_Sphere_UE::
    InitializeComponent()
    -> void
{
    Super::InitializeComponent();

    // If this component is ever cloned/duplicated, InitializeComponent could run on the cloned object if RegisterComponentWithWorld is called
    // In this case, the delegates are already bound, because the duplication process also copies the bindings from the original component
    if (NOT this->OnComponentBeginOverlap.IsAlreadyBound(this, &ThisType::OnBeginOverlap))
    {
        this->OnComponentBeginOverlap.AddDynamic(this, &ThisType::OnBeginOverlap);
    }

    if (NOT this->OnComponentEndOverlap.IsAlreadyBound(this, &ThisType::OnEndOverlap))
    {
        this->OnComponentEndOverlap.AddDynamic(this, &ThisType::OnEndOverlap);
    }
}

auto
    UCk_Sensor_ActorComponent_Sphere_UE::
    OnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32 InOtherBodyIndex,
        bool InFromSweep,
        const FHitResult& InHitResult)
    -> void
{
    ck_sensor::DoOnBeginOverlap
    (
        InOverlappedComponent,
        InOtherActor,
        InOtherComp,
        InOtherBodyIndex,
        InFromSweep,
        InHitResult
    );
}

auto
    UCk_Sensor_ActorComponent_Sphere_UE::
    OnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32 InOtherBodyIndex)
    -> void
{
    ck_sensor::DoOnEndOverlap
    (
        InOverlappedComponent,
        InOtherActor,
        InOtherComp,
        InOtherBodyIndex
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Sensor_ActorComponent_Capsule_UE::
    Get_Type() const
    -> ECk_OverlapBody_Type
{
    return ECk_OverlapBody_Type::Sensor;
}

auto
    UCk_Sensor_ActorComponent_Capsule_UE::
    Get_OwningEntity() const
    -> const FCk_Handle&
{
    return _OwningEntity;
}

auto
    UCk_Sensor_ActorComponent_Capsule_UE::
    InitializeComponent()
    -> void
{
    Super::InitializeComponent();

    // If this component is ever cloned/duplicated, InitializeComponent could run on the cloned object if RegisterComponentWithWorld is called
    // In this case, the delegates are already bound, because the duplication process also copies the bindings from the original component
    if (NOT this->OnComponentBeginOverlap.IsAlreadyBound(this, &ThisType::OnBeginOverlap))
    {
        this->OnComponentBeginOverlap.AddDynamic(this, &ThisType::OnBeginOverlap);
    }

    if (NOT this->OnComponentEndOverlap.IsAlreadyBound(this, &ThisType::OnEndOverlap))
    {
        this->OnComponentEndOverlap.AddDynamic(this, &ThisType::OnEndOverlap);
    }
}

auto
    UCk_Sensor_ActorComponent_Capsule_UE::
    OnBeginOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32 InOtherBodyIndex,
        bool InFromSweep,
        const FHitResult& InHitResult)
    -> void
{
    ck_sensor::DoOnBeginOverlap
    (
        InOverlappedComponent,
        InOtherActor,
        InOtherComp,
        InOtherBodyIndex,
        InFromSweep,
        InHitResult
    );
}

auto
    UCk_Sensor_ActorComponent_Capsule_UE::
    OnEndOverlap(
        UPrimitiveComponent* InOverlappedComponent,
        AActor* InOtherActor,
        UPrimitiveComponent* InOtherComp,
        int32 InOtherBodyIndex)
    -> void
{
    ck_sensor::DoOnEndOverlap
    (
        InOverlappedComponent,
        InOtherActor,
        InOtherComp,
        InOtherBodyIndex
    );
}

// --------------------------------------------------------------------------------------------------------------------
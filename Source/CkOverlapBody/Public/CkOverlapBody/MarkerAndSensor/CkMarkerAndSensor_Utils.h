#pragma once

#include "CkActor/ActorModifier/CkActorModifier_Utils.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include "CkGraphics/CkGraphics_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment.h"
#include "CkOverlapBody/Sensor/CkSensor_Fragment.h"

#include "CkPhysics/CkPhysics_Common.h"
#include "CkPhysics/CkPhysics_Utils.h"

#include <CoreMinimal.h>

#include "CkMarkerAndSensor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKOVERLAPBODY_API UCk_Utils_MarkerAndSensor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MarkerAndSensor_UE);

public:
    using InitializerFuncType = TFunction<void(UActorComponent*)>;

public:
    static auto
    Draw_Marker_DebugLines(
        UObject* InOuter,
        const ck::FFragment_Marker_Current& InMarkerCurrent,
        const FCk_Fragment_Marker_ParamsData& InMarkerParams) -> void;

    static auto
    Draw_Sensor_DebugLines(
        UObject* InOuter,
        const ck::FFragment_Sensor_Current& InSensorCurrent,
        const FCk_Fragment_Sensor_ParamsData& InSensorParams) -> void;

public:
    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams>
    static auto
    Add_MarkerOrSensor_ActorComponent(
        FCk_Handle& InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, ECk_ShapeType T_MarkerOrSensorShapeType, typename T_MarkerOrSensorParams>
    static auto
    Add_MarkerOrSensor_ActorComponent(
        FCk_Handle& InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType>
    static auto
    Make_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorPhysicsParams>
    static auto
    Set_MarkerOrSensor_PhysicsParams(
        T_MarkerOrSensorCompType* InMarkerOrSensorComp,
        T_MarkerOrSensorPhysicsParams InMarkerOrSensorPhysicsParams,
        ECk_EnableDisable InEnableGenerateOverlapEvents) -> void;

private:
    template <typename T_MarkerOrSensorCurrent, typename T_MarkerOrSensorParams>
    static auto
    DoDraw_MarkerOrSensor_DebugLines(
        UObject* InOuter,
        const T_MarkerOrSensorCurrent& InMarkerOrSensorCurrent,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType>
    static auto
    DoAdd_MarkerOrSensor_ActorComponent(
        FCk_Handle& InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Box == T_MarkerOrSensorShapeType, uint8> = 0>
    static auto
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Sphere == T_MarkerOrSensorShapeType, uint8> = 0>
    static auto
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Capsule == T_MarkerOrSensorShapeType, uint8> = 0>
    static auto
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_MarkerOrSensorCurrent, typename T_MarkerOrSensorParams>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoDraw_MarkerOrSensor_DebugLines(
        UObject* InOuter,
        const T_MarkerOrSensorCurrent& InMarkerOrSensorCurrent,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOuter), TEXT("Invalid Outer supplied to DoDraw_MarkerOrSensor_DebugLines"))
    { return; }

    if (NOT InMarkerOrSensorParams.Get_ShowDebug())
    { return; }

    const auto& DebugParams     = InMarkerOrSensorParams.Get_DebugParams();
    const auto& ShapeParams     = InMarkerOrSensorParams.Get_ShapeParams();
    const auto& ShapeDimensions = ShapeParams.Get_ShapeDimensions();

    constexpr auto UsePersistentDebugDrawLines = false;
    constexpr auto DebugLineLifetime = -1.0f;

    const auto& [shapeComp, enableDisable] = [&]() -> TTuple<UShapeComponent*, ECk_EnableDisable>
    {
        if constexpr (std::is_same_v<T_MarkerOrSensorCurrent, ck::FFragment_Marker_Current>)
        {
            const auto& MarkerCurrent = InMarkerOrSensorCurrent;
            const auto& Marker = MarkerCurrent.Get_Marker().Get();

            return MakeTuple(Marker, MarkerCurrent.Get_EnableDisable());
        }
        else if constexpr (std::is_same_v<T_MarkerOrSensorCurrent, ck::FFragment_Sensor_Current>)
        {
            const auto& SensorCurrent = InMarkerOrSensorCurrent;
            const auto& Sensor = SensorCurrent.Get_Sensor().Get();

            return MakeTuple(Sensor, SensorCurrent.Get_EnableDisable());
        }
        else
        {
            static_assert("Unknown Marker/Sensor Current struct type passed to Draw_MarkerOrSensor_DebugLines");
            return {};
        }
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(shapeComp), TEXT("Invalid Marker/Sensor ActorComp stored to draw debug lines for"))
    { return; }

    const auto& Transform  = shapeComp->GetComponentTransform();
    const auto& IsDisabled = enableDisable == ECk_EnableDisable::Disable;

    const auto& ColorMultiplier         = IsDisabled ? 0.4f : 1.0f;
    const auto& LineThicknessMultiplier = IsDisabled ? 1.0f : 2.0f;

    const auto& DebugLineThickness = DebugParams.Get_LineThickness() * LineThicknessMultiplier;
    const auto& DebugLineColor     = UCk_Utils_Graphics_UE::Get_ModifiedColorIntensity(DebugParams.Get_DebugLineColor(), ColorMultiplier);

    switch (const auto& ShapeType = ShapeDimensions.Get_ShapeType())
    {
        case ECk_ShapeType::Box:
        {
            const auto& Box = ShapeDimensions.Get_BoxExtents();

            DrawDebugBox
            (
                InOuter->GetWorld(),
                Transform.GetLocation(),
                Box.Get_Extents(),
                Transform.GetRotation(),
                DebugLineColor,
                UsePersistentDebugDrawLines,
                DebugLineLifetime,
                0.0f,
                DebugLineThickness
            );

            break;
        }
        case ECk_ShapeType::Sphere:
        {
            const auto& Sphere = ShapeDimensions.Get_SphereRadius();

            DrawDebugSphere
            (
                InOuter->GetWorld(),
                Transform.GetLocation(),
                Sphere.Get_Radius(),
                12,
                DebugLineColor,
                UsePersistentDebugDrawLines,
                DebugLineLifetime,
                0.0f,
                DebugLineThickness
            );

            break;
        }
        case ECk_ShapeType::Capsule:
        {
            const auto& Capsule = ShapeDimensions.Get_CapsuleSize();

            DrawDebugCapsule
            (
                InOuter->GetWorld(),
                Transform.GetLocation(),
                Capsule.Get_HalfHeight(),
                Capsule.Get_Radius(),
                Transform.GetRotation(),
                DebugLineColor,
                UsePersistentDebugDrawLines,
                DebugLineLifetime,
                0.0f,
                DebugLineThickness
            );

            break;
        }
        default:
        {
            CK_INVALID_ENUM(ShapeType);
            break;
        }
    }
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams>
auto
    UCk_Utils_MarkerAndSensor_UE::
    Add_MarkerOrSensor_ActorComponent(
        FCk_Handle& InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> void
{
    if constexpr(std::is_same_v<T_MarkerOrSensorCompType, UCk_Marker_ActorComponent_Box_UE> || std::is_same_v<T_MarkerOrSensorCompType, UCk_Sensor_ActorComponent_Box_UE>)
    {
        Add_MarkerOrSensor_ActorComponent<T_MarkerOrSensorCompType, ECk_ShapeType::Box>(InMarkerOrSensorEntity, InMarkerOrSensorAttachedEntityAndActor, InMarkerOrSensorParams);
    }
    else if constexpr(std::is_same_v<T_MarkerOrSensorCompType, UCk_Marker_ActorComponent_Sphere_UE> || std::is_same_v<T_MarkerOrSensorCompType, UCk_Sensor_ActorComponent_Sphere_UE>)
    {
        Add_MarkerOrSensor_ActorComponent<T_MarkerOrSensorCompType, ECk_ShapeType::Sphere>(InMarkerOrSensorEntity, InMarkerOrSensorAttachedEntityAndActor, InMarkerOrSensorParams);
    }
    else if constexpr(std::is_same_v<T_MarkerOrSensorCompType, UCk_Marker_ActorComponent_Capsule_UE> || std::is_same_v<T_MarkerOrSensorCompType, UCk_Sensor_ActorComponent_Capsule_UE>)
    {
        Add_MarkerOrSensor_ActorComponent<T_MarkerOrSensorCompType, ECk_ShapeType::Capsule>(InMarkerOrSensorEntity, InMarkerOrSensorAttachedEntityAndActor, InMarkerOrSensorParams);
    }
    else
    {
        static_assert("Invalid Actor Component type for Marker/Sensor");
    }
}

template <typename T_MarkerOrSensorCompType, ECk_ShapeType T_MarkerOrSensorShapeType, typename T_MarkerOrSensorParams>
auto
    UCk_Utils_MarkerAndSensor_UE::
    Add_MarkerOrSensor_ActorComponent(
        FCk_Handle& InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> void
{
    DoAdd_MarkerOrSensor_ActorComponent<T_MarkerOrSensorCompType, T_MarkerOrSensorParams, T_MarkerOrSensorShapeType>(InMarkerOrSensorEntity, InMarkerOrSensorAttachedEntityAndActor, InMarkerOrSensorParams);
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoAdd_MarkerOrSensor_ActorComponent(
        FCk_Handle& InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> void
{
    const auto ActorPtr = InMarkerOrSensorAttachedEntityAndActor.Get_Actor();

    CK_ENSURE_IF_NOT(ck::IsValid(ActorPtr),
        TEXT("Handle [{}] does NOT have a valid Actor for us to Attach Sensor/Marker to!"),
        InMarkerOrSensorEntity)
    { return; }

    const auto& MarkerOrSensorAttachedActor = ActorPtr.Get();
    const auto& MarkerAttachedActorRootComponent = MarkerOrSensorAttachedActor->GetRootComponent();

    CK_ENSURE_IF_NOT(ck::IsValid(MarkerAttachedActorRootComponent),
        TEXT("Invalid Root Component for Marker's Owning Actor [{}]!"),
        MarkerOrSensorAttachedActor)
    { return; }

    auto* ShapeHolderComponent = UCk_Utils_Actor_UE::Request_AddNewActorComponent<USceneComponent>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<USceneComponent>
        {
            MarkerOrSensorAttachedActor,
            MarkerAttachedActorRootComponent,
        }
        .Set_IsUnique(false)
    );

    CK_ENSURE_IF_NOT(ck::IsValid(ShapeHolderComponent),
        TEXT("Failed to add the shape holder SceneComponent to Marker/Sensor Owning Actor [{}]"),
        MarkerOrSensorAttachedActor)
    { return; }

    const auto Make_MarkerOrSensor_ComponentParams = [&]() -> FCk_AddActorComponent_Params
    {
        static constexpr auto IsComponentTickEnabled = true;

        return FCk_AddActorComponent_Params{ShapeHolderComponent}
                .Set_IsTickEnabled(IsComponentTickEnabled)
                .Set_TickInterval(FCk_Time::ZeroSecond())
                .Set_AttachmentSocket(NAME_None)
                .Set_AttachmentType(InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType());
    };

    UCk_Utils_ActorModifier_UE::Request_AddActorComponent
    (
        InMarkerOrSensorAttachedEntityAndActor.Get_Handle(),
        FCk_Request_ActorModifier_AddActorComponent{T_MarkerOrSensorCompType::StaticClass()}
            .Set_ComponentParams(Make_MarkerOrSensor_ComponentParams())
            .Set_IsUnique(false)
            .Set_InitializerFunc(Make_MarkerOrSensor_InitializerFunction<T_MarkerOrSensorCompType, T_MarkerOrSensorParams, T_MarkerOrSensorShapeType>(
                InMarkerOrSensorEntity, InMarkerOrSensorParams)),
        {}, {}
    );
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType>
auto
    UCk_Utils_MarkerAndSensor_UE::
    Make_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType
{
    return DoMake_MarkerOrSensor_InitializerFunction<T_MarkerOrSensorCompType, T_MarkerOrSensorParams, T_MarkerOrSensorShapeType>(InMarkerOrSensorEntity, InMarkerOrSensorParams);
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Box == T_MarkerOrSensorShapeType, uint8>>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> InitializerFuncType
{
    auto InitializerFunc = [=](UActorComponent* InActorComp) mutable -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InActorComp), TEXT("Invalid Actor Component"))
        { return; }

        auto* MarkerOrSensorComp = Cast<T_MarkerOrSensorCompType>(InActorComp);

        CK_ENSURE_IF_NOT(ck::IsValid(MarkerOrSensorComp), TEXT("Invalid Marker/Sensor Actor Component"))
        { return; }

        MarkerOrSensorComp->_OwningEntity = InMarkerOrSensorEntity;

        const auto* OverlappedComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InActorComp);

        switch(const auto& OverlappedCompOverlapBodyType = OverlappedComponentAsOverlapBody->Get_Type())
        {
            case ECk_OverlapBody_Type::Marker:
            {
                InMarkerOrSensorEntity.Get<ck::FFragment_Marker_Current>()._Marker = Cast<UShapeComponent>(InActorComp);
                break;
            }
            case ECk_OverlapBody_Type::Sensor:
            {
                InMarkerOrSensorEntity.Get<ck::FFragment_Sensor_Current>()._Sensor = Cast<UShapeComponent>(InActorComp);
                break;
            }
            case ECk_OverlapBody_Type::Other:
            {
                CK_TRIGGER_ENSURE(TEXT("OverlapBody Component [{}] is neither a Marker or Sensor"), InActorComp);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(OverlappedCompOverlapBodyType);
                break;
            }
        }

        const auto& BoxExtents = InMarkerOrSensorParams.Get_ShapeParams().Get_ShapeDimensions().Get_BoxExtents().Get_Extents();

        MarkerOrSensorComp->SetBoxExtent(BoxExtents);
        MarkerOrSensorComp->AddLocalTransform(InMarkerOrSensorParams.Get_RelativeTransform());
        // we never want scale to affect our Sensors/Markers
        MarkerOrSensorComp->SetWorldScale3D(FVector::OneVector);

        const auto& PhysicsParams = InMarkerOrSensorParams.Get_PhysicsParams();
        const auto& StartingState = InMarkerOrSensorParams.Get_StartingState();

        Set_MarkerOrSensor_PhysicsParams(MarkerOrSensorComp, PhysicsParams, StartingState);

        if (InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
        {
            static constexpr auto CallModifyOnComponent = true;
            MarkerOrSensorComp->DetachFromComponent
            (
                FDetachmentTransformRules
                {
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    CallModifyOnComponent
                }
            );
        }
    };

    return InitializerFunc;
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Sphere == T_MarkerOrSensorShapeType, uint8>>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> InitializerFuncType
{
    auto InitializerFunc = [=](UActorComponent* InActorComp) mutable -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InActorComp), TEXT("Invalid Actor Component"))
        { return; }

        auto* MarkerOrSensorComp = Cast<T_MarkerOrSensorCompType>(InActorComp);

        CK_ENSURE_IF_NOT(ck::IsValid(MarkerOrSensorComp), TEXT("Invalid Marker/Sensor Actor Component"))
        { return; }

        MarkerOrSensorComp->_OwningEntity = InMarkerOrSensorEntity;

        const auto* OverlappedComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InActorComp);

        switch(const auto& OverlappedCompOverlapBodyType = OverlappedComponentAsOverlapBody->Get_Type())
        {
            case ECk_OverlapBody_Type::Marker:
            {
                InMarkerOrSensorEntity.Get<ck::FFragment_Marker_Current>()._Marker = Cast<UShapeComponent>(InActorComp);
                break;
            }
            case ECk_OverlapBody_Type::Sensor:
            {
                InMarkerOrSensorEntity.Get<ck::FFragment_Sensor_Current>()._Sensor = Cast<UShapeComponent>(InActorComp);
                break;
            }
            case ECk_OverlapBody_Type::Other:
            {
                CK_TRIGGER_ENSURE(TEXT("OverlapBody Component [{}] is neither a Marker or Sensor"), InActorComp);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(OverlappedCompOverlapBodyType);
                break;
            }
        }

        const auto& SphereRadius = InMarkerOrSensorParams.Get_ShapeParams().Get_ShapeDimensions().Get_SphereRadius().Get_Radius();

        MarkerOrSensorComp->SetSphereRadius(SphereRadius);
        MarkerOrSensorComp->AddLocalTransform(InMarkerOrSensorParams.Get_RelativeTransform());
        // we never want scale to affect our Sensors/Markers
        MarkerOrSensorComp->SetWorldScale3D(FVector::OneVector);

        const auto& PhysicsParams = InMarkerOrSensorParams.Get_PhysicsParams();
        const auto& StartingState = InMarkerOrSensorParams.Get_StartingState();

        Set_MarkerOrSensor_PhysicsParams(MarkerOrSensorComp, PhysicsParams, StartingState);

        if (InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
        {
            constexpr auto CallModifyOnComponent = true;
            MarkerOrSensorComp->DetachFromComponent
            (
                FDetachmentTransformRules
                {
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    CallModifyOnComponent
                }
            );
        }
    };

    return InitializerFunc;
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Capsule == T_MarkerOrSensorShapeType, uint8>>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle& InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> InitializerFuncType
{
    auto InitializerFunc = [=](UActorComponent* InActorComp) mutable -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InActorComp), TEXT("Invalid Actor Component"))
        { return; }

        auto* MarkerOrSensorComp = Cast<T_MarkerOrSensorCompType>(InActorComp);

        CK_ENSURE_IF_NOT(ck::IsValid(MarkerOrSensorComp), TEXT("Invalid Marker/Sensor Actor Component"))
        { return; }

        MarkerOrSensorComp->_OwningEntity = InMarkerOrSensorEntity;

        const auto* OverlappedComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InActorComp);

        switch(const auto& OverlappedCompOverlapBodyType = OverlappedComponentAsOverlapBody->Get_Type())
        {
            case ECk_OverlapBody_Type::Marker:
            {
                InMarkerOrSensorEntity.Get<ck::FFragment_Marker_Current>()._Marker = Cast<UShapeComponent>(InActorComp);
                break;
            }
            case ECk_OverlapBody_Type::Sensor:
            {
                InMarkerOrSensorEntity.Get<ck::FFragment_Sensor_Current>()._Sensor = Cast<UShapeComponent>(InActorComp);
                break;
            }
            case ECk_OverlapBody_Type::Other:
            {
                CK_TRIGGER_ENSURE(TEXT("OverlapBody Component [{}] is neither a Marker or Sensor"), InActorComp);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(OverlappedCompOverlapBodyType);
                break;
            }
        }

        const auto& CapsuleSize = InMarkerOrSensorParams.Get_ShapeParams().Get_ShapeDimensions().Get_CapsuleSize();

        MarkerOrSensorComp->SetCapsuleRadius(CapsuleSize.Get_Radius());
        MarkerOrSensorComp->SetCapsuleHalfHeight(CapsuleSize.Get_HalfHeight());
        MarkerOrSensorComp->AddLocalTransform(InMarkerOrSensorParams.Get_RelativeTransform());
        // we never want scale to affect our Sensors/Markers
        MarkerOrSensorComp->SetWorldScale3D(FVector::OneVector);

        const auto& PhysicsParams = InMarkerOrSensorParams.Get_PhysicsParams();
        const auto& StartingState = InMarkerOrSensorParams.Get_StartingState();

        Set_MarkerOrSensor_PhysicsParams(MarkerOrSensorComp, PhysicsParams, StartingState);

        if (InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
        {
            constexpr auto CallModifyOnComponent = true;
            MarkerOrSensorComp->DetachFromComponent
            (
                FDetachmentTransformRules
                {
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    CallModifyOnComponent
                }
            );
        }
    };

    return InitializerFunc;
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorPhysicsParams>
auto
    UCk_Utils_MarkerAndSensor_UE::
    Set_MarkerOrSensor_PhysicsParams(
        T_MarkerOrSensorCompType* InMarkerOrSensorComp,
        T_MarkerOrSensorPhysicsParams InMarkerOrSensorPhysicsParams,
        ECk_EnableDisable InEnableGenerateOverlapEvents)
    -> void
{
    const auto& CollisionProfileName = InMarkerOrSensorPhysicsParams.Get_CollisionProfileName();

    const auto& CollisionDetectionType = InMarkerOrSensorPhysicsParams.Get_CollisionType();
    const auto& NavigationEffect       = InMarkerOrSensorPhysicsParams.Get_NavigationEffect();
    const auto& OverlapBehavior        = InMarkerOrSensorPhysicsParams.Get_OverlapBehavior();
    const auto& CollisionEnabled       = InEnableGenerateOverlapEvents == ECk_EnableDisable::Enable
                                         ? ECollisionEnabled::QueryOnly
                                         : ECollisionEnabled::NoCollision;

    UCk_Utils_Physics_UE::Request_SetCollisionProfileName(InMarkerOrSensorComp, CollisionProfileName.Name);
    UCk_Utils_Physics_UE::Request_SetCollisionDetectionType(InMarkerOrSensorComp, CollisionDetectionType);
    UCk_Utils_Physics_UE::Request_SetNavigationEffects(InMarkerOrSensorComp, NavigationEffect);
    UCk_Utils_Physics_UE::Request_SetOverlapBehavior(InMarkerOrSensorComp, OverlapBehavior);
    UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(InMarkerOrSensorComp, InEnableGenerateOverlapEvents);
    UCk_Utils_Physics_UE::Request_SetCollisionEnabled(InMarkerOrSensorComp, CollisionEnabled);
}

// --------------------------------------------------------------------------------------------------------------------

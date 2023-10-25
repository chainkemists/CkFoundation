#pragma once

#include "CkActor/ActorModifier/CkActorModifier_Utils.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"

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
        FCk_Handle InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, ECk_ShapeType T_MarkerOrSensorShapeType, typename T_MarkerOrSensorParams>
    static auto
    Add_MarkerOrSensor_ActorComponent(
        FCk_Handle InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType>
    static auto
    Make_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
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
        FCk_Handle InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> void;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Box == T_MarkerOrSensorShapeType, uint8> = 0>
    static auto
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Sphere == T_MarkerOrSensorShapeType, uint8> = 0>
    static auto
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType;

    template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Capsule == T_MarkerOrSensorShapeType, uint8> = 0>
    static auto
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
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

    const auto& debugParams     = InMarkerOrSensorParams.Get_DebugParams();
    const auto& shapeParams     = InMarkerOrSensorParams.Get_ShapeParams();
    const auto& shapeDimensions = shapeParams.Get_ShapeDimensions();

    constexpr auto usePersistentDebugDrawLines = false;
    constexpr auto debugLineLifetime = -1.0f;

    const auto& [shapeComp, enableDisable] = [&]() -> TTuple<UShapeComponent*, ECk_EnableDisable>
    {
        if constexpr (std::is_same_v<T_MarkerOrSensorCurrent, ck::FFragment_Marker_Current>)
        {
            const ck::FFragment_Marker_Current& markerCurrent = InMarkerOrSensorCurrent;
            const auto& marker = markerCurrent.Get_Marker().Get();

            return MakeTuple(marker, markerCurrent.Get_EnableDisable());
        }
        else if constexpr (std::is_same_v<T_MarkerOrSensorCurrent, ck::FFragment_Sensor_Current>)
        {
            const ck::FFragment_Sensor_Current& sensorCurrent = InMarkerOrSensorCurrent;
            const auto& sensor = sensorCurrent.Get_Sensor().Get();

            return MakeTuple(sensor, sensorCurrent.Get_EnableDisable());
        }
        else
        {
            static_assert("Unknown Marker/Sensor Current struct type passed to Draw_MarkerOrSensor_DebugLines");
            return {};
        }
    }();

    CK_ENSURE_IF_NOT(ck::IsValid(shapeComp), TEXT("Invalid Marker/Sensor ActorComp stored to draw debug lines for"))
    { return; }

    const auto& transform  = shapeComp->GetComponentTransform();
    const auto& isDisabled = enableDisable == ECk_EnableDisable::Disable;

    const auto& colorMultiplier         = isDisabled ? 0.4f : 1.0f;
    const auto& lineThicknessMultiplier = isDisabled ? 1.0f : 2.0f;

    const auto& debugLineThickness = debugParams.Get_LineThickness() * lineThicknessMultiplier;
    const auto& debugLineColor     = UCk_Utils_Graphics_UE::Get_ModifiedColorIntensity(debugParams.Get_DebugLineColor(), colorMultiplier);

    switch (const auto& shapeType = shapeDimensions.Get_ShapeType())
    {
        case ECk_ShapeType::Box:
        {
            const auto& box = shapeDimensions.Get_BoxExtents();

            DrawDebugBox
            (
                InOuter->GetWorld(),
                transform.GetLocation(),
                box.Get_Extents(),
                transform.GetRotation(),
                debugLineColor,
                usePersistentDebugDrawLines,
                debugLineLifetime,
                0.0f,
                debugLineThickness
            );

            break;
        }
        case ECk_ShapeType::Sphere:
        {
            const auto& sphere = shapeDimensions.Get_SphereRadius();

            DrawDebugSphere
            (
                InOuter->GetWorld(),
                transform.GetLocation(),
                sphere.Get_Radius(),
                12,
                debugLineColor,
                usePersistentDebugDrawLines,
                debugLineLifetime,
                0.0f,
                debugLineThickness
            );

            break;
        }
        case ECk_ShapeType::Capsule:
        {
            const auto& capsule = shapeDimensions.Get_CapsuleSize();

            DrawDebugCapsule
            (
                InOuter->GetWorld(),
                transform.GetLocation(),
                capsule.Get_HalfHeight(),
                capsule.Get_Radius(),
                transform.GetRotation(),
                debugLineColor,
                usePersistentDebugDrawLines,
                debugLineLifetime,
                0.0f,
                debugLineThickness
            );

            break;
        }
        default:
        {
            CK_INVALID_ENUM(shapeType);
            break;
        }
    }
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams>
auto
    UCk_Utils_MarkerAndSensor_UE::
    Add_MarkerOrSensor_ActorComponent(
        FCk_Handle InMarkerOrSensorEntity,
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
        FCk_Handle InMarkerOrSensorEntity,
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
        FCk_Handle InMarkerOrSensorEntity,
        const FCk_EntityOwningActor_BasicDetails& InMarkerOrSensorAttachedEntityAndActor,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> void
{
    const auto& markerOrSensorAttachedActor = InMarkerOrSensorAttachedEntityAndActor.Get_Actor().Get();
    const auto& markerAttachedActorRootComponent = markerOrSensorAttachedActor->GetRootComponent();

    CK_ENSURE_IF_NOT(ck::IsValid(markerAttachedActorRootComponent),
        TEXT("Invalid Root Component for Marker's Owning Actor [{}]!"),
        markerOrSensorAttachedActor)
    { return; }

    auto* shapeHolderComponent = UCk_Utils_Actor_UE::Request_AddNewActorComponent<USceneComponent>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<USceneComponent>
        {
            markerOrSensorAttachedActor,
            USceneComponent::StaticClass(),
            false,
            markerAttachedActorRootComponent,
            NAME_None
        }
    );

    CK_ENSURE_IF_NOT(ck::IsValid(shapeHolderComponent),
        TEXT("Failed to add the shape holder SceneComponent to Marker/Sensor Owning Actor [{}]"),
        markerOrSensorAttachedActor)
    { return; }

    const auto& actorScale            = markerOrSensorAttachedActor->GetActorScale3D();
    const auto& actorScaleXNearlyZero = FMath::IsNearlyZero(actorScale.X);
    const auto& actorScaleYNearlyZero = FMath::IsNearlyZero(actorScale.Y);
    const auto& actorScaleZNearlyZero = FMath::IsNearlyZero(actorScale.Z);

    CK_ENSURE_IF_NOT(NOT actorScaleXNearlyZero && NOT actorScaleYNearlyZero && NOT actorScaleZNearlyZero,
        TEXT("Marker/Sensor Owning Actor [{}] has a scale that is too small! Scale: [{}]"),
        markerOrSensorAttachedActor,
        actorScale)
    {
        const auto& adjustedShapeHolderScale = FVector::OneVector / actorScale;
        shapeHolderComponent->SetRelativeScale3D(adjustedShapeHolderScale);
    }

    const auto Make_MarkerOrSensor_ComponentParams = [&]() -> FCk_AddActorComponent_Params
    {
        const auto& attachmentType = InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach
                               ? ECk_ActorComponent_AttachmentPolicy::DoNotAttach
                               : ECk_ActorComponent_AttachmentPolicy::Attach;

        static constexpr auto isComponentTickEnabled = true;

        return FCk_AddActorComponent_Params{ isComponentTickEnabled, FCk_Time::Zero, attachmentType, shapeHolderComponent, NAME_None };
    };

    UCk_Utils_ActorModifier_UE::Request_AddActorComponent
    (
        InMarkerOrSensorAttachedEntityAndActor.Get_Handle(),
        FCk_Request_ActorModifier_AddActorComponent
        {
            T_MarkerOrSensorCompType::StaticClass(),
            false,
            Make_MarkerOrSensor_ComponentParams(),
            Make_MarkerOrSensor_InitializerFunction<T_MarkerOrSensorCompType, T_MarkerOrSensorParams, T_MarkerOrSensorShapeType>(InMarkerOrSensorEntity, InMarkerOrSensorParams)
        },
        {}
    );
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType>
auto
    UCk_Utils_MarkerAndSensor_UE::
    Make_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams) -> InitializerFuncType
{
    return DoMake_MarkerOrSensor_InitializerFunction<T_MarkerOrSensorCompType, T_MarkerOrSensorParams, T_MarkerOrSensorShapeType>(InMarkerOrSensorEntity, InMarkerOrSensorParams);
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Box == T_MarkerOrSensorShapeType, uint8>>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> InitializerFuncType
{
    auto initializerFunc = [=](UActorComponent* InActorComp) mutable -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InActorComp), TEXT("Invalid Actor Component"))
        { return; }

        auto* markerOrSensorComp = Cast<T_MarkerOrSensorCompType>(InActorComp);

        CK_ENSURE_IF_NOT(ck::IsValid(markerOrSensorComp), TEXT("Invalid Marker/Sensor Actor Component"))
        { return; }

        markerOrSensorComp->_OwningEntity = InMarkerOrSensorEntity;

        const auto* overlappedComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InActorComp);

        switch(const auto& overlappedCompOverlapBodyType = overlappedComponentAsOverlapBody->Get_Type())
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
                CK_ENSURE_FALSE(TEXT("OverlapBody Component [{}] is neither a Marker or Sensor"), InActorComp);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(overlappedCompOverlapBodyType);
                break;
            }
        }

        const auto& physicsParams = InMarkerOrSensorParams.Get_PhysicsParams();
        const auto& startingState = InMarkerOrSensorParams.Get_StartingState();

        Set_MarkerOrSensor_PhysicsParams(markerOrSensorComp, physicsParams, startingState);

        const auto& boxExtents = InMarkerOrSensorParams.Get_ShapeParams().Get_ShapeDimensions().Get_BoxExtents().Get_Extents();

        markerOrSensorComp->SetBoxExtent(boxExtents);
        markerOrSensorComp->AddLocalTransform(InMarkerOrSensorParams.Get_RelativeTransform());

        if (InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
        {
            static constexpr auto callModifyOnComponent = true;
            markerOrSensorComp->DetachFromComponent
            (
                FDetachmentTransformRules
                {
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    callModifyOnComponent
                }
            );
        }
    };

    return initializerFunc;
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Sphere == T_MarkerOrSensorShapeType, uint8>>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> InitializerFuncType
{
    auto initializerFunc = [=](UActorComponent* InActorComp) mutable -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InActorComp), TEXT("Invalid Actor Component"))
        { return; }

        auto* markerOrSensorComp = Cast<T_MarkerOrSensorCompType>(InActorComp);

        CK_ENSURE_IF_NOT(ck::IsValid(markerOrSensorComp), TEXT("Invalid Marker/Sensor Actor Component"))
        { return; }

        markerOrSensorComp->_OwningEntity = InMarkerOrSensorEntity;

        const auto* overlappedComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InActorComp);

        switch(const auto& overlappedCompOverlapBodyType = overlappedComponentAsOverlapBody->Get_Type())
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
                CK_ENSURE_FALSE(TEXT("OverlapBody Component [{}] is neither a Marker or Sensor"), InActorComp);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(overlappedCompOverlapBodyType);
                break;
            }
        }

        const auto& physicsParams = InMarkerOrSensorParams.Get_PhysicsParams();
        const auto& startingState = InMarkerOrSensorParams.Get_StartingState();

        Set_MarkerOrSensor_PhysicsParams(markerOrSensorComp, physicsParams, startingState);

        const auto& sphereRadius = InMarkerOrSensorParams.Get_ShapeParams().Get_ShapeDimensions().Get_SphereRadius().Get_Radius();

        markerOrSensorComp->SetSphereRadius(sphereRadius);
        markerOrSensorComp->AddLocalTransform(InMarkerOrSensorParams.Get_RelativeTransform());

        if (InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
        {
            constexpr auto callModifyOnComponent = true;
            markerOrSensorComp->DetachFromComponent
            (
                FDetachmentTransformRules
                {
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    callModifyOnComponent
                }
            );
        }
    };

    return initializerFunc;
}

template <typename T_MarkerOrSensorCompType, typename T_MarkerOrSensorParams, ECk_ShapeType T_MarkerOrSensorShapeType, std::enable_if_t<ECk_ShapeType::Capsule == T_MarkerOrSensorShapeType, uint8>>
auto
    UCk_Utils_MarkerAndSensor_UE::
    DoMake_MarkerOrSensor_InitializerFunction(
        FCk_Handle InMarkerOrSensorEntity,
        const T_MarkerOrSensorParams& InMarkerOrSensorParams)
    -> InitializerFuncType
{
    auto initializerFunc = [=](UActorComponent* InActorComp) mutable -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InActorComp), TEXT("Invalid Actor Component"))
        { return; }

        auto* markerOrSensorComp = Cast<T_MarkerOrSensorCompType>(InActorComp);

        CK_ENSURE_IF_NOT(ck::IsValid(markerOrSensorComp), TEXT("Invalid Marker/Sensor Actor Component"))
        { return; }

        markerOrSensorComp->_OwningEntity = InMarkerOrSensorEntity;

        const auto* overlappedComponentAsOverlapBody = Cast<ICk_OverlapBody_Interface>(InActorComp);

        switch(const auto& overlappedCompOverlapBodyType = overlappedComponentAsOverlapBody->Get_Type())
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
                CK_ENSURE_FALSE(TEXT("OverlapBody Component [{}] is neither a Marker or Sensor"), InActorComp);
                break;
            }
            default:
            {
                CK_INVALID_ENUM(overlappedCompOverlapBodyType);
                break;
            }
        }

        const auto& physicsParams = InMarkerOrSensorParams.Get_PhysicsParams();
        const auto& startingState = InMarkerOrSensorParams.Get_StartingState();

        Set_MarkerOrSensor_PhysicsParams(markerOrSensorComp, physicsParams, startingState);

        const auto& capsuleSize = InMarkerOrSensorParams.Get_ShapeParams().Get_ShapeDimensions().Get_CapsuleSize();

        markerOrSensorComp->SetCapsuleRadius(capsuleSize.Get_Radius());
        markerOrSensorComp->SetCapsuleHalfHeight(capsuleSize.Get_HalfHeight());
        markerOrSensorComp->AddLocalTransform(InMarkerOrSensorParams.Get_RelativeTransform());

        if (InMarkerOrSensorParams.Get_AttachmentParams().Get_AttachmentType() == ECk_ActorComponent_AttachmentPolicy::DoNotAttach)
        {
            constexpr auto callModifyOnComponent = true;
            markerOrSensorComp->DetachFromComponent
            (
                FDetachmentTransformRules
                {
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    EDetachmentRule::KeepWorld,
                    callModifyOnComponent
                }
            );
        }
    };

    return initializerFunc;
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
    const auto& collisionProfileName = InMarkerOrSensorPhysicsParams.Get_CollisionProfileName();

    const auto& collisionDetectionType = InMarkerOrSensorPhysicsParams.Get_CollisionType();
    const auto& navigationEffect       = InMarkerOrSensorPhysicsParams.Get_NavigationEffect();
    const auto& overlapBehavior        = InMarkerOrSensorPhysicsParams.Get_OverlapBehavior();
    const auto& collisionEnabled       = InEnableGenerateOverlapEvents == ECk_EnableDisable::Enable
                                         ? ECollisionEnabled::QueryOnly
                                         : ECollisionEnabled::NoCollision;

    UCk_Utils_Physics_UE::Request_SetCollisionProfileName(InMarkerOrSensorComp, collisionProfileName);
    UCk_Utils_Physics_UE::Request_SetCollisionDetectionType(InMarkerOrSensorComp, collisionDetectionType);
    UCk_Utils_Physics_UE::Request_SetNavigationEffects(InMarkerOrSensorComp, navigationEffect);
    UCk_Utils_Physics_UE::Request_SetOverlapBehavior(InMarkerOrSensorComp, overlapBehavior);
    UCk_Utils_Physics_UE::Request_SetCollisionProfileName(InMarkerOrSensorComp, collisionProfileName.Name);
    UCk_Utils_Physics_UE::Request_SetGenerateOverlapEvents(InMarkerOrSensorComp, InEnableGenerateOverlapEvents);
    UCk_Utils_Physics_UE::Request_SetCollisionEnabled(InMarkerOrSensorComp, collisionEnabled);
}

// --------------------------------------------------------------------------------------------------------------------

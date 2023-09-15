#include "CkMarker_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Marker_BasicDetails::
    FCk_Marker_BasicDetails(
        FGameplayTag InMarkerName,
        FCk_Handle InMarkerEntity,
        FCk_EntityOwningActor_BasicDetails InMarkerAttachedEntityAndActor)
    : _MarkerName(InMarkerName)
    , _MarkerEntity(InMarkerEntity)
    , _MarkerAttachedEntityAndActor(InMarkerAttachedEntityAndActor)
{
}

auto
    FCk_Marker_BasicDetails::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_MarkerName() == InOther.Get_MarkerName() &&
           Get_MarkerEntity() == InOther.Get_MarkerEntity() &&
           Get_MarkerAttachedEntityAndActor() == InOther.Get_MarkerAttachedEntityAndActor();
}

auto
    GetTypeHash(
        const FCk_Marker_BasicDetails& InBasicDetails)
    -> uint32
{
    return GetTypeHash(InBasicDetails.Get_MarkerName()) +
           GetTypeHash(InBasicDetails.Get_MarkerEntity()) +
           GetTypeHash(InBasicDetails.Get_MarkerAttachedEntityAndActor());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Marker_DebugInfo::
    FCk_Marker_DebugInfo(
        float InLineThickness,
        FColor InDebugLineColor)
    : _LineThickness(InLineThickness)
    , _DebugLineColor(InDebugLineColor)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Marker_PhysicsInfo::
    FCk_Marker_PhysicsInfo(
        ECk_CollisionDetectionType InCollisionType,
        ECk_NavigationEffect InNavigationEffect,
        ECk_ComponentOverlapBehavior InOverlapBehavior,
        FName InCollisionProfileName)
    : _CollisionType(InCollisionType)
    , _NavigationEffect(InNavigationEffect)
    , _OverlapBehavior(InOverlapBehavior)
    , _CollisionProfileName(InCollisionProfileName)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Marker_AttachmentInfo::
    FCk_Marker_AttachmentInfo(
        ECk_ActorComponent_AttachmentPolicy     InAttachmentType,
        ECk_Marker_BoneTransform_UsagePolicy    InUseBoneTransformOrNot,
        FName                                   InBoneName,
        ECk_Marker_BoneTransform_PositionPolicy InUseBonePosition,
        ECk_Marker_BoneTransform_RotationPolicy InUseBoneRotation)
    : _AttachmentType(InAttachmentType)
    , _UseBoneTransformOrNot(InUseBoneTransformOrNot)
    , _BoneName(InBoneName)
    , _UseBonePosition(InUseBonePosition)
    , _UseBoneRotation(InUseBoneRotation)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Marker_ShapeInfo::
    FCk_Marker_ShapeInfo(
        FCk_ShapeDimensions InShapeDimensions)
        : _ShapeDimensions(InShapeDimensions)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Marker_EnableDisable::
    FCk_Request_Marker_EnableDisable(
        ECk_EnableDisable InEnableDisable)
    : _EnableDisable(InEnableDisable)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Marker_ParamsData::
    FCk_Fragment_Marker_ParamsData(
        FGameplayTag              InMarkerName,
        FCk_Marker_ShapeInfo      InShapeParams,
        FCk_Marker_PhysicsInfo    InPhysicsParams,
        FCk_Marker_AttachmentInfo InAttachmentParams,
        FTransform                InRelativeTransform,
        ECk_EnableDisable         InStartingState,
        ECk_Net_ReplicationType   InReplicationType,
        bool                      InShowDebug,
        FCk_Marker_DebugInfo      InDebugParams)
    : _MarkerName(InMarkerName)
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
    UCk_Marker_ActorComponent_Box_UE::
    Get_Type() const
    -> ECk_OverlapBody_Type
{
    return ECk_OverlapBody_Type::Marker;
}

auto
    UCk_Marker_ActorComponent_Box_UE::
    Get_OwningEntity() const
    -> FCk_Handle
{
    return _OwningEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Marker_ActorComponent_Sphere_UE::
    Get_Type() const
    -> ECk_OverlapBody_Type
{
    return ECk_OverlapBody_Type::Marker;
}

auto
    UCk_Marker_ActorComponent_Sphere_UE::
    Get_OwningEntity() const
    -> FCk_Handle
{
    return _OwningEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Marker_ActorComponent_Capsule_UE::
    Get_Type() const
    -> ECk_OverlapBody_Type
{
    return ECk_OverlapBody_Type::Marker;
}

auto
    UCk_Marker_ActorComponent_Capsule_UE::
    Get_OwningEntity() const
    -> FCk_Handle
{
    return _OwningEntity;
}

// --------------------------------------------------------------------------------------------------------------------

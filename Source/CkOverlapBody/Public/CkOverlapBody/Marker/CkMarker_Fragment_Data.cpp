#include "CkMarker_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Marker, TEXT("Marker"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Marker_BasicDetails::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_MarkerName() == InOther.Get_MarkerName() &&
           Get_MarkerEntity() == InOther.Get_MarkerEntity();
}

auto
    GetTypeHash(
        const FCk_Marker_BasicDetails& InBasicDetails)
    -> uint32
{
    return GetTypeHash(InBasicDetails.Get_MarkerName()) +
           GetTypeHash(InBasicDetails.Get_MarkerEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Marker_AttachmentInfo::
    Get_AttachmentPolicy() const
    -> ECk_Marker_AttachmentPolicy
{
    return static_cast<ECk_Marker_AttachmentPolicy>(_AttachmentPolicyFlags);
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
    -> const FCk_Handle&
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
    -> const FCk_Handle&
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
    -> const FCk_Handle&
{
    return _OwningEntity;
}

// --------------------------------------------------------------------------------------------------------------------

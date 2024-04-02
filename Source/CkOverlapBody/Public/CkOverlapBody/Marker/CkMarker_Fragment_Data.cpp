#include "CkMarker_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Marker_BasicInfo::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_Marker() == InOther.Get_Marker() &&
           Get_Owner() == InOther.Get_Owner();
}

auto
    GetTypeHash(
        const FCk_Marker_BasicInfo& InBasicDetails)
    -> uint32
{
    return GetTypeHash(InBasicDetails.Get_Marker()) +
           GetTypeHash(InBasicDetails.Get_Owner());
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

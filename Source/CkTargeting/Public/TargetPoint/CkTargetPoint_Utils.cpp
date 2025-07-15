#include "CkTargetPoint_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcs/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TargetPoint_UE::
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_Transform
{
    auto TargetPointEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(TargetPointEntity, *ck::Format_UE(TEXT("TARGET POINT: [{}]"), InTransform));

    if (InLifetime == ECk_Lifetime::AfterOneFrame)
    { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(TargetPointEntity); }

    return UCk_Utils_Transform_UE::Add(TargetPointEntity, InTransform, ECk_Replication::DoesNotReplicate);
}

auto
    UCk_Utils_TargetPoint_UE::
    Create_FromLocation(
        const FCk_Handle& InOwner,
        const FVector& InLocation,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_Transform
{
    return Create(InOwner, FTransform{InLocation}, InLifetime);
}

auto
    UCk_Utils_TargetPoint_UE::
    Create_Transient_FromLocation(
        const FVector& InLocation,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_Transform
{
    return Create_Transient(FTransform{InLocation}, InWorldContextObject, InLifetime);
}

auto
    UCk_Utils_TargetPoint_UE::
    Create_Transient(
        const FTransform& InTransform,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_Transform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject),
        TEXT("WorldContextObject [{}] is INVALID. Unable to Create a Transient TargetPoint Entity"), InWorldContextObject)
    { return {}; }

    auto TargetPointEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    UCk_Utils_Handle_UE::Set_DebugName(TargetPointEntity, *ck::Format_UE(TEXT("(Transient) TARGET POINT: [{}]"), InTransform));

    if (InLifetime == ECk_Lifetime::AfterOneFrame)
    { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(TargetPointEntity); }

    return UCk_Utils_Transform_UE::Add(TargetPointEntity, InTransform, ECk_Replication::DoesNotReplicate);
}

// --------------------------------------------------------------------------------------------------------------------

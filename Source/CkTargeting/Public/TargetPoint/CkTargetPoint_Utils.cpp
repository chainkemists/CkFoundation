#include "CkTargetPoint_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TargetPoint_UE::
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform)
    -> FCk_Handle_Transform
{
    auto TargetPointEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(TargetPointEntity, *ck::Format_UE(TEXT("TARGET POINT: [{}]"), InTransform));

    return UCk_Utils_Transform_UE::Add(TargetPointEntity, InTransform, ECk_Replication::DoesNotReplicate);
}

auto
    UCk_Utils_TargetPoint_UE::
    Create_Transient(
        const FTransform& InTransform,
        const UObject* InWorldContextObject)
    -> FCk_Handle_Transform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject),
        TEXT("WorldContextObject [{}] is INVALID. Unable to Create a Transient TargetPoint Entity"), InWorldContextObject)
    { return {}; }

    const auto& TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(InWorldContextObject->GetWorld());

    auto TargetPointEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(TransientEntity);
    UCk_Utils_Handle_UE::Set_DebugName(TargetPointEntity, *ck::Format_UE(TEXT("TARGET POINT: [{}]"), InTransform));

    return UCk_Utils_Transform_UE::Add(TargetPointEntity, InTransform, ECk_Replication::DoesNotReplicate);
}

// --------------------------------------------------------------------------------------------------------------------

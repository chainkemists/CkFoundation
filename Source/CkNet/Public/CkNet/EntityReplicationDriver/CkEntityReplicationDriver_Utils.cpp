#include "CkEntityReplicationDriver_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_EntityReplicationDriver_UE::
    Add(
        const FCk_Handle InHandle)
    -> void
{
    TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_Replicate(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo)
    -> void
{
    CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Entity [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of Entity with ConstructionScript [{}]"),
        InHandle,
        InConstructionInfo.Get_ConstructionScript())
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionInfo.Get_ConstructionScript()),
        TEXT("Unable to ReplicateEntity as ConstructionScript is [{}]"),
        InConstructionInfo.Get_ConstructionScript())
    { return; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    UCk_Utils_Net_UE::Copy(InHandle, NewEntity);
    InConstructionInfo.Get_ConstructionScript()->Construct(NewEntity);

    switch(const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle))
    {
        case ECk_Net_NetRoleType::Client:
        {
            InHandle.AddOrGet<ck::FFragment_ReplicationDriver_Requests>()._Requests.Emplace(
                FCk_EntityReplicationDriver_ConstructionInfo{InConstructionInfo}
                .Set_OriginalEntity(NewEntity.Get_Entity()));
            break;
        }
        case ECk_Net_NetRoleType::Host:
        case ECk_Net_NetRoleType::Server:
        {
            const auto& RepDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewEntity);

            RepDriver->Set_ReplicationData
            (
                FCk_EntityReplicationDriver_ReplicationData
                {
                    InConstructionInfo,
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{ReplicatedObjects.Get_ReplicatedObjects()}
                }.Set_OwningEntityDriver(InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
            );
            break;
        }
        case ECk_Net_NetRoleType::None:
        default:
            CK_INVALID_ENUM(NetMode);
            break;
    }
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_ReplicateEntityOnReplicatedActor(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor& InConstructionInfo)
    -> void
{
    const auto& RepDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    RepDriver->Set_ReplicationData_ReplicatedActor(InConstructionInfo);
}

// --------------------------------------------------------------------------------------------------------------------

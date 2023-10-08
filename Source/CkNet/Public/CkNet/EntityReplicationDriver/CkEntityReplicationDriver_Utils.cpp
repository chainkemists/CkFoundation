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
    //const auto EntityWithActor = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwnerIf(InHandle, [](FCk_Handle Handle)
    //{
    //    return UCk_Utils_OwningActor_UE::Has(Handle);
    //});

    //CK_ENSURE_IF_NOT(ck::IsValid(EntityWithActor), TEXT("Invalid Actor"))
    //{ return; }

    //const auto& BasicDetails   = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(EntityWithActor);
    //const auto& OwningActor    = BasicDetails.Get_Actor().Get();
    //const auto& OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    //CK_ENSURE_IF_NOT(ck::IsValid(OutermostActor),
    //    TEXT("OutermostActor for Actor [{}] with Entity [{}] is [{}]. Does this Entity require replication?"),
    //    OwningActor, EntityWithActor, OutermostActor)
    //{ return; }

    //const auto Subsystem = OutermostActor->GetWorld()->GetSubsystem<UCk_EntityReplicationDriver_Subsystem_UE>();
    //Subsystem->CreateEntityReplicationDrivers(OutermostActor);

    //TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle, Subsystem->Get_NextAvailableReplicationDriver());
    TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_Replicate(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo)
    -> void
{
    //CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
    //    TEXT("Entity [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of Entity with ConstructionScript [{}]"),
    //    InHandle,
    //    InConstructionInfo.Get_ConstructionScript())
    //{ return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionInfo.Get_ConstructionScript()),
        TEXT("Unable to ReplicateEntity as ConstructionScript is [{}]"),
        InConstructionInfo.Get_ConstructionScript())
    { return; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
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
            Add(NewEntity);

            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewEntity);

            auto ReplicatedObjectsData = FCk_EntityReplicationDriver_ReplicateObjects_Data{};

            for (const auto& ReplicatedObject : ReplicatedObjects.Get_ReplicatedObjects())
            {
                CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObject),
                    TEXT("Invalid Replicated Object encountered for Entity [{}] on the SERVER.[{}]"),
                    NewEntity,
                    ck::Context(InHandle))
                { continue; }

                ReplicatedObjectsData.Update_Objects([&](auto& InReplicatedObjects)
                    { InReplicatedObjects.Emplace(ReplicatedObject->GetClass()); });

                ReplicatedObjectsData.Update_NetStableNames([&](auto& InNetStableNames)
                    { InNetStableNames.Emplace(ReplicatedObject->GetFName()); });
            }

            const auto& RepDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

            RepDriver->Update_ReplicationData(
                [&](TArray<FCk_EntityReplicationDriver_ReplicationData>& ReplicationData)
                {
                    ReplicationData.Emplace(FCk_EntityReplicationDriver_ReplicationData{InConstructionInfo, ReplicatedObjectsData});
                });
            break;
        }
        case ECk_Net_NetRoleType::None:
        default:
            CK_INVALID_ENUM(NetMode);
            break;
    }









    //InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()->Request_ReplicateEntity(InConstructionInfo);
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

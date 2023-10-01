#include "CkEntityReplicationDriver_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Request_ReplicateEntity_Implementation(
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionInfo.Get_ConstructionScript()),
        TEXT("Unable to ReplicateEntity as ConstructionScript is [{}].[{}]"),
        InConstructionInfo.Get_ConstructionScript(),
        ck::Context(this))
    { return; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Get_AssociatedEntity());
    InConstructionInfo.Get_ConstructionScript()->Construct(NewEntity);
    UCk_Utils_EntityReplicationDriver_UE::Add(NewEntity);

    const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewEntity);

    auto ReplicatedObjectsData = FCk_EntityReplicationDriver_ReplicateObjects_Data{};

    for (const auto& ReplicatedObject : ReplicatedObjects.Get_ReplicatedObjects())
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObject),
            TEXT("Invalid Replicated Object encountered for Entity [{}] on the SERVER.[{}]"),
            NewEntity,
            ck::Context(this))
        { continue; }

        ReplicatedObjectsData.Update_Objects([&](auto& InReplicatedObjects)
            { InReplicatedObjects.Emplace(ReplicatedObject->GetClass()); });

        ReplicatedObjectsData.Update_NetStableNames([&](auto& InNetStableNames)
            { InNetStableNames.Emplace(ReplicatedObject->GetFName()); });
    }

    _ReplicationData.Emplace(FCk_EntityReplicationDriver_ReplicationData{InConstructionInfo, ReplicatedObjectsData});
}

// --------------------------------------------------------------------------------------------------------------------
void
    UCk_Fragment_EntityReplicationDriver_Rep::GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _ReplicationData);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData()
    -> void
{
    for (auto Index = _ReplicateFrom; Index < _ReplicationData.Num(); ++Index)
    {
        // --------------------------------------------------------------------------------------------------------------------

        const auto& ReplicationData = _ReplicationData[Index];
        const auto& ConstructionInfo = ReplicationData.Get_ConstructionInfo();
        const auto& ReplicatedObjects = ReplicationData.Get_ReplicatedObjectsData();

        FCk_ReplicatedObjects ROs;

        CK_ENSURE_IF_NOT(ReplicatedObjects.Get_Objects().Num() == ReplicatedObjects.Get_NetStableNames().Num(),
            TEXT("Expected Objects Array to be the same size as the associated Names Array. "
                "Unable to proceed with construction of Replicated Objects.[{}]"), ck::Context(this))
        { return; }

        ck::algo::ForEachView(ReplicatedObjects.Get_Objects(), ReplicatedObjects.Get_NetStableNames(),
        [&](TSubclassOf<UCk_Ecs_ReplicatedObject_UE> InRepObjectClass, FName InNetStableName)
        {
            // TODO: Sending garbage entity handle until we manage to link it up properly
            ROs.Update_ReplicatedObjects([&](TArray<UCk_ReplicatedObject_UE*>& InROs)
            {
                InROs.Emplace(Create(InRepObjectClass, Get_ReplicatedActor(), InNetStableName, FCk_Handle{}));
            });
        });

        // --------------------------------------------------------------------------------------------------------------------

        const auto NewOrExistingEntity = [&]()
        {
            if (Get_AssociatedEntity()->IsValid(ConstructionInfo.Get_OriginalEntity()))
            {
                return ck::MakeHandle(ConstructionInfo.Get_OriginalEntity(), Get_AssociatedEntity());
            }

            return UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Get_AssociatedEntity());
        }();

        UCk_Utils_Net_UE::Copy(Get_AssociatedEntity(), NewOrExistingEntity);

        ConstructionInfo.Get_ConstructionScript()->Construct(NewOrExistingEntity);

        UCk_Utils_ReplicatedObjects_UE::Add(NewOrExistingEntity, ROs);
    }
}

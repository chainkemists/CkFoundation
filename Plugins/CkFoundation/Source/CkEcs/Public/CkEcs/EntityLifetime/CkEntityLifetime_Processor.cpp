#include "CkEntityLifetime_Processor.h"

#include <CkCore/Actor/CkActor_Utils.h>

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    FCk_Processor_EntityLifetime_EntityJustCreated::
        FCk_Processor_EntityLifetime_EntityJustCreated(
            const FRegistryType& InRegistry)
        : _Registry(InRegistry)
    {
    }

    auto
        FCk_Processor_EntityLifetime_EntityJustCreated::
        Tick(
            FTimeType)
        -> void
    {
        _Registry.Clear<FCk_Tag_EntityJustCreated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_EntityLifetime_TriggerDestroyEntity::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        Super::Tick(InDeltaT);

        _Registry.Clear<FCk_Tag_TriggerDestroyEntity>();
    }

    auto
        FCk_Processor_EntityLifetime_TriggerDestroyEntity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("Entity [{}] set to 'Pending Destroy'"), InHandle);
        InHandle.Add<FCk_Tag_PendingDestroyEntity>(InHandle);

        if (InHandle.Has<FCk_Tag_Replicated>())
        {
            ck::algo::ForEachIsValid(UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(InHandle).Get_ReplicatedObjects(),
                [&](UCk_ReplicatedObject_UE* InRO)
                {
                    const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(InRO);

                    // In this case, we are one of the clients and we do NOT need to go any further
                    if (ck::Is_NOT_Valid(OutermostActor))
                    { return; }

                    const auto EcsReplicatedObject = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO);

                    EcsReplicatedObject->Request_TriggerDestroyAssociatedEntity();
                });
        }

        // TODO: Invoke Signal when Signals are ready
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_EntityLifetime_PendingDestroyEntity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("Destroying Entity [{}]"), InHandle);

        InHandle.Remove<FCk_Tag_PendingDestroyEntity>();

        InHandle->DestroyEntity(InHandle.Get_Entity());
    }

    // --------------------------------------------------------------------------------------------------------------------
}

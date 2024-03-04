#include "CkEntityLifetime_Processor.h"

#include <CkCore/Actor/CkActor_Utils.h>

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_EntityLifetime_EntityJustCreated::
        FProcessor_EntityLifetime_EntityJustCreated(
            const FRegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    auto
        FProcessor_EntityLifetime_EntityJustCreated::
        Tick(
            FTimeType)
        -> void
    {
        _TransientEntity.Clear<FTag_EntityJustCreated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Await::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        Super::Tick(InDeltaT);

        _TransientEntity.Clear<FTag_DestroyEntity_Initiate>();
    }

    auto
        FProcessor_EntityLifetime_DestructionPhase_Await::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("Entity [{}] set to 'Awaiting Destruction'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Await>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Finalize::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        Super::Tick(InDeltaT);

        _TransientEntity.Clear<FTag_DestroyEntity_Await>();
    }

    auto
        FProcessor_EntityLifetime_DestructionPhase_Finalize::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("Entity [{}] set to 'Finalizing Destruction'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Finalize, ck::IsValid_Policy_IncludePendingKill>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestroyEntity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("Destroying Entity [{}]"), InHandle);

        if (InHandle.Has<FTag_Replicated>())
        {
            ck::algo::ForEachIsValid(UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(InHandle).Get_ReplicatedObjects(),
                [&](const TWeakObjectPtr<UCk_ReplicatedObject_UE>& InRO)
                {
                    const auto EcsRO = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO.Get());

                    if (ck::Is_NOT_Valid(EcsRO))
                    { return; }

                    UCk_Ecs_ReplicatedObject_UE::Destroy(EcsRO);

                    const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(InRO.Get());

                    // In this case, we are one of the clients and we do NOT need to go any further
                    if (ck::Is_NOT_Valid(OutermostActor, ck::IsValid_Policy_IncludePendingKill{}))
                    { return; }

                    EcsRO->Request_TriggerDestroyAssociatedEntity();
                });
        }

        InHandle->DestroyEntity(InHandle.Get_Entity());
    }

    // --------------------------------------------------------------------------------------------------------------------
}

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
        DoTick(
            FTimeType)
        -> void
    {
        _TransientEntity.Clear<FTag_EntityJustCreated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_InitiateConfirm::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'Awaiting Destruction'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Initiate_Confirm>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Await::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        Super::DoTick(InDeltaT);

        // Do NOT clear tag here. Why? Because it might clear the tag for something that was _just_ initiated to destroy
        // _TransientEntity.Clear<FTag_DestroyEntity_Initiate>();
    }

    auto
        FProcessor_EntityLifetime_DestructionPhase_Await::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'Awaiting Destruction'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Await>();
        InHandle.Remove<FTag_DestroyEntity_Initiate, ck::IsValid_Policy_IncludePendingKill>();
        InHandle.Remove<FTag_DestroyEntity_Initiate_Confirm, ck::IsValid_Policy_IncludePendingKill>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Finalize::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        Super::DoTick(InDeltaT);

        // Do NOT clear tag here, see similar comment above
        // _TransientEntity.Clear<FTag_DestroyEntity_Await>();
    }

    auto
        FProcessor_EntityLifetime_DestructionPhase_Finalize::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'Finalizing Destruction'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Finalize, ck::IsValid_Policy_IncludePendingKill>();
        InHandle.Remove<FTag_DestroyEntity_Await, ck::IsValid_Policy_IncludePendingKill>();
    }

    auto
        FProcessor_EntityLifetime_DestroyEntity::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _EntitiesToDestroy.Empty();
        Super::DoTick(InDeltaT);
        QUICK_SCOPE_CYCLE_COUNTER(DestroyEntities)
        _TransientEntity.Get_Registry().DestroyEntities(_EntitiesToDestroy);
        _EntitiesToDestroy.Empty();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestroyEntity::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Destroying Entity [{}]"), InHandle);

        if (InHandle.Has<FTag_Replicated>())
        {
            ck::algo::ForEachIsValid(UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(InHandle).Get_ReplicatedObjects(),
                [&](const TWeakObjectPtr<UCk_ReplicatedObject_UE>& InRO)
                {
                    const auto EcsRO = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO.Get());

                    if (ck::Is_NOT_Valid(EcsRO))
                    { return; }

                    UCk_Ecs_ReplicatedObject_UE::Destroy(EcsRO);

                    // In this case, we are one of the clients and we do NOT need to go any further
                    if (const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(InRO.Get());
                        ck::Is_NOT_Valid(OutermostActor, ck::IsValid_Policy_IncludePendingKill{}))
                    { return; }

                    EcsRO->Request_TriggerDestroyAssociatedEntity();
                });
        }

        if (ck::UUtils_Signal_EntityDestroyed::Has(InHandle))
        {
            ck::UUtils_Signal_EntityDestroyed::Broadcast(InHandle, ck::MakePayload(InHandle));
        }

        _EntitiesToDestroy.Emplace(InHandle.Get_Entity());
    }

    // --------------------------------------------------------------------------------------------------------------------
}
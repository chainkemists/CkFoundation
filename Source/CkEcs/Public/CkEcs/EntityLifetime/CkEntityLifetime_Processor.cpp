#include "CkEntityLifetime_Processor.h"

#include <CkCore/Actor/CkActor_Utils.h>

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityLifetime_EntityJustCreated);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityLifetime_DestructionPhase_Endplay);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityLifetime_DestructionPhase_Teardown);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityLifetime_DestructionPhase_Await);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityLifetime_DestructionPhase_Finalize);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityLifetime_DestroyEntity);

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_EntityJustCreated::
        DoTick(
            FTimeType)
        -> void
    {
        // Unconditional: "was created on the frame that just ended" is a statement about the frame, not about the
        // entity, so holding it for a quarantined entity would make it read as freshly created for the whole load.
        _TransientEntity.Clear_Unconditional<FTag_EntityJustCreated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Endplay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
            -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'End Play'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_EndPlay>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
            -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'Teardown'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Teardown>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Await::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
            -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'Destroy'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Await, ck::IsValid_Policy_IncludePendingKill>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityLifetime_DestructionPhase_Finalize::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
            -> void
    {
        ecs::VeryVerbose(TEXT("[DESTRUCTION] Entity [{}] set to 'Finalizing Destruction'"), InHandle);
        InHandle.Add<FTag_DestroyEntity_Finalize, ck::IsValid_Policy_IncludePendingKill>();
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
        _TransientEntity.Get_RegistryView().DestroyEntities(_EntitiesToDestroy);
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

        // FTag_Replicated alone isn't enough: a restored husk can carry the tag WITHOUT
        // FFragment_ReplicatedObjects_Params, and would then trip the Get<...> ensure below.
        if (InHandle.Has<FTag_Replicated>() && UCk_Utils_ReplicatedObjects_UE::Has(InHandle))
        {
            ck::algo::ForEachIsValid(UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(InHandle).Get_ReplicatedObjects(),
                [&](const TStrongObjectPtr<UCk_ReplicatedObject_UE>& InRO)
                {
                    const auto EcsRO = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO.Get());

                    if (ck::Is_NOT_Valid(EcsRO))
                    { return; }

                    UCk_Ecs_ReplicatedObject_UE::Destroy(EcsRO);

                    // In this case, we are one of the clients, and we do NOT need to go any further
                    if (const auto OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(InRO.Get());
                        ck::Is_NOT_Valid(OutermostActor, ck::IsValid_Policy_IncludePendingKill{}))
                    { return; }

                    EcsRO->Request_TriggerDestroyAssociatedEntity();
                });
        }

        _EntitiesToDestroy.Emplace(InHandle.Get_Entity());
    }

    // --------------------------------------------------------------------------------------------------------------------
}
#include "CkObjectiveOwner_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"
#include "CkObjective/Objective/CkObjective_Utils.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_ObjectiveOwner_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_ObjectiveOwner_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_ObjectiveOwner_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_objective
{
    auto
        OnObjectiveCollectionUpdated(
            const FCk_Handle_EntityCollection& InObjectiveCollection,
            const FCk_EntityCollection_Content& InPreviousContent,
            const FCk_EntityCollection_Content& InCurrentContent,
            const TArray<FCk_Handle>& InEntitiesAdded,
            const TArray<FCk_Handle>& InEntitiesRemoved)
        -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InObjectiveCollection);
        auto ObjectiveOwner = UCk_Utils_ObjectiveOwner_UE::Cast(LifetimeOwner);

        for (const auto& EntityAdded : InEntitiesAdded)
        {
            auto ObjectiveAdded = UCk_Utils_Objective_UE::Cast(EntityAdded);
            ck::UUtils_Signal_OnObjectiveOwner_ObjectiveAdded::Broadcast(ObjectiveOwner, ck::MakePayload(ObjectiveOwner, ObjectiveAdded));
        }

        for (const auto& EntityRemoved : InEntitiesRemoved)
        {
            auto ObjectiveRemoved = UCk_Utils_Objective_UE::Cast(EntityRemoved);
            ck::UUtils_Signal_OnObjectiveOwner_ObjectiveRemoved::Broadcast(ObjectiveOwner, ck::MakePayload(ObjectiveOwner, ObjectiveRemoved));
        }
    }

    // Also true during escalated full-scope passes, which tick gated processors before restore provenance is stamped.
    auto Get_IsLoadGateActive(
        const FCk_Handle& InHandle)
        -> bool
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return false; }

        const auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EcsWorld))
        { return false; }

        return EcsWorld->Get_IsLoadGateActive();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ObjectiveOwner_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Params& InParams,
            FFragment_ObjectiveOwner_Current& InCurrent)
        -> void
    {
        // Ahead of the marker consume on purpose: leaving it set re-runs this once the gate drops.
        if (ck_objective::Get_IsLoadGateActive(InHandle))
        { return; }

        InHandle.Remove<MarkedDirtyBy>();

        const auto& CollectionHandle = InCurrent.Get_ObjectivesEntityCollection();
        UUtils_Signal_EntityCollection_OnCollectionUpdated::Bind<&ck_objective::OnObjectiveCollectionUpdated>(
            CollectionHandle, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::DoNothing);

        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        { return; }

        // Restored children respawn from their own recipes; seeding on top of them is the duplicate.
        if (InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        for (const auto& DefaultObjectives = InParams.Get_DefaultObjectives();
            const auto& ObjectiveClass : DefaultObjectives)
        {
            CK_ENSURE_IF_NOT(ck::IsValid(ObjectiveClass), TEXT("Entity [{}] has an INVALID default Objective in its Params!"), InHandle)
            { continue; }

            if (ck::IsValid(ObjectiveClass))
            {
                UCk_Utils_ObjectiveOwner_UE::Request_AddObjective(InHandle, FCk_Request_ObjectiveOwner_AddObjective{ObjectiveClass}, {});
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ObjectiveOwner_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FFragment_ObjectiveOwner_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ObjectiveOwner_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                if (DoHandleRequest(InHandle, InCurrent, InRequest))
                { Result = ECk_Request_OperationResult::Succeeded; }

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_ObjectiveOwner_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FCk_Request_ObjectiveOwner_AddObjective& InRequest)
        -> bool
    {
        const auto& ObjectiveClass = InRequest.Get_ObjectiveClass();
        CK_ENSURE_IF_NOT(ck::IsValid(ObjectiveClass), TEXT("INVALID Objective Class requested to ADD to [{}]!"), InHandle)
        { return false; }

        auto ObjectiveEntityToUse = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
        const auto& AutoStartObjective = InRequest.Get_AutoStartObjective();

        const auto& PendingObjectiveEntity = UCk_Utils_EntityScript_UE::Add(ObjectiveEntityToUse, ObjectiveClass, FInstancedStruct{},
        [InHandle, AutoStartObjective](FCk_Handle InConstructedEntity)
        {
            auto ObjectiveOwner = InHandle;
            if (ck::Is_NOT_Valid(ObjectiveOwner))
            { return; }

            auto ObjectiveEntity = UCk_Utils_Objective_UE::Cast(InConstructedEntity);
            auto CollectionHandle = ObjectiveOwner.Get<FFragment_ObjectiveOwner_Current>().Get_ObjectivesEntityCollection();

            UCk_Utils_EntityCollection_UE::Request_AddEntities(CollectionHandle, FCk_Request_EntityCollection_AddEntities{{ObjectiveEntity}}, {});

            if (AutoStartObjective)
            {
                UCk_Utils_Objective_UE::Request_Start(ObjectiveEntity, FCk_Request_Objective_Start{}, {});
            }
        });

        CK_ENSURE_IF_NOT(ck::IsValid(PendingObjectiveEntity), TEXT("Failed to create new Objective of class [{}]"), ObjectiveClass)
        { return false; }

        return true;
    }

    auto
        FProcessor_ObjectiveOwner_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest)
        -> bool
    {
        const auto& ObjectiveHandle = InRequest.Get_ObjectiveHandle();

        if (ck::Is_NOT_Valid(ObjectiveHandle, ck::IsValid_Policy_IncludePendingKill{}))
        { return false; }

        auto CollectionHandle = InCurrent.Get_ObjectivesEntityCollection();

        UCk_Utils_EntityCollection_UE::Request_RemoveEntities(CollectionHandle, FCk_Request_EntityCollection_RemoveEntities{ {ObjectiveHandle} }, {});

        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ObjectiveOwner_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
#include "CkObjectiveOwner_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"
#include "CkObjective/Objective/CkObjective_Utils.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Utils.h"

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
        InHandle.Remove<MarkedDirtyBy>();

        const auto& CollectionHandle = InCurrent.Get_ObjectivesEntityCollection();
        UUtils_Signal_EntityCollection_OnCollectionUpdated::Bind<&ck_objective::OnObjectiveCollectionUpdated>(
            CollectionHandle, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::DoNothing);

        if (UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        {
            for (const auto& DefaultObjectives = InParams.Get_DefaultObjectives();
                const auto& ObjectiveClass : DefaultObjectives)
            {
                CK_ENSURE_IF_NOT(ck::IsValid(ObjectiveClass), TEXT("Entity [{}] has an INVALID default Objective in its Params!"), InHandle)
                { continue; }

                if (ck::IsValid(ObjectiveClass))
                {
                    UCk_Utils_ObjectiveOwner_UE::Request_AddObjective(InHandle, FCk_Request_ObjectiveOwner_AddObjective{ObjectiveClass});
                }
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
                DoHandleRequest(InHandle, InCurrent, InRequest);

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
        -> void
    {
        const auto& ObjectiveClass = InRequest.Get_ObjectiveClass();
        CK_ENSURE_IF_NOT(ck::IsValid(ObjectiveClass), TEXT("INVALID Objective Class requested to ADD to [{}]!"), InHandle)
        { return; }

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

            UCk_Utils_EntityCollection_UE::Request_AddEntities(CollectionHandle, FCk_Request_EntityCollection_AddEntities{{ObjectiveEntity}});

            //UUtils_Signal_OnObjectiveOwner_ObjectiveAdded::Broadcast(ObjectiveOwner, MakePayload(ObjectiveOwner, ObjectiveEntity));

            if (AutoStartObjective)
            {
                UCk_Utils_Objective_UE::Request_Start(ObjectiveEntity, FCk_Request_Objective_Start{});
            }
        });

        CK_ENSURE_IF_NOT(ck::IsValid(PendingObjectiveEntity), TEXT("Failed to create new Objective of class [{}]"), ObjectiveClass)
        { return; }
    }

    auto
        FProcessor_ObjectiveOwner_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest)
        -> void
    {
        const auto& ObjectiveHandle = InRequest.Get_ObjectiveHandle();

        if (ck::Is_NOT_Valid(ObjectiveHandle, ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        auto CollectionHandle = InCurrent.Get_ObjectivesEntityCollection();

        UCk_Utils_EntityCollection_UE::Request_RemoveEntities(CollectionHandle, FCk_Request_EntityCollection_RemoveEntities{ {ObjectiveHandle} });

        //UUtils_Signal_OnObjectiveOwner_ObjectiveRemoved::Broadcast(InHandle, MakePayload(InHandle, ObjectiveHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------
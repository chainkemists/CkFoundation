#include "CkObjectiveOwner_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkObjective/Objective/CkObjective_EntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkObjective/Objective/CkObjective_Utils.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ObjectiveOwner_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Params& InParams)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

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

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ObjectiveOwner_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_ObjectiveOwner_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InRequest);

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
            const FCk_Request_ObjectiveOwner_AddObjective& InRequest)
        -> void
    {
        const auto& ObjectiveClass = InRequest.Get_ObjectiveClass();
        CK_ENSURE_IF_NOT(ck::IsValid(ObjectiveClass), TEXT("INVALID Objective Class requested to ADD to [{}]!"), InHandle)
        { return; }

        auto ObjectiveEntityToUse = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

        const auto& PendingObjectiveEntity = UCk_Utils_EntityScript_UE::Add(ObjectiveEntityToUse, ObjectiveClass, FInstancedStruct{},
        [InHandle](FCk_Handle InConstructedEntity)
        {
            if (ck::Is_NOT_Valid(InHandle))
            { return; }

            auto ObjectiveEntity = UCk_Utils_Objective_UE::Cast(InConstructedEntity);
            auto ObjectiveOwner = InHandle;

            UCk_Utils_ObjectiveOwner_UE::RecordOfObjectives_Utils::Request_Connect(ObjectiveOwner, ObjectiveEntity);

            UUtils_Signal_OnObjectiveOwner_ObjectiveAdded::Broadcast(InHandle, MakePayload(ObjectiveOwner, ObjectiveEntity));
        });

        CK_ENSURE_IF_NOT(ck::IsValid(PendingObjectiveEntity), TEXT("Failed to create new Objective of class [{}]"), ObjectiveClass)
        { return; }
    }

    auto
        FProcessor_ObjectiveOwner_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest)
        -> void
    {
        auto ObjectiveHandle = InRequest.Get_ObjectiveHandle();

        if (ck::Is_NOT_Valid(ObjectiveHandle, ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        if (NOT UCk_Utils_ObjectiveOwner_UE::RecordOfObjectives_Utils::Request_Disconnect(InHandle, ObjectiveHandle))
        { return; }

        UUtils_Signal_OnObjectiveOwner_ObjectiveRemoved::Broadcast(InHandle, MakePayload(InHandle, ObjectiveHandle));
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ObjectiveHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
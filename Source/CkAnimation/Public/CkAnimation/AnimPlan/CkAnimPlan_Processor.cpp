#include "CkAnimPlan_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkAnimation/AnimPlan/CkAnimPlan_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AnimPlan_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AnimPlan_Params& InParams,
            FFragment_AnimPlan_Current& InCurrent,
            FFragment_AnimPlan_Requests& InRequestsComp) const
        -> void
    {
        const auto PreviousCurrent = InCurrent;

        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_AnimPlan_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);
            }));
        });

        if (PreviousCurrent.Get_AnimState() != InCurrent.Get_AnimState() || PreviousCurrent.Get_AnimCluster() != InCurrent.Get_AnimCluster())
        {
            const auto& AnimGoal = InParams.Get_Params().Get_AnimGoal();
            const auto PreviousAnimState = FCk_AnimPlan_State{AnimGoal, PreviousCurrent.Get_AnimCluster(), PreviousCurrent.Get_AnimState()};
            const auto NewAnimState = FCk_AnimPlan_State{AnimGoal, InCurrent.Get_AnimCluster(), InCurrent.Get_AnimState()};

            UUtils_Signal_AnimPlan_OnPlanChanged::Broadcast(InHandle, ck::MakePayload(InHandle, PreviousAnimState, NewAnimState));

            UCk_Utils_AnimPlan_UE::Request_TryReplicateAnimPlan(InHandle);
        }
    }

    auto
        FProcessor_AnimPlan_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimPlan_Current& InCurrent,
            const FCk_Request_AnimPlan_UpdateAnimCluster& InRequest)
        -> void
    {
        const auto& NewAnimCluster = InRequest.Get_NewAnimCluster();

        if (InCurrent.Get_AnimCluster() == NewAnimCluster)
        { return; }

        InCurrent._AnimCluster = NewAnimCluster;
        InCurrent._AnimState = FGameplayTag::EmptyTag;
    }

    auto
        FProcessor_AnimPlan_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimPlan_Current& InCurrent,
            const FCk_Request_AnimPlan_UpdateAnimState& InRequest)
        -> void
    {
        const auto& NewAnimState = InRequest.Get_NewAnimState();
        const auto& NewAnimCluster = InRequest.Get_NewAnimCluster();

        if (InCurrent.Get_AnimCluster() == NewAnimCluster && InCurrent.Get_AnimState() == NewAnimState)
        { return; }

        if (ck::IsValid(NewAnimState))
        {
            CK_ENSURE_IF_NOT(ck::IsValid(NewAnimCluster),
                TEXT("Trying to update the AnimPlan [{}] AnimState to [{}] without providing a valid AnimCluster!"),
                InHandle,
                NewAnimState)
            { return; }
        }

        InCurrent._AnimCluster = NewAnimCluster;
        InCurrent._AnimState = NewAnimState;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AnimPlan_Replicate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AnimPlan_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AnimPlan_Params& InParams,
            FFragment_AnimPlan_Current& InCurrent) const
        -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_AnimPlan_Rep>(
            LifetimeOwner, [&](UCk_Fragment_AnimPlan_Rep* InRepComp)
        {
            InRepComp->Broadcast_AddOrUpdate(FCk_AnimPlan_State{
                InParams.Get_Params().Get_AnimGoal(), InCurrent.Get_AnimCluster(), InCurrent.Get_AnimState()});
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

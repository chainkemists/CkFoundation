#include "CkAnimState_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AnimState_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AnimState_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            FFragment_AnimState_Requests& InRequestsComp) const
        -> void
    {
        InComp.Set_ComponentsModified(ECk_AnimStateComponents::None);

        const auto PreviousAnimStateCurrent = FCk_AnimState_Current{InComp.Get_AnimGoal(), InComp.Get_AnimCluster(), InComp.Get_AnimState(), InComp.Get_AnimOverlay()};

        algo::ForEachRequest(InRequestsComp._SetGoalRequest,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
        });

        algo::ForEachRequest(InRequestsComp._SetStateRequest,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
        });

        algo::ForEachRequest(InRequestsComp._SetClusterRequest,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
        });

        algo::ForEachRequest(InRequestsComp._SetOverlayRequest,
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InComp, InRequest);
        });

        const auto NewAnimStateCurrent = FCk_AnimState_Current{InComp.Get_AnimGoal(), InComp.Get_AnimCluster(), InComp.Get_AnimState(), InComp.Get_AnimOverlay()};

        if (NewAnimStateCurrent != PreviousAnimStateCurrent)
        {
            animation::VeryVerbose(TEXT("Updated AnimState [Old: {} | New: {}] of Entity [{}]"), PreviousAnimStateCurrent, NewAnimStateCurrent, InHandle);
            InHandle.AddOrGet<ck::FTag_AnimState_Updated>();
        }
    }

    auto
        FProcessor_AnimState_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetGoal& InRequest)
        -> void
    {
        const auto PreviousAnimGoal = InComp.Get_AnimGoal();
        const auto& NewAnimGoal = InRequest.Get_NewAnimGoal();

        if (PreviousAnimGoal == NewAnimGoal)
        { return; }

        animation::Verbose
        (
            TEXT("Updating AnimState Goal from [{}] to [{}] on Entity [{}]"),
            PreviousAnimGoal.Get_AnimGoal(),
            NewAnimGoal.Get_AnimGoal(),
            InHandle
        );

        InComp._AnimGoal = NewAnimGoal;
        InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_AnimStateComponents::Goal);

        UUtils_Signal_AnimState_OnGoalChanged::Broadcast(
            InHandle,
            MakePayload(InHandle,
                FCk_Payload_AnimState_OnGoalChanged{InHandle, PreviousAnimGoal, NewAnimGoal}));
    }

    auto
        FProcessor_AnimState_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,

            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetState& InRequest)
        -> void
    {
        const auto PreviousAnimState = InComp.Get_AnimState();
        const auto& NewAnimState = InRequest.Get_NewAnimState();

        if (PreviousAnimState == NewAnimState)
        { return; }

        animation::Verbose
        (
            TEXT("Updating AnimState State from [{}] to [{}] on Entity [{}]"),
            PreviousAnimState.Get_AnimState(),
            NewAnimState.Get_AnimState(),
            InHandle
        );

        InComp._AnimState = NewAnimState;
        InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_AnimStateComponents::State);

        UUtils_Signal_AnimState_OnStateChanged::Broadcast(
            InHandle,
            MakePayload(InHandle,
                FCk_Payload_AnimState_OnStateChanged{InHandle, PreviousAnimState, NewAnimState}));
    }

    auto
        FProcessor_AnimState_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetCluster& InRequest)
        -> void
    {
        const auto PreviousAnimCluster = InComp.Get_AnimCluster();
        const auto& NewAnimCluster = InRequest.Get_NewAnimCluster();

        if (PreviousAnimCluster == NewAnimCluster)
        { return; }

        animation::Verbose
        (
            TEXT("Updating AnimState Cluster from [{}] to [{}] on Entity [{}]"),
            PreviousAnimCluster.Get_AnimCluster(),
            NewAnimCluster.Get_AnimCluster(),
            InHandle
        );

        InComp._AnimCluster = NewAnimCluster;
        InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_AnimStateComponents::Cluster);

        UUtils_Signal_AnimState_OnClusterChanged::Broadcast(
            InHandle,
            MakePayload(InHandle,
                FCk_Payload_AnimState_OnClusterChanged{InHandle, PreviousAnimCluster, NewAnimCluster}));
    }

    auto
        FProcessor_AnimState_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimState_Current& InComp,
            const FCk_Request_AnimState_SetOverlay& InRequest)
        -> void
    {
        const auto PreviousAnimOverlay = InComp.Get_AnimOverlay();
        const auto& NewAnimOverlay = InRequest.Get_NewAnimOverlay();

        if (PreviousAnimOverlay == NewAnimOverlay)
        { return; }

        animation::Verbose
        (
            TEXT("Updating AnimState Overlay from [{}] to [{}] on Entity [{}]"),
            PreviousAnimOverlay.Get_AnimOverlay(),
            NewAnimOverlay.Get_AnimOverlay(),
            InHandle
        );

        InComp._AnimOverlay = NewAnimOverlay;
        InComp.Set_ComponentsModified(InComp.Get_ComponentsModified() | ECk_AnimStateComponents::Overlay);

        UUtils_Signal_AnimState_OnOverlayChanged::Broadcast(
            InHandle,
            MakePayload(InHandle,
                FCk_Payload_AnimState_OnOverlayChanged{InHandle, PreviousAnimOverlay, NewAnimOverlay}));
    }

    auto
        FProcessor_AnimState_Replicate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<FTag_AnimState_Updated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AnimState_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AnimState_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_AnimState_Rep>& InComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_AnimState_Rep>(InHandle, [&](UCk_Fragment_AnimState_Rep* InRepComp)
        {
            InRepComp->Set_AnimGoal(InCurrent.Get_AnimGoal());
            InRepComp->Set_AnimState(InCurrent.Get_AnimState());
            InRepComp->Set_AnimCluster(InCurrent.Get_AnimCluster());
            InRepComp->Set_AnimOverlay(InCurrent.Get_AnimOverlay());

            InCurrent.Set_ComponentsModified(ECk_AnimStateComponents::None);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

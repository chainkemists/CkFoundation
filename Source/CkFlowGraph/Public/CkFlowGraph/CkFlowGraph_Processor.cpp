#include "CkFlowGraph_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Game/CkGame_Utils.h"

#include "CkFlowGraph/CkFlowGraph_Log.h"
#include "CkFlowGraph/Subsystem/CkFlowGraph_Subsystem.h"

#include "CkNet/CkNet_Utils.h"

#include "Flow/Public/FlowSubsystem.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace internal::flow
    {
        auto
            Get_FlowSubsystem(
                const UWorld* InWorld)
            -> UFlowSubsystem*
        {
            const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(InWorld);

            CK_ENSURE_IF_NOT(ck::IsValid(GameInstance),
                TEXT("Failed to retrieve valid Game Instance! Cannot retrieve Flow Subsystem"))
            { return {}; }

            return GameInstance->GetSubsystem<UFlowSubsystem>();
        }

        auto
            Get_FlowGraphSubsystem(
                const UWorld* InWorld)
            -> UCk_FlowGraph_Subsystem_UE*
        {
            return UCk_Utils_FlowGraph_Subsystem_UE::Get_Subsystem(InWorld);
        }
    }

    auto
        FProcessor_FlowGraph_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_FlowGraph_Params& InParams,
            FFragment_FlowGraph_Current& InCurrent,
            const FFragment_FlowGraph_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_FlowGraph_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_FlowGraph_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_FlowGraph_Params& InParams,
            FFragment_FlowGraph_Current& InCurrent,
            const FCk_Request_FlowGraph_Start& InRequest)
        -> void
    {
        if (InCurrent.Get_Status() == ECk_FlowGraph_Status::Running)
        { return; }

        const auto& RootFlow = InParams.Get_Params().Get_RootFlow();

        CK_ENSURE_IF_NOT(ck::IsValid(RootFlow),
            TEXT("Invalid RootFlow specified for Entity [{}]"), InHandle)
        { return; }

        const auto& WorldForEntity = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        auto* FlowSubsystem      = internal::flow::Get_FlowSubsystem(WorldForEntity);
        auto* FlowGraphSubsystem = internal::flow::Get_FlowGraphSubsystem(WorldForEntity);

        CK_ENSURE_IF_NOT(ck::IsValid(FlowSubsystem),
            TEXT("Invalid Flow Subsystem"))
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(FlowGraphSubsystem),
            TEXT("Invalid Flow Graph Subsystem"))
        { return; }

        const auto& FlowGraphInstance = FlowSubsystem->CreateRootFlow(WorldForEntity, RootFlow);

        CK_ENSURE_IF_NOT(ck::IsValid(FlowGraphSubsystem),
            TEXT("Failed to create valid Flow Graph Instance using Root Flow [{}]"), RootFlow)
        { return; }

        FlowGraphSubsystem->Request_AddFlowGraph(InHandle, FlowGraphInstance);

        InCurrent._Status = ECk_FlowGraph_Status::Running;

        FlowGraphInstance->StartFlow();
    }

    auto
        FProcessor_FlowGraph_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_FlowGraph_Params& InParams,
            FFragment_FlowGraph_Current& InCurrent,
            const FCk_Request_FlowGraph_Stop& InRequest)
        -> void
    {
        if (InCurrent.Get_Status() == ECk_FlowGraph_Status::NotRunning)
        { return; }

        const auto& WorldForEntity = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        auto* FlowSubsystem      = internal::flow::Get_FlowSubsystem(WorldForEntity);
        auto* FlowGraphSubsystem = internal::flow::Get_FlowGraphSubsystem(WorldForEntity);

        CK_ENSURE_IF_NOT(ck::IsValid(FlowSubsystem),
            TEXT("Invalid Flow Subsystem"))
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(FlowGraphSubsystem),
            TEXT("Invalid Flow Graph Subsystem"))
        { return; }

        const auto& RootFlow = InParams.Get_Params().Get_RootFlow();

        FlowSubsystem->FinishRootFlow(WorldForEntity, RootFlow, EFlowFinishPolicy::Abort);
        FlowGraphSubsystem->Request_RemoveFlowGraph(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
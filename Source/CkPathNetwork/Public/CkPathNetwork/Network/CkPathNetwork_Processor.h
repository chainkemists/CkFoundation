#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Building the graph is pure math (no world access) — safe on any tick.
    class CKPATHNETWORK_API FProcessor_PathNetwork_Setup : public ck_exp::TProcessor<
        FProcessor_PathNetwork_Setup,
        FCk_Handle_PathNetwork,
        ck::TReadOnly<FFragment_PathNetwork_Params>,
        ck::TReadWrite<FFragment_PathNetwork_Graph>,
        FTag_PathNetwork_NeedsBuild,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_PathNetwork_NeedsBuild;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Rebuild replaces the ribbon set and bumps the epoch; follower corridors notice the move and
    // replan (FProcessor_PathNetworkFollower_InvalidateOnRebuild).
    class CKPATHNETWORK_API FProcessor_PathNetwork_HandleRequests : public ck_exp::TProcessor<
        FProcessor_PathNetwork_HandleRequests,
        FCk_Handle_PathNetwork,
        ck::TReadWrite<FFragment_PathNetwork_Params>,
        ck::TReadWrite<FFragment_PathNetwork_Graph>,
        ck::TReadWrite<FFragment_PathNetwork_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_PathNetwork_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph,
            FFragment_PathNetwork_Requests& InRequests) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph,
            const FCk_Request_PathNetwork_Rebuild& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed network's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKPATHNETWORK_API FProcessor_PathNetwork_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_PathNetwork_CancelPendingRequests,
        FCk_Handle_PathNetwork,
        ck::TReadOnly<FFragment_PathNetwork_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Each FindRoute runs the full plan: overlay candidates -> A* over FRouteGraph -> navmesh
    // validation/reprice loop -> corridor compile (side-keeping offsets) -> signals. Requests past
    // the per-frame budget (_MaxRouteQueriesPerFrame) are re-queued verbatim and retried next tick.
    class CKPATHNETWORK_API FProcessor_PathNetworkFollower_HandleRequests : public ck_exp::TProcessor<
        FProcessor_PathNetworkFollower_HandleRequests,
        FCk_Handle_PathNetworkFollower,
        ck::TReadOnly<FFragment_PathNetworkFollower_Params>,
        ck::TReadWrite<FFragment_PathNetworkFollower_Corridor>,
        ck::TReadWrite<FFragment_PathNetworkFollower_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_PathNetworkFollower_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_PathNetworkFollower_Requests& InRequests) const -> void;

    private:
        // Every DoHandleRequest failure path (FailRoute) leaves OutResult untouched — the caller
        // primes it Failed before the call, mirroring MakeCompletionGuard's default-Failed contract.
        auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            const FCk_Request_PathNetworkFollower_FindRoute& InRequest,
            ECk_Request_OperationResult& OutResult) const -> void;

    private:
        mutable int32 _BudgetRemainingThisTick = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed follower's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKPATHNETWORK_API FProcessor_PathNetworkFollower_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_PathNetworkFollower_CancelPendingRequests,
        FCk_Handle_PathNetworkFollower,
        ck::TReadOnly<FFragment_PathNetworkFollower_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Re-issues the route request for the same goal when the network epoch moved under the corridor.
    class CKPATHNETWORK_API FProcessor_PathNetworkFollower_InvalidateOnRebuild : public ck_exp::TProcessor<
        FProcessor_PathNetworkFollower_InvalidateOnRebuild,
        FCk_Handle_PathNetworkFollower,
        ck::TReadOnly<FFragment_PathNetworkFollower_Params>,
        ck::TReadWrite<FFragment_PathNetworkFollower_Corridor>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

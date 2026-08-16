#pragma once

#include "CkResolverDataBundle_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverDataBundle_StartNewPhase : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_StartNewPhase,
        FCk_Handle_ResolverDataBundle,
        ck::TReadOnly<FFragment_ResolverDataBundle_Params>,
        ck::TReadWrite<FFragment_ResolverDataBundle_Current>,
        FTag_ResolverDataBundle_StartNewPhase,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_ResolverDataBundle_StartNewPhase;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ResolverDataBundle_Params& InParams,
            FFragment_ResolverDataBundle_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverDataBundle_HandleRequests : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_HandleRequests,
        FCk_Handle_ResolverDataBundle,
        ck::TReadWrite<FFragment_ResolverDataBundle_Current>,
        ck::TReadWrite<FFragment_ResolverDataBundle_Requests>,
        ck::TExclude<FTag_ResolverDataBundle_StartNewPhase>,
        ck::TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_ResolverDataBundle_StartNewPhase>;
        using MarkedDirtyBy = FFragment_ResolverDataBundle_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            FFragment_ResolverDataBundle_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            const FCk_Request_ResolverDataBundle_ModifierOperation& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            const FCk_Request_ResolverDataBundle_MetadataOperation& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed bundle's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKRESOLVER_API FProcessor_ResolverDataBundle_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_CancelPendingRequests,
        FCk_Handle_ResolverDataBundle,
        ck::TReadOnly<FFragment_ResolverDataBundle_Requests>,
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
            const FFragment_ResolverDataBundle_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverDataBundle_ResolveOperations : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_ResolveOperations,
        FCk_Handle_ResolverDataBundle,
        ck::TReadWrite<FFragment_ResolverDataBundle_Current>,
        ck::TReadOnly<FFragment_ResolverDataBundle_PendingOperations>,
        ck::TExclude<FTag_ResolverDataBundle_StartNewPhase>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_ResolverDataBundle_HandleRequests>;
        using TProcessor::TProcessor;

    public:
        using MarkedDirtyBy = FFragment_ResolverDataBundle_PendingOperations;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InCurrent,
            const FFragment_ResolverDataBundle_PendingOperations& InPendingOperations) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverDataBundle_Calculate : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_Calculate,
        FCk_Handle_ResolverDataBundle,
        ck::TReadOnly<FFragment_ResolverDataBundle_Params>,
        ck::TReadWrite<FFragment_ResolverDataBundle_Current>,
        FTag_ResolverDataBundle_NeedsCalculate,
        ck::TExclude<FFragment_ResolverDataBundle_Requests>,
        ck::TExclude<FFragment_ResolverDataBundle_PendingOperations>,
        ck::TExclude<FTag_ResolverDataBundle_CalculateDone>,
        ck::TExclude<FTag_ResolverDataBundle_StartNewPhase>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_ResolverDataBundle_ResolveOperations>;
        // Without this the whole phase cascade is gated to one phase per frame — see the tag's
        // declaration in CkResolverDataBundle_Fragment.h.
        using MarkedDirtyBy = FTag_ResolverDataBundle_NeedsCalculate;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ResolverDataBundle_Params& InParams,
            FFragment_ResolverDataBundle_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

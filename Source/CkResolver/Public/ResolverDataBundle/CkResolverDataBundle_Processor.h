#pragma once

#include "CkResolverDataBundle_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverDataBundle_StartNewPhase : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_StartNewPhase,
        FCk_Handle_ResolverDataBundle,
        FFragment_ResolverDataBundle_Params,
        FFragment_ResolverDataBundle_Current,
        FTag_ResolverDataBundle_StartNewPhase,
        CK_IGNORE_PENDING_KILL>
    {
    public:
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
        FFragment_ResolverDataBundle_Current,
        FFragment_ResolverDataBundle_Requests,
        ck::TExclude<FTag_ResolverDataBundle_StartNewPhase>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
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
            const FRequest_ResolverDataBundle_ModifierOperation& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ResolverDataBundle_Current& InComp,
            const FRequest_ResolverDataBundle_MetadataOperation& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverDataBundle_ResolveOperations : public ck_exp::TProcessor<
        FProcessor_ResolverDataBundle_ResolveOperations,
        FCk_Handle_ResolverDataBundle,
        FFragment_ResolverDataBundle_Current,
        FFragment_ResolverDataBundle_PendingOperations,
        ck::TExclude<FTag_ResolverDataBundle_StartNewPhase>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
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
        FFragment_ResolverDataBundle_Params,
        FFragment_ResolverDataBundle_Current,
        // we explicitly want to look for StartNewPhase because it's possible there were never any operations at all in a Phase
        FTag_ResolverDataBundle_StartNewPhase,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ResolverDataBundle_StartNewPhase;

    public:
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

#pragma once

#include "Ck2dGridBlocker_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKGRID_API FProcessor_2dGridBlocker_Setup : public ck_exp::TProcessor<
            FProcessor_2dGridBlocker_Setup,
            FCk_Handle_2dGridBlocker,
            TReadOnly<FFragment_2dGridBlocker_Params>,
            TReadWrite<FFragment_2dGridBlocker_Current>,
            FTag_2dGridBlocker_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_2dGridBlocker_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridBlocker_Params& InParams,
            FFragment_2dGridBlocker_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKGRID_API FProcessor_2dGridBlocker_Requests : public ck_exp::TProcessor<
            FProcessor_2dGridBlocker_Requests,
            FCk_Handle_2dGridBlocker,
            TReadOnly<FFragment_2dGridBlocker_Params>,
            TReadWrite<FFragment_2dGridBlocker_Current>,
            TReadWrite<FFragment_2dGridBlocker_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_2dGridBlocker_Setup>;
        using MarkedDirtyBy = FFragment_2dGridBlocker_Requests;

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
            const FFragment_2dGridBlocker_Params& InParams,
            FFragment_2dGridBlocker_Current& InCurrent,
            FFragment_2dGridBlocker_Requests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKGRID_API FProcessor_2dGridBlocker_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_2dGridBlocker_CancelPendingRequests,
        FCk_Handle_2dGridBlocker,
        ck::TReadOnly<FFragment_2dGridBlocker_Requests>,
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
            const FFragment_2dGridBlocker_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

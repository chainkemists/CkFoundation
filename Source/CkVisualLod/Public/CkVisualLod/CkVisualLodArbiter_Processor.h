#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVisualLod/CkVisualLodArbiter_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVISUALLOD_API FProcessor_VisualLodArbiter_Setup
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_Setup, FCk_Handle_VisualLodArbiter,
            ck::TReadOnly<FFragment_VisualLodArbiter_Params>, ck::TReadWrite<FFragment_VisualLodArbiter_Current>,
            FTag_VisualLodArbiter_NeedsSetup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_VisualLodArbiter_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISUALLOD_API FProcessor_VisualLodArbiter_HandleRequests
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_HandleRequests, FCk_Handle_VisualLodArbiter,
            ck::TReadWrite<FFragment_VisualLodArbiter_Current>, ck::TReadWrite<FFragment_VisualLodArbiter_Requests>,
            TExclude<FTag_VisualLodArbiter_NeedsSetup>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_VisualLodArbiter_Setup>;
        using MarkedDirtyBy = FFragment_VisualLodArbiter_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            FFragment_VisualLodArbiter_Requests& InRequests) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_SetObserver& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_ClearObserver& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The flip driver: resolves the view, walks the domain's members (per-entity flips + far
    // updates), then spends the promote budgets on the ranked best. Runs in the gameplay band so
    // member-transform writes land before FProcessor_IskmCrowd_Advance (FGroup_Transform_SyncFrom)
    class CKVISUALLOD_API FProcessor_VisualLodArbiter_Update
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_Update, FCk_Handle_VisualLodArbiter,
            ck::TReadOnly<FFragment_VisualLodArbiter_Params>, ck::TReadWrite<FFragment_VisualLodArbiter_Current>,
            TExclude<FTag_VisualLodArbiter_NeedsSetup>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_VisualLodArbiter_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISUALLOD_API FProcessor_VisualLodArbiter_CancelPendingRequests
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_CancelPendingRequests, FCk_Handle_VisualLodArbiter,
            ck::TReadOnly<FFragment_VisualLodArbiter_Requests>, CK_IF_END_PLAY>
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
            const FFragment_VisualLodArbiter_Requests& InRequests)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

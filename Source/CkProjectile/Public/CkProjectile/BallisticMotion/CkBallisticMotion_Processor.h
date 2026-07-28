#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkProjectile/BallisticMotion/CkBallisticMotion_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROJECTILE_API FProcessor_BallisticMotion_HandleRequests : public ck_exp::TProcessor<
            FProcessor_BallisticMotion_HandleRequests,
            FCk_Handle_BallisticMotion,
            ck::TReadOnly<FFragment_BallisticMotion_Params>,
            ck::TReadWrite<FFragment_BallisticMotion_Current>,
            ck::TReadOnly<FFragment_BallisticMotion_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FFragment_BallisticMotion_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FFragment_BallisticMotion_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FCk_Request_BallisticMotion_Launch& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FCk_Request_BallisticMotion_Stop& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed BallisticMotion's
    // still-queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKPROJECTILE_API FProcessor_BallisticMotion_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_BallisticMotion_CancelPendingRequests,
        FCk_Handle_BallisticMotion,
        ck::TReadOnly<FFragment_BallisticMotion_Requests>,
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
            const FFragment_BallisticMotion_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROJECTILE_API FProcessor_BallisticMotion_HandleImpacts : public ck_exp::TProcessor<
            FProcessor_BallisticMotion_HandleImpacts,
            FCk_Handle_BallisticMotion,
            FTag_BallisticMotion_Active,
            ck::TReadOnly<FFragment_BallisticMotion_Params>,
            ck::TReadWrite<FFragment_BallisticMotion_Current>,
            ck::TReadOnly<FFragment_Probe_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BallisticMotion_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FFragment_Probe_Current& InProbeCurrent) -> void;

    private:
        static auto
        DoHandleImpact(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent,
            const FCk_Probe_OverlapInfo& InOverlap) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROJECTILE_API FProcessor_BallisticMotion_UpdateTrajectory : public ck_exp::TProcessor<
            FProcessor_BallisticMotion_UpdateTrajectory,
            FCk_Handle_BallisticMotion,
            FTag_BallisticMotion_Active,
            ck::TReadOnly<FFragment_BallisticMotion_Params>,
            ck::TReadWrite<FFragment_BallisticMotion_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_BallisticMotion_HandleImpacts>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            FFragment_BallisticMotion_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

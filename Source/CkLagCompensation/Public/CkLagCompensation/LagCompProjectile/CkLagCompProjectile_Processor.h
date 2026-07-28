#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkLagCompensation/LagCompProjectile/CkLagCompProjectile_Fragment.h"

#include "CkProjectile/BallisticMotion/CkBallisticMotion_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Anchors the projectile's trajectory in the past (compensation window + fudge factor) and
    // sweeps the catch-up slice-by-slice against every RewindHistory entity. The validated
    // trajectory and the simulated one are the same closed-form function of the same inputs, so
    // validation and visuals cannot diverge — there is no ordering dependency between them
    class CKLAGCOMPENSATION_API FProcessor_LagCompProjectile_HandleRequests : public ck_exp::TProcessor<
            FProcessor_LagCompProjectile_HandleRequests,
            FCk_Handle_BallisticMotion,
            ck::TReadOnly<FFragment_BallisticMotion_Params>,
            ck::TReadOnly<FFragment_LagCompProjectile_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FFragment_LagCompProjectile_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            const FFragment_LagCompProjectile_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BallisticMotion_Params& InParams,
            const FCk_Request_LagCompProjectile_LaunchCompensated& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed entity's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKLAGCOMPENSATION_API FProcessor_LagCompProjectile_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_LagCompProjectile_CancelPendingRequests,
        FCk_Handle_BallisticMotion,
        ck::TReadOnly<FFragment_LagCompProjectile_Requests>,
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
            const FFragment_LagCompProjectile_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

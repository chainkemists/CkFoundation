#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment.h"
#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

#include "CkProjectile/Homing/CkHoming_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPROJECTILE_API FProcessor_Homing_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Homing_HandleRequests,
            FCk_Handle_Homing,
            ck::TReadOnly<FFragment_Homing_Params>,
            ck::TReadWrite<FFragment_Homing_Current>,
            ck::TReadOnly<FFragment_Homing_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FFragment_Homing_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_Homing& InHandle,
            const FFragment_Homing_Params& InParams,
            FFragment_Homing_Current& InCurrent,
            const FFragment_Homing_Requests& InRequestsComp) const -> void;

    private:
        // Only the SetTargetEntity overload has a genuine rejection path; all five overloads return
        // bool uniformly so the drain loop below (a single generic lambda over the request variant)
        // can thread the result out without special-casing per request type.
        static auto
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_SetTargetEntity& InRequest) -> bool;

        static auto
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_SetTargetLocation& InRequest) -> bool;

        static auto
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_ClearTarget& InRequest) -> bool;

        static auto
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_SetDesiredTimeToImpact& InRequest) -> bool;

        static auto
        DoHandleRequest(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent,
            const FCk_Request_Homing_EnableDisable& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed Homing's still-
    // queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKPROJECTILE_API FProcessor_Homing_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Homing_CancelPendingRequests,
        FCk_Handle_Homing,
        ck::TReadOnly<FFragment_Homing_Requests>,
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
            const FFragment_Homing_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    /**
     * Evaluates the ProNav guidance law each frame and writes (base + homing) acceleration for the
     * EulerIntegrator to consume the same frame (RunBefore). Steering is authority-only — clients
     * see the result through the existing Velocity replication
     */
    class CKPROJECTILE_API FProcessor_Homing_Update : public ck_exp::TProcessor<
            FProcessor_Homing_Update,
            FCk_Handle_Homing,
            FTag_Homing_Active,
            FTag_HasAuthority,
            ck::TReadOnly<FFragment_Homing_Params>,
            ck::TReadWrite<FFragment_Homing_Current>,
            ck::TReadOnly<FFragment_Velocity_Current>,
            ck::TReadWrite<FFragment_Acceleration_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_Homing_HandleRequests>;
        using RunBefore = TDepList<FProcessor_EulerIntegrator_Update>;
        using MarkedDirtyBy = FTag_Homing_Active;
        // Time-stepping producer: re-running with DeltaT=0 would re-derive guidance from unchanged
        // state for no benefit, and the desired-time-to-impact countdown must tick exactly once per frame
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_Homing& InHandle,
            const FFragment_Homing_Params& InParams,
            FFragment_Homing_Current& InCurrent,
            const FFragment_Velocity_Current& InVelocity,
            FFragment_Acceleration_Current& InAcceleration) const -> void;

    private:
        static auto
        DoDeactivate(
            FCk_Handle_Homing InHandle,
            FFragment_Homing_Current& InCurrent) -> void;

        static auto
        DoCheckForMissedTarget(
            FCk_Handle_Homing InHandle,
            const FFragment_Homing_Params& InParams,
            FFragment_Homing_Current& InCurrent,
            const FCk_Homing_GuidanceState& InState) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

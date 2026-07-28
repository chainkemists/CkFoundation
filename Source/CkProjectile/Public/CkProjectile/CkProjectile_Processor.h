#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"
#include "CkProjectile/CkProjectile_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Projectile_HandleRequests;

    class CKPROJECTILE_API FProcessor_Projectile_Update : public TProcessor<
            FProcessor_Projectile_Update,
            FTag_Projectile,
            ck::TReadOnly<FFragment_EulerIntegrator_Current>,
            FTag_EulerIntegrator_NeedsUpdate,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_Projectile_HandleRequests>;
        using MarkedDirtyBy = FTag_EulerIntegrator_NeedsUpdate;
        // Time-stepping consumer: re-running with DeltaT=0 would re-enqueue the same _DistanceOffset
        // and double per-frame movement. The integrator's NeedsUpdate tag is sticky (not consumed by us).
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EulerIntegrator_Current& InIntegratorComp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROJECTILE_API FProcessor_Projectile_HandleRequests : public TProcessor<
            FProcessor_Projectile_HandleRequests,
            ck::TReadOnly<FFragment_Projectile_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using MarkedDirtyBy = FFragment_Projectile_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Projectile_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_Projectile_Requests::RequestType& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed request entity's
    // still-pending request is never drained. This fires its completion delegate with Failed_Cancelled
    // so a caller awaiting completion terminates instead of hanging. The request is stored singular
    // (not a list/variant) here, so this calls TryFireCompletion directly rather than the
    // FireCancelledForPending container helper.
    class CKPROJECTILE_API FProcessor_Projectile_CancelPendingRequests : public TProcessor<
        FProcessor_Projectile_CancelPendingRequests,
        ck::TReadOnly<FFragment_Projectile_Requests>,
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
            const FFragment_Projectile_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

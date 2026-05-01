#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gate 2 — pattern replication of FProcessor_Projectile_Update.
    // Reads the post-integration FFragment_EulerIntegrator_Current::_DistanceOffset and enqueues a
    // Request_AddLocationOffset on the agent's Transform handle. The CkEcsExt Transform processor
    // (FGroup_Transform) drains the request next frame and updates the SceneNode.
    //
    // RunAfter = FProcessor_EulerIntegrator_Update so we observe the value the integrator just produced.
    // TExclude<FTag_CrowdAgent_Asleep> is forward-compatible (Gate 4): asleep agents skip the offset apply.
    class CKCROWD_API FProcessor_CrowdAgent_ApplyOffset : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_ApplyOffset,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_EulerIntegrator_Current>,
            FTag_EulerIntegrator_NeedsUpdate,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_EulerIntegrator_Update>;
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
            const FFragment_EulerIntegrator_Current& InIntegrator) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

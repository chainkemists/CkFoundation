#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment.h"

#include "CkProjectile/CkProjectile_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Pattern replication of FProcessor_Projectile_Update.
    // Reads the post-integration FFragment_EulerIntegrator_Current::_DistanceOffset and stages it
    // into FFragment_CrowdAgent_PendingDisplacement. It does NOT write the Transform — the single
    // Transform writer for a crowd agent is FProcessor_CrowdAgent_ConstrainToNavmesh, which walks
    // the accumulated displacement along the navmesh surface before applying it.
    //
    // RunAfter = FProcessor_EulerIntegrator_Update so we observe the value the integrator just produced.
    // TExclude<FTag_CrowdAgent_Asleep> is forward-compatible: asleep agents skip the offset apply.
    class CKCROWD_API FProcessor_CrowdAgent_ApplyOffset : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_ApplyOffset,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_EulerIntegrator_Current>,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            FTag_EulerIntegrator_NeedsUpdate,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        // Shares MarkedDirtyBy (FTag_EulerIntegrator_NeedsUpdate) with FProcessor_Projectile_Update;
        // the two act on disjoint entity sets (crowd agents vs projectiles), so ordering is
        // immaterial for correctness — declare it explicitly to silence the dirty-marker-conflict
        // advisory. EulerIntegrator_Update stays first (we observe the value it just produced).
        using RunAfter = TDepList<FProcessor_EulerIntegrator_Update, FProcessor_Projectile_Update>;
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
            const FFragment_EulerIntegrator_Current& InIntegrator,
            FFragment_CrowdAgent_PendingDisplacement& InPending) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

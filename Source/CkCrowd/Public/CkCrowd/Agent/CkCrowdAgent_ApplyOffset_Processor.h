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
    // Stages the post-integration _DistanceOffset into FFragment_CrowdAgent_PendingDisplacement.
    // Never writes the Transform — FProcessor_CrowdAgent_ConstrainToNavmesh is the single Transform
    // writer for a crowd agent.
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
        // Disjoint entity sets with FProcessor_Projectile_Update (agents vs projectiles) despite the
        // shared dirty marker; the explicit dep only silences the dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_EulerIntegrator_Update, FProcessor_Projectile_Update>;
        using MarkedDirtyBy = FTag_EulerIntegrator_NeedsUpdate;
        // The NeedsUpdate tag is sticky, so a DeltaT=0 pump would re-enqueue the same _DistanceOffset.
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

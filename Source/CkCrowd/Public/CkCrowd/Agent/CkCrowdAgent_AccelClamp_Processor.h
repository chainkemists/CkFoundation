#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Phase 1.2 — clamps the per-frame VELOCITY DELTA on FFragment_CrowdAgent_DesiredVelocity.
    // Mirrors DetourCrowd.cpp:integrate() 53-69. Critical because Steering writes a fresh
    // `Direction * TargetSpeed` each frame — direction can flip arbitrarily, which is the root
    // cause of the head-on vibration mode. Capping |dv| ≤ MaxAccel × dt forces direction changes
    // to ramp instead of snap.
    //
    // Group: FGroup_Physics. RunAfter Steering (and, in Phase 2, AvoidanceSample) so this processor
    // sees whichever solver wrote last. RunBefore VelocityBridge so the bridge ships the clamped
    // value into the physics layer.
    //
    // Disable for A/B comparison via UCk_Crowd_ProjectSettings_UE._AccelClampMode = Disabled.
    class CKCROWD_API FProcessor_CrowdAgent_AccelClamp : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_AccelClamp,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

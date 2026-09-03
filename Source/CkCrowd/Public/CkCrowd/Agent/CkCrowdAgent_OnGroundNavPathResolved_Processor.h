#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnPathResolved_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Identity of the last ground route installed into this agent's nav-path slot: goal pins which
    // MoveTo it answered, epoch pins which published field it was planned against. The GroundNav
    // result fragment persists after the episode ends, so without this a Ready answer would be
    // re-installed on every frame the agent still holds an active goal.
    struct CKCROWD_API FFragment_CrowdAgent_InstalledGroundNavPath
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_InstalledGroundNavPath);

        friend class FProcessor_CrowdAgent_OnGroundNavPathResolved;

    private:
        FVector _GoalLocation = FVector::ZeroVector;
        int64 _PlannedAgainstEpoch = 0;

    public:
        CK_PROPERTY_GET(_GoalLocation);
        CK_PROPERTY_GET(_PlannedAgainstEpoch);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The view deliberately does NOT require PathPending: a field rebuild replans for agents that are
    // already WALKING, and the fresh route must swap in mid-walk.
    //
    // PumpPolicy::SkipPump — the body broadcasts Nav_OnPathReady (via the install seam) and
    // Nav_OnPathFailed; pumping with DeltaT=0 would re-broadcast before the tag transition takes effect.
    class CKCROWD_API FProcessor_CrowdAgent_OnGroundNavPathResolved : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_OnGroundNavPathResolved,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_GroundNavPath_Result>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        // HandleRequests stamps PathPending + _ActiveGoal + parks the nav-path slot this processor
        // keys off, so a MoveTo issued this frame is observed with consistent state.
        // Every resolve processor installs into FFragment_CrowdAgent_PathFollow; the chain fixes which install wins.
        using RunAfter = TDepList<
            FProcessor_CrowdAgent_HandleRequests,
            FProcessor_CrowdAgent_OnPathResolved>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_GroundNavPath_Result& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnPathResolved_Processor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Reconciles the shared nav-path slot against the agent's actual movement state.
    //
    // The view is keyed on the SLOT rather than on FTag_CrowdAgent_PathPending, and that is the
    // point: a tag-keyed watchdog cannot see an episode that ended without releasing its query,
    // because ending it is exactly what removes the tag. Keying on the slot means this converges
    // from arbitrary state — including an orphan that a path we have not found still produces.
    //
    // Two failure shapes, deliberately handled differently:
    //   - Live episode, past the timeout -> the provider never answered. Write Failed so
    //     FProcessor_CrowdAgent_OnPathResolved performs the ONE transition and fires the single
    //     OnGoalFailed; duplicating that here would double-broadcast.
    //   - No live episode at all -> the orphan. Nothing is listening (the state machine unbinds
    //     OnGoalFailed when it leaves Locomotion), so this clears the slot and ensures instead of
    //     broadcasting into nobody.
    class CKCROWD_API FProcessor_CrowdAgent_PathPendingWatchdog : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PathPendingWatchdog,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>,
            TExclude<FTag_DestroyEntity_Initiate>>
    {
    public:
        using Group = FGroup_Gameplay;
        // OnPathResolved writes the same slot and owns the tag transition, so reconciling ahead of it
        // would judge a result that has not landed yet.
        using RunAfter = TDepList<FProcessor_CrowdAgent_OnPathResolved>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

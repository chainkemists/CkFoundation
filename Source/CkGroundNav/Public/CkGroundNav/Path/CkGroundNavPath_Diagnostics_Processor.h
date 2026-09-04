#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"
#include "CkGroundNav/Path/CkGroundNavPath_Fragment_Data.h"
#include "CkGroundNav/Path/CkGroundNavPath_Processor.h"

// --------------------------------------------------------------------------------------------------------------------
// Scheduling note. This reads what the slice PUBLISHED, so RunAfter names it in the group they share:
// a pass ordered before it would report every plan a tick late and would date it a tick early, which
// is the one thing a dated column must not do.
//
// Nothing here plans, searches, publishes or clears. It copies, and the fragments it copies from are
// taken read-only so that stays true.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * Copies this agent's planner state onto FFragment_GroundNavPath_Diagnostics, once a tick.
     *
     * The fragment is composed on FIRST VISIT rather than by the feature's Add, so one that exists is
     * one a pass actually wrote and a reader never has to tell a stamped default from an unstamped
     * one. Nothing removes it, which is what Current and Result do: the path feature has no teardown
     * of its own, and its fragments go when the entity does.
     *
     * The date it writes is a PLAN's and not a tick's, and the published sequence is what tells the
     * two apart. The pass re-dates only when the slot's sequence has moved past the one it last dated,
     * so a plan left standing for the next hundred ticks keeps the date it was planned at rather than
     * the date it was last looked at. Two plans that publish between two passes move the sequence
     * twice and are dated once, at the tick the pass saw them - which is the later plan's date, and
     * the later plan is the one the slot is carrying.
     *
     * DoTick reads the gate once for the whole pass rather than once per agent, and stamping nobody is
     * all a closed gate does: what was stamped before it closed stays exactly as it was stamped.
     */
    class CKGROUNDNAV_API FProcessor_GroundNavPath_Diagnostics : public ck_exp::TProcessor<
        FProcessor_GroundNavPath_Diagnostics,
        FCk_Handle_GroundNavPath,
        ck::TReadOnly<FFragment_GroundNavPath_Current>,
        ck::TReadOnly<FFragment_GroundNavPath_Result>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_GroundNavPath_Slice>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType                               InDeltaT,
            HandleType                             InPathEntity,
            const FFragment_GroundNavPath_Current& InCurrent,
            const FFragment_GroundNavPath_Result&  InResult) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

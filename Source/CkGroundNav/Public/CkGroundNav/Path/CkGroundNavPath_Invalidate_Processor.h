#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

#include "CkNavigation/NavSurface/CkNavSurface_Processor.h"

// --------------------------------------------------------------------------------------------------------------------
// Scheduling note. The queue this reads is EMPTIED by FProcessor_NavSurface_RevisionWatch, which
// broadcasts a publish and then drops it, so the ordering below is not a preference: run after the
// watch and there is nothing left to compare a corridor against. RunBefore states that once, in the
// same group the watch sits in, rather than leaving it to whichever order the two happened to
// register in.
//
// Nothing here plans, and nothing here consumes: the flag is raised for the path's own consumer and
// this module never removes it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * Raises FTag_GroundNavPath_RepathRequired on every agent whose cached corridor a published
     * surface rebuild could have moved.
     *
     * The corridor box is already inflated by the body's radius plus the margin the plan stored it
     * with, so the test here is a plain box intersection and deliberately exact: FBox::Intersect is
     * CLOSED, and a rebuild whose face lands exactly on the corridor's is a rebuild that reached it.
     * A queued box that is INVALID is a publisher that did not know where it rebuilt, which rules
     * nothing out and therefore flags everything.
     *
     * Two things stop a burst from being answered twice. The tag is idempotent, so a publish carrying
     * a dozen overlapping boxes still leaves one flag; and a corridor found on the epoch the world is
     * publishing NOW already postdates every rebuild the queue can describe, so it is skipped whole.
     */
    class CKGROUNDNAV_API FProcessor_GroundNavPath_InvalidateOnRebuilt : public ck_exp::TProcessor<
        FProcessor_GroundNavPath_InvalidateOnRebuilt,
        FCk_Handle_GroundNavPath,
        ck::TReadOnly<FFragment_GroundNavPath_Current>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunBefore = TDepList<FProcessor_NavSurface_RevisionWatch>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Current& InCurrent) const -> void;

    private:
        // The world entity's queue, BORROWED for the length of one pass and never copied - a copy would
        // be a second account of what was published, and the drain that empties the first runs after
        // this. Null outside a pass, and on every pass that found nothing to answer.
        const TArray<FBox>* _PublishedRebuilds = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
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
     *
     * BOUNDS ARE THE FLOOR, LINK IDENTITY NARROWS ONE CASE. The registry entry carries a note naming
     * the epoch of the last publish that could have moved ground and the authored link ids every
      * link-only publish since it moved; a corridor caches the ids it crosses. A corridor planned at or
      * after that geometry publish checks only link ids changed after its own epoch and whether its
      * saved filter now denies a used plate. A corridor older than geometry falls to bounds, as does a
      * world with no field to read a note from.
     *
     * A SEARCH IN FLIGHT IS ANSWERED BY ITS REQUEST, NOT BY ITS CORRIDOR. A sliced search pins the
     * field it reads at Request_Begin and holds it for the whole episode, so a rebuild published
     * mid-search moves ground the eventual route will not have seen - and there is no corridor yet
     * to intersect. That agent is measured against the box its request's two ends span, grown by the
     * same inflation a published corridor's box carries, and the answer is PARKED on the result slot
     * rather than raised as a repath: a search has to finish before it can be told to start again.
     * The success publish spends it (CkGroundNavPath_Processor.cpp, DoPublish_Success).
     *
     * THIS ARM FIRES AT MOST ONCE PER AGENT LIFETIME. It is reachable only while the path holds no
     * corridor, and a corridor, once published, never goes invalid again - so every rebuild an agent
     * meets after its first successful plan is answered by the corridor half above instead.
     */
    class CKGROUNDNAV_API FProcessor_GroundNavPath_InvalidateOnRebuilt : public ck_exp::TProcessor<
        FProcessor_GroundNavPath_InvalidateOnRebuilt,
        FCk_Handle_GroundNavPath,
        ck::TReadOnly<FFragment_GroundNavPath_Params>,
        ck::TReadOnly<FFragment_GroundNavPath_Current>,
        ck::TReadWrite<FFragment_GroundNavPath_Result>,
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
            const FFragment_GroundNavPath_Params& InParams,
            const FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult) const -> void;

    private:
        // Flags only when the corridor crosses a link changed after its plan.
        auto
        DoTry_FlagOnChangedLink(
            HandleType                                                InPathEntity,
            const FFragment_GroundNavPath_Current&                    InCurrent,
            const groundnav::world_fields::FCk_GroundNav_PublishNote& InNote) const -> void;

        auto
        DoTry_FlagOnDeniedPlate(
            HandleType                             InPathEntity,
            const FFragment_GroundNavPath_Current& InCurrent,
            const groundnav::FCk_GroundNav_FieldPtr& InField) const -> void;

        // The corridor-less half: an episode whose search has BEGUN is measured against its request's
        // own bounds, and what it finds is parked on the slot for the publish to spend.
        auto
        DoTry_ArmInFlightSearch(
            HandleType                             InPathEntity,
            const FFragment_GroundNavPath_Params&  InParams,
            const FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result&        InResult) const -> void;

    private:
        // The world entity's queue, BORROWED for the length of one pass and never copied - a copy would
        // be a second account of what was published, and the drain that empties the first runs after
        // this. Null outside a pass, and on every pass that found nothing to answer.
        const TArray<FBox>* _PublishedRebuilds = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------

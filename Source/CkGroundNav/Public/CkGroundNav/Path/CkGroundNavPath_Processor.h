#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGroundNav/Path/CkGroundNavPath_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Scheduling note. Nothing here touches Jolt: a search reads only the immutable field the volume
// already published, so it needs no window around the physics step and sits in the ordinary gameplay
// group with every other request drain.
//
// The drain and the slice are SEPARATE processors, and deliberately unlike CkVoxelNav, where one
// synchronous search runs to completion inside the drain. A ground search is sliced across frames, so
// the work has to happen on ticks where no request arrived - which a MarkedDirtyBy processor is never
// scheduled for. The slice below therefore declares no MarkedDirtyBy and is gated on the in-flight
// tag instead, exactly as FProcessor_GroundNavVolume_Build is: the scheduler then runs it once per
// main tick and never pumps it, which is what makes a per-tick budget mean anything.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * The episode transitions the two processors below share: begin, publish, clear.
     *
     * A struct rather than free functions only because the fragments keep their state private and a
     * namespace cannot be befriended. Nothing outside this pair has any business calling these - an
     * episode published from anywhere else would fire a completion the drain is still holding.
     */
    struct CKGROUNDNAV_API FGroundNavPath_Episode
    {
    public:
        /** Stands a search up over whichever field covers the start, or leaves the episode parked when
         *  there is no field or the field has not baked that ground. */
        static auto
        DoTry_Begin(
            FCk_Handle_GroundNavPath InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent) -> void;

        // Routes a terminal search onto the success or the failure publish.
        static auto
        DoPublish_Terminal(
            FCk_Handle_GroundNavPath InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult) -> void;

        static auto
        DoPublish_Success(
            FCk_Handle_GroundNavPath InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult) -> void;

        // Also the deferral timeout's exit, which has no search to read a count or an epoch off.
        static auto
        DoPublish_Failure(
            FCk_Handle_GroundNavPath InPathEntity,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            ECk_GroundNav_PathStatus InStatus,
            int32 InExpansionCount,
            int64 InPlannedAgainstEpoch) -> void;

        static auto
        DoClear(
            FFragment_GroundNavPath_Current& InCurrent) -> void;

        // The one line that proves the provider answered, on every publish path.
        static auto
        DoLog_Published(
            FCk_Handle_GroundNavPath InPathEntity,
            const FCk_GroundNavPath_Result& InPublished) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Drains FindPath and AbandonPath, and stands the search up.
     *
     * A FindPath that resolves no field, or whose first slice answers Unbuilt, is PARKED rather than
     * failed: ground nobody has baked yet is worth waiting for, and the slice below re-probes it.
     */
    class CKGROUNDNAV_API FProcessor_GroundNavPath_HandleRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavPath_HandleRequests,
        FCk_Handle_GroundNavPath,
        ck::TReadOnly<FFragment_GroundNavPath_Params>,
        ck::TReadWrite<FFragment_GroundNavPath_Current>,
        ck::TReadWrite<FFragment_GroundNavPath_Result>,
        ck::TReadWrite<FFragment_GroundNavPath_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FFragment_GroundNavPath_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            FFragment_GroundNavPath_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            const FCk_Request_GroundNavPath_FindPath& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            const FCk_Request_GroundNavPath_AbandonPath& InRequest) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One budgeted slice of searching per tick, spread over every agent with an episode in flight.
     *
     * CkGroundNav owns this budget and actually spends it - both ceilings are read on every entity and
     * an exhausted one stops the pass rather than being reset and ignored. The searches-per-frame cap
     * bounds how many agents may be touched at all; the time slice bounds what each of them may spend,
     * and the remainder of it is what the next agent is handed.
     */
    class CKGROUNDNAV_API FProcessor_GroundNavPath_Slice : public ck_exp::TProcessor<
        FProcessor_GroundNavPath_Slice,
        FCk_Handle_GroundNavPath,
        ck::TReadOnly<FFragment_GroundNavPath_Params>,
        ck::TReadWrite<FFragment_GroundNavPath_Current>,
        ck::TReadWrite<FFragment_GroundNavPath_Result>,
        FTag_GroundNavPath_SearchInFlight,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_GroundNavPath_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult) const -> void;

    private:
        mutable int32 _SearchesRemainingThisTick = 0;

        mutable FCk_Time _SliceRemainingThisTick;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Both processors above exclude owners already tagged for destruction, so a destroyed agent's
    // queued requests are never drained AND its in-flight episode never reports. This fires both as
    // Failed_Cancelled, so a caller awaiting completion terminates instead of hanging on a delegate
    // that no longer has a processor to fire it.
    class CKGROUNDNAV_API FProcessor_GroundNavPath_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavPath_CancelPendingRequests,
        FCk_Handle_GroundNavPath,
        ck::TReadWrite<FFragment_GroundNavPath_Current>,
        ck::TReadOnly<FFragment_GroundNavPath_Requests>,
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
            HandleType InPathEntity,
            FFragment_GroundNavPath_Current& InCurrent,
            const FFragment_GroundNavPath_Requests& InRequests) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

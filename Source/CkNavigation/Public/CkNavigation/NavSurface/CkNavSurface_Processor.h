#pragma once

#include "CkNavigation/NavSurface/CkNavSurface_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNAVIGATION_API FProcessor_NavSurfaceMarkup_HandleRequests : public ck_exp::TProcessor<
        FProcessor_NavSurfaceMarkup_HandleRequests,
        FCk_Handle_NavSurfaceMarkup,
        ck::TReadWrite<FFragment_NavSurfaceMarkup_Requests>,
        ck::TReadWrite<FFragment_NavSurfaceMarkup_Current>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_NavSurfaceMarkup_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Requests& InRequests,
            FFragment_NavSurfaceMarkup_Current& InCurrent) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_NavSurface_AreaMarkup& InRequest) -> ECk_Request_OperationResult;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKNAVIGATION_API FProcessor_NavSurfaceMarkup_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_NavSurfaceMarkup_CancelPendingRequests,
        FCk_Handle_NavSurfaceMarkup,
        ck::TReadOnly<FFragment_NavSurfaceMarkup_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_NavSurfaceMarkup_Requests& InRequests) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKNAVIGATION_API FProcessor_NavSurfaceMarkup_EndPlay : public ck_exp::TProcessor<
        FProcessor_NavSurfaceMarkup_EndPlay,
        FCk_Handle_NavSurfaceMarkup,
        ck::TReadWrite<FFragment_NavSurfaceMarkup_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurfaceMarkup_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    /**
     * The neutral link-traversal handshake, drained on the game thread.
     *
     * No provider is consulted: crossing a link changes no cell, no plate and no clearance, so the
     * whole feature is consumer-observable state on the entity doing the crossing.
     *
     * One link at a time. A Begin naming the correlator already running is the caller's intent
     * already holding, so it Succeeds silently; a Begin naming a different one while a crossing is
     * live is refused rather than silently replacing it, because the crossing that was dropped would
     * never report an end.
     */
    class CKNAVIGATION_API FProcessor_NavSurface_LinkTraversal_HandleRequests : public ck_exp::TProcessor<
        FProcessor_NavSurface_LinkTraversal_HandleRequests,
        FCk_Handle,
        ck::TReadWrite<FFragment_NavSurface_LinkTraversal_Requests>,
        ck::TReadWrite<FFragment_NavSurface_LinkTraversal_Current>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_NavSurface_LinkTraversal_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Requests& InRequests,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent) const -> void;

        // Clears the crossing and reports it ended with InResult. Public because teardown ends a live
        // crossing the same way a cancel does, and two copies of "what ending means" would drift.
        static auto DoEnd_Traversal(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            ECk_Request_OperationResult InResult) -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            const FCk_Request_NavSurface_BeginLinkTraversal& InRequest) -> ECk_Request_OperationResult;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            const FCk_Request_NavSurface_CompleteLinkTraversal& InRequest) -> ECk_Request_OperationResult;

        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent,
            const FCk_Request_NavSurface_CancelLinkTraversal& InRequest) -> ECk_Request_OperationResult;
    };

    // --------------------------------------------------------------------------------------------------------------------

    /**
     * Teardown for both halves of the handshake at once. The queue is cancelled where it stands, and a
     * crossing that was still live reports its end as Failed_Cancelled - a listener waiting for the
     * body to come off the ladder is owed an answer even when the answer is that it never did.
     *
     * The view keys on _Current rather than on the queue: the drain takes the requests fragment OFF
     * the entity (CopyAndRemove), so a mid-crossing traverser normally carries no queue at all.
     */
    class CKNAVIGATION_API FProcessor_NavSurface_LinkTraversal_EndPlay : public ck_exp::TProcessor<
        FProcessor_NavSurface_LinkTraversal_EndPlay,
        FCk_Handle,
        ck::TReadWrite<FFragment_NavSurface_LinkTraversal_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_LinkTraversal_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    /**
     * Delivers the neutral OnSurfaceRebuilt signal, and keeps the world's provider fragment's
     * health reading current.
     *
     * Two sources feed it. A provider that knows WHERE it rebuilt pushes each region through
     * Request_NotifySurfaceRebuilt and this drains the queue one broadcast per push, in order. A
     * provider that only advances a revision counter — Recast — is polled, and gets one broadcast
     * carrying an invalid box. The poll runs only on a tick that drained nothing, so a push is
     * never re-reported as an anonymous revision move.
     *
     * The group is pinned because the delay between a publish and the reaction to it is a
     * contract, not an accident: publishes land in FGroup_Transform (CkGroundNav's volume build
     * and republish), and FGroup_Gameplay_TimeDelta opens the next frame ahead of FGroup_Gameplay,
     * where CkCrowd's path refresh consumes the signal. One frame, every time, in that order.
     */
    class CKNAVIGATION_API FProcessor_NavSurface_RevisionWatch : public ck_exp::TProcessor<
        FProcessor_NavSurface_RevisionWatch,
        FCk_Handle,
        ck::TReadWrite<FFragment_NavSurface_Provider>,
        ck::TReadWrite<FFragment_NavSurface_RevisionWatch>,
        ck::TReadWrite<FFragment_NavSurface_PendingRebuilds>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NavSurface_Provider& InProvider,
            FFragment_NavSurface_RevisionWatch& InWatch,
            FFragment_NavSurface_PendingRebuilds& InPending) -> void;

        // Empties the pushed queue, one broadcast per region in publish order. True when it
        // broadcast anything, which is what tells the caller to skip the revision poll.
        static auto DoBroadcast_PendingRebuilds(
            HandleType InHandle,
            FFragment_NavSurface_RevisionWatch& InWatch,
            FFragment_NavSurface_PendingRebuilds& InPending,
            int64 InRevision) -> bool;

        // One broadcast with an invalid box when the provider's counter moved past the last one
        // reported, and nothing otherwise.
        static auto DoBroadcast_RevisionPoll(
            HandleType InHandle,
            FFragment_NavSurface_RevisionWatch& InWatch,
            int64 InRevision) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

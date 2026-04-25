#pragma once

#include "CkNav_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class ARecastNavMesh;
class dtCrowd;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------
    // FProcessor_Nav_HandleRequests
    //
    // Drains FFragment_Nav_Requests on each nav agent, runs FindPathSync per request, writes
    // FFragment_Nav_PathResult, broadcasts OnNavPathReady / OnNavPathFailed.
    //
    // Per-frame budget reset in DoTick (P3-2). MaxPathQueriesPerFrame from project settings;
    // 0 = disabled-with-warn. Tests stay queued (FTag_Nav_PathPending stays set) when budget
    // runs out and resume next frame.
    //
    // ForEachEntity is non-const because the budget counter is mutated during draining.
    // ----------------------------------------------------------------------------------------------------------------
    class CKNAVIGATION_API FProcessor_Nav_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Nav_HandleRequests,
            FCk_Handle_NavAgent,
            ck::TReadOnly<FFragment_Nav_AgentParams>,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadWrite<FFragment_Nav_Requests>,
            ck::TReadWrite<FFragment_Nav_PathResult>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_PostTransform;
        using MarkedDirtyBy = FFragment_Nav_Requests;

    public:
        FProcessor_Nav_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakPtr<dtCrowd>& InCrowdWeak,
            const TWeakObjectPtr<ARecastNavMesh>& InNavMeshWeak);

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_AgentParams& InParams,
            const FFragment_Transform& InTransform,
            FFragment_Nav_Requests& InRequests,
            FFragment_Nav_PathResult& InPathResult) -> void;

    private:
        // P3-2 budget. Reset to project-settings value at the top of each frame in DoTick.
        // Decremented in ForEachEntity per query consumed. When 0 (and budget > 0 globally),
        // remaining agents skip this frame; their FTag_Nav_PathPending stays set so they
        // don't double-queue. When budget == 0 globally, treat as DISABLED (warn-once at init).
        int32 _RemainingQueriesThisFrame = 0;

        // Captured for uniform factory signature; HandleRequests doesn't use either today.
        // Kept so all CkNav processors register through the same CK_NAV_FACTORY macro.
        TWeakPtr<dtCrowd>              _CrowdWeak;
        TWeakObjectPtr<ARecastNavMesh> _NavMeshWeak;
    };
}

// --------------------------------------------------------------------------------------------------------------------

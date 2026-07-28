#pragma once

#include "CkGeometryCollection_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCHAOS_API FProcessor_GeometryCollection_HandleRequests : public ck_exp::TProcessor<
            FProcessor_GeometryCollection_HandleRequests,
            FCk_Handle_GeometryCollection,
            TReadOnly<FFragment_GeometryCollection_Params>,
            TReadOnly<FFragment_GeometryCollection_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Chaos;
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FFragment_GeometryCollection_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FCk_Request_GeometryCollection_ApplyRadialStrain& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed GeometryCollection's
    // still-queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKCHAOS_API FProcessor_GeometryCollection_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_GeometryCollection_CancelPendingRequests,
        FCk_Handle_GeometryCollection,
        TReadOnly<FFragment_GeometryCollection_Requests>,
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
            HandleType InHandle,
            const FFragment_GeometryCollection_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollection_CrumbleNonActiveClusters : public ck_exp::TProcessor<
            FProcessor_GeometryCollection_CrumbleNonActiveClusters,
            FCk_Handle_GeometryCollection,
            TReadOnly<FFragment_GeometryCollection_Params>,
            FTag_GeometryCollection_CrumbleNonAnchoredClusters,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Chaos;
        using RunAfter = TDepList<FProcessor_GeometryCollection_HandleRequests>;
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollection_RemoveAllAnchors : public ck_exp::TProcessor<
            FProcessor_GeometryCollection_RemoveAllAnchors,
            FCk_Handle_GeometryCollection,
            TReadOnly<FFragment_GeometryCollection_Params>,
            FTag_GeometryCollection_RemoveAllAnchors,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Chaos;
        // Both Crumble and RemoveAllAnchors share MarkedDirtyBy + RunAfter HandleRequests, leaving them
        // unordered vs each other (disjoint tags, both TReadOnly Params — order is immaterial). Add the
        // sibling to impose HandleRequests -> Crumble -> RemoveAllAnchors and silence the advisory.
        using RunAfter = TDepList<FProcessor_GeometryCollection_HandleRequests, FProcessor_GeometryCollection_CrumbleNonActiveClusters>;
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

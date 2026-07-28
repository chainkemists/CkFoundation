#pragma once

#include "CkResourceLoader_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRESOURCELOADER_API FProcessor_ResourceLoader_HandleRequests : public TProcessor<
            FProcessor_ResourceLoader_HandleRequests,
            ck::TReadWrite<FFragment_ResourceLoader_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_ResourceLoader_Requests;

    public:
        using ThisType = FProcessor_ResourceLoader_HandleRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResourceLoader_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            FCk_Handle InHandle,
            const FCk_Request_ResourceLoader_LoadObject& InRequest) const -> bool;

        auto DoHandleRequest(
            FCk_Handle InHandle,
            const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest) const -> bool;

    public:
        auto DoOnPendingObjectStreamed(
            HandleType InHandle,
            FCk_ResourceLoader_ObjectReference_Soft InObjectStreamed) const -> void;

        auto DoOnPendingObjectBatchStreamed(
            HandleType InHandle,
            TArray<FCk_ResourceLoader_ObjectReference_Soft> InObjectBatchStreamed) const -> void;

        auto DoOnObjectLoaded(
            HandleType InHandle,
            FCk_ResourceLoader_LoadedObject InObjectLoaded) const -> void;

        auto DoOnObjectBatchLoaded(
            HandleType InHandle,
            FCk_ResourceLoader_LoadedObjectBatch InObjectBatchLoaded) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed request entity's still-
    // queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKRESOURCELOADER_API FProcessor_ResourceLoader_CancelPendingRequests : public TProcessor<
        FProcessor_ResourceLoader_CancelPendingRequests,
        ck::TReadOnly<FFragment_ResourceLoader_Requests>,
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
            const FFragment_ResourceLoader_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

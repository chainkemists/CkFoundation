#pragma once

#include "CkResourceLoader_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRESOURCELOADER_API FProcessor_ResourceLoader_HandleRequests : public TProcessor<
            FProcessor_ResourceLoader_HandleRequests,
            FFragment_ResourceLoader_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ResourceLoader_Requests;

    public:
        using ThisType = FProcessor_ResourceLoader_HandleRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResourceLoader_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            FCk_Handle InHandle,
            const FCk_Request_ResourceLoader_LoadObject& InRequest) const -> void;

        auto DoHandleRequest(
            FCk_Handle InHandle,
            const FCk_Request_ResourceLoader_LoadObjectBatch& InRequest) const -> void;

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
}

// --------------------------------------------------------------------------------------------------------------------

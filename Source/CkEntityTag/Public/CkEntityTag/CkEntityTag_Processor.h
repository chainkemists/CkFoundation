#pragma once

#include "CkEntityTag_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYTAG_API FProcessor_EntityTag_HandleRequests : public ck_exp::TProcessor<
            FProcessor_EntityTag_HandleRequests,
            FCk_Handle,
            FFragment_EntityTag_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityTag_Requests;
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityTag_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_Add& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FCk_Request_EntityTag_TryRemove& InRequest) -> void;
    };
}

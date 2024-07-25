#pragma once

#include "CkResolverTarget_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRESOLVER_API FProcessor_ResolverTarget_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ResolverTarget_HandleRequests,
            FCk_Handle_ResolverTarget,
            FFragment_ResolverTarget_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ResolverTarget_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ResolverTarget_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ResolverTarget_InitiateNewResolution& InNewResolution) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

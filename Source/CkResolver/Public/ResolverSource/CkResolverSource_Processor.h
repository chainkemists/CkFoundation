#pragma once

#include "CkResolverSource_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRESOLVER_API FProcessor_ResolverSource_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ResolverSource_HandleRequests,
            FCk_Handle_ResolverSource,
            FFragment_ResolverSource_Params,
            FFragment_ResolverSource_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ResolverSource_Requests;

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
            const ck::FFragment_ResolverSource_Params& InParams,
            FFragment_ResolverSource_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const ck::FFragment_ResolverSource_Params& InParams,
            const FCk_Request_ResolverSource_InitiateNewResolution& InNewResolution) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

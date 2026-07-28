#pragma once

#include "CkVatProxy_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVAT_API FProcessor_VatProxy_Setup : public ck_exp::TProcessor<
            FProcessor_VatProxy_Setup,
            FCk_Handle_VatProxy,
            TReadOnly<FFragment_VatProxy_Params>,
            TReadWrite<FFragment_VatProxy_Current>,
            FTag_VatProxy_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FTag_VatProxy_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVAT_API FProcessor_VatProxy_HandleRequests : public ck_exp::TProcessor<
            FProcessor_VatProxy_HandleRequests,
            FCk_Handle_VatProxy,
            TReadOnly<FFragment_VatProxy_Params>,
            TReadWrite<FFragment_VatProxy_Current>,
            TReadWrite<FFragment_VatProxy_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FFragment_VatProxy_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            FFragment_VatProxy_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_PlayClip& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_Stop& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_SetPlayRate& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed VatProxy's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKVAT_API FProcessor_VatProxy_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_VatProxy_CancelPendingRequests,
            FCk_Handle_VatProxy,
            TReadOnly<FFragment_VatProxy_Requests>,
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
            const FFragment_VatProxy_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // CPU mirror of the GPU playback clock for Once clips: absolute world time, no dt accumulation,
    // so re-execution within a tick is idempotent.
    class CKVAT_API FProcessor_VatProxy_FireSignals : public ck_exp::TProcessor<
            FProcessor_VatProxy_FireSignals,
            FCk_Handle_VatProxy,
            TReadOnly<FFragment_VatProxy_Params>,
            TReadWrite<FFragment_VatProxy_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

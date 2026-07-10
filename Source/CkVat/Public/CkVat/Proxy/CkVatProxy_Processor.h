#pragma once

#include "CkVatProxy_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Validates the collection and resolves the initial clip into playback state. The rendering hookup
    // (IsmProxy composition + look MID + per-instance custom data) lands in Gate 3 — until then a Vat
    // entity carries playback state without a visual.
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
            const FCk_Request_VatProxy_PlayClip& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_SetPlayRate& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // CPU mirror of the GPU playback clock for play-once clips: fires OnClipFinished exactly once when
    // the clip-local time passes the baked play length. Uses absolute world time (no dt accumulation),
    // so re-execution within a tick is idempotent. Reverse (negative-rate) once-completion is not
    // detected — recorded follow-up.
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

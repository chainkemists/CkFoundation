#pragma once

#include "CkVat_Fragment.h"

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
    class CKVAT_API FProcessor_Vat_Setup : public ck_exp::TProcessor<
            FProcessor_Vat_Setup,
            FCk_Handle_Vat,
            TReadOnly<FFragment_Vat_Params>,
            TReadWrite<FFragment_Vat_Current>,
            FTag_Vat_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FTag_Vat_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVAT_API FProcessor_Vat_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Vat_HandleRequests,
            FCk_Handle_Vat,
            TReadOnly<FFragment_Vat_Params>,
            TReadWrite<FFragment_Vat_Current>,
            TReadWrite<FFragment_Vat_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;
        using MarkedDirtyBy = FFragment_Vat_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            FFragment_Vat_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            const FCk_Request_Vat_PlayClip& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            const FCk_Request_Vat_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            const FCk_Request_Vat_SetPlayRate& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkAudio_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKAUDIO_API FProcessor_Audio_Setup : public ck_exp::TProcessor<
            FProcessor_Audio_Setup,
            FCk_Handle_Audio,
            FFragment_Audio_Params,
            FFragment_Audio_Current,
            FTag_Audio_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Audio_RequiresSetup;

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
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_Audio_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Audio_HandleRequests,
            FCk_Handle_Audio,
            FFragment_Audio_Params,
            FFragment_Audio_Current,
            FFragment_Audio_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Audio_Requests;

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
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent,
            FFragment_Audio_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent,
            const FCk_Request_Audio_ExampleRequest& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_Audio_Teardown : public ck_exp::TProcessor<
            FProcessor_Audio_Teardown,
            FCk_Handle_Audio,
            FFragment_Audio_Params,
            FFragment_Audio_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Audio_Params& InParams,
            FFragment_Audio_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAUDIO_API FProcessor_Audio_Replicate : public ck_exp::TProcessor<
            FProcessor_Audio_Replicate,
            FCk_Handle_Audio,
            FFragment_Audio_Current,
            TObjectPtr<UCk_Fragment_Audio_Rep>,
            FTag_Audio_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Audio_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Audio_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Audio_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
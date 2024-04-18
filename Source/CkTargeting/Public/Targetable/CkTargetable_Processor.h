#pragma once

#include "CkTargetable_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTARGETING_API FProcessor_Targetable_Setup : public ck_exp::TProcessor<
            FProcessor_Targetable_Setup,
            FCk_Handle_Targetable,
            FFragment_Targetable_Params,
            FFragment_Targetable_Current,
            FTag_Targetable_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Targetable_NeedsSetup;

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
            HandleType& InHandle,
            const FFragment_Targetable_Params& InParams,
            FFragment_Targetable_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTARGETING_API FProcessor_Targetable_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Targetable_HandleRequests,
            FCk_Handle_Targetable,
            FFragment_Targetable_Current,
            FFragment_Targetable_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Targetable_Requests;

    public:
        CK_USING_BASE_CONSTRUCTORS(TProcessor);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_Targetable_Current& InCurrent,
            const FFragment_Targetable_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_Targetable_Current& InCurrent,
            const FFragment_Targetable_Requests::EnableDisableRequestType& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

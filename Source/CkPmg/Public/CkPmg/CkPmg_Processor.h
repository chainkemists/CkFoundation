#pragma once

#include "CkPmg_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Donut_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Donut_Setup,
            FCk_Handle_Pmg_Donut,
            FFragment_Pmg_Donut_Params,
            FFragment_Pmg_Donut_Current,
            FTag_Pmg_Donut_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Pmg_Donut_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_Donut_Params& InParams,
            FFragment_Pmg_Donut_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Donut_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Pmg_Donut_HandleRequests,
            FCk_Handle_Pmg_Donut,
            FFragment_Pmg_Donut_Current,
            FFragment_Pmg_Donut_UpdateParams,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Pmg_Donut_UpdateParams;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FFragment_Pmg_Donut_UpdateParams& InRequest) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FCk_Request_Pmg_Donut_UpdateParams& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Donut_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Pmg_Donut_UpdateTransform,
            FCk_Handle_Pmg_Donut,
            FFragment_Pmg_Donut_Current,
            FFragment_Transform,
            FTag_Transform_Updated,
            TExclude<FTag_Pmg_Donut_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Donut_EndPlay : public ck_exp::TProcessor<
            FProcessor_Pmg_Donut_EndPlay,
            FCk_Handle_Pmg_Donut,
            FFragment_Pmg_Donut_Current,
            CK_IF_END_PLAY>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkShapeSphere_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeSphere_Setup : public ck_exp::TProcessor<
            FProcessor_ShapeSphere_Setup,
            FCk_Handle_ShapeSphere,
            FFragment_ShapeSphere_Params,
            FFragment_ShapeSphere_Current,
            FTag_ShapeSphere_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeSphere_RequiresSetup;

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
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeSphere_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeSphere_HandleRequests,
            FCk_Handle_ShapeSphere,
            FFragment_ShapeSphere_Params,
            FFragment_ShapeSphere_Current,
            FFragment_ShapeSphere_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ShapeSphere_Requests;

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
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            FFragment_ShapeSphere_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            const FCk_Request_ShapeSphere_UpdateShape& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeSphere_Teardown : public ck_exp::TProcessor<
            FProcessor_ShapeSphere_Teardown,
            FCk_Handle_ShapeSphere,
            FFragment_ShapeSphere_Params,
            FFragment_ShapeSphere_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeSphere_Replicate : public ck_exp::TProcessor<
            FProcessor_ShapeSphere_Replicate,
            FCk_Handle_ShapeSphere,
            FFragment_ShapeSphere_Current,
            TObjectPtr<UCk_Fragment_ShapeSphere_Rep>,
            FTag_ShapeSphere_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeSphere_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeSphere_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeSphere_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkShapeCylinder_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeCylinder_Setup : public ck_exp::TProcessor<
            FProcessor_ShapeCylinder_Setup,
            FCk_Handle_ShapeCylinder,
            FFragment_ShapeCylinder_Params,
            FFragment_ShapeCylinder_Current,
            FTag_ShapeCylinder_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeCylinder_RequiresSetup;

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
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCylinder_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeCylinder_HandleRequests,
            FCk_Handle_ShapeCylinder,
            FFragment_ShapeCylinder_Params,
            FFragment_ShapeCylinder_Current,
            FFragment_ShapeCylinder_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ShapeCylinder_Requests;

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
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            FFragment_ShapeCylinder_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            const FCk_Request_ShapeCylinder_UpdateShape& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCylinder_Teardown : public ck_exp::TProcessor<
            FProcessor_ShapeCylinder_Teardown,
            FCk_Handle_ShapeCylinder,
            FFragment_ShapeCylinder_Params,
            FFragment_ShapeCylinder_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCylinder_Replicate : public ck_exp::TProcessor<
            FProcessor_ShapeCylinder_Replicate,
            FCk_Handle_ShapeCylinder,
            FFragment_ShapeCylinder_Current,
            TObjectPtr<UCk_Fragment_ShapeCylinder_Rep>,
            FTag_ShapeCylinder_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeCylinder_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeCylinder_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeCylinder_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

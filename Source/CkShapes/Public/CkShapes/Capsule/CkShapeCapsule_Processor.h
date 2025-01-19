#pragma once

#include "CkShapeCapsule_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeCapsule_Setup : public ck_exp::TProcessor<
            FProcessor_ShapeCapsule_Setup,
            FCk_Handle_ShapeCapsule,
            FFragment_ShapeCapsule_Params,
            FFragment_ShapeCapsule_Current,
            FTag_ShapeCapsule_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeCapsule_RequiresSetup;

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
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCapsule_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeCapsule_HandleRequests,
            FCk_Handle_ShapeCapsule,
            FFragment_ShapeCapsule_Params,
            FFragment_ShapeCapsule_Current,
            FFragment_ShapeCapsule_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ShapeCapsule_Requests;

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
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            FFragment_ShapeCapsule_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            const FCk_Request_ShapeCapsule_UpdateShape& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCapsule_Teardown : public ck_exp::TProcessor<
            FProcessor_ShapeCapsule_Teardown,
            FCk_Handle_ShapeCapsule,
            FFragment_ShapeCapsule_Params,
            FFragment_ShapeCapsule_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCapsule_Replicate : public ck_exp::TProcessor<
            FProcessor_ShapeCapsule_Replicate,
            FCk_Handle_ShapeCapsule,
            FFragment_ShapeCapsule_Current,
            TObjectPtr<UCk_Fragment_ShapeCapsule_Rep>,
            FTag_ShapeCapsule_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeCapsule_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeCapsule_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeCapsule_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

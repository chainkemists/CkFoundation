#pragma once

#include "CkTargeter_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTARGETING_API FProcessor_Targeter_Update : public ck_exp::TProcessor<
            FProcessor_Targeter_Update,
            FCk_Handle_Targeter,
            FFragment_Targeter_Params,
            FTag_Targeter_NeedsUpdate,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Targeter_NeedsUpdate;

    public:
        CK_USING_BASE_CONSTRUCTORS(TProcessor);

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTARGETING_API FProcessor_Targeter_Cleanup : public ck_exp::TProcessor<
            FProcessor_Targeter_Cleanup,
            FCk_Handle_Targeter,
            FFragment_Targeter_Current,
            FTag_Targeter_NeedsCleanup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Targeter_NeedsCleanup;

    public:
        CK_USING_BASE_CONSTRUCTORS(TProcessor);

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_Targeter_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTARGETING_API FProcessor_Targeter_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Targeter_HandleRequests,
            FCk_Handle_Targeter,
            FFragment_Targeter_Params,
            FFragment_Targeter_Current,
            FFragment_Targeter_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Targeter_Requests;

    public:
        CK_USING_BASE_CONSTRUCTORS(TProcessor);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent,
            FFragment_Targeter_Requests& InRequests) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent,
            const FCk_Request_Targeter_QueryTargets& InRequest) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

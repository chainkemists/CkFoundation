#pragma once

#include "CkVfx_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKFX_API FProcessor_Vfx_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Vfx_HandleRequests,
            FCk_Handle_Vfx,
            FFragment_Vfx_Current,
            FFragment_Vfx_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Vfx_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Vfx_Current& InComp,
            FFragment_Vfx_Requests& InRequestsComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

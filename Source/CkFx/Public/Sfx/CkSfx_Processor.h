#pragma once

#include "CkSfx_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKFX_API FProcessor_Sfx_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Sfx_HandleRequests,
            FCk_Handle_Sfx,
            FFragment_Sfx_Current,
            FFragment_Sfx_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Sfx_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sfx_Current& InComp,
            FFragment_Sfx_Requests& InRequestsComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

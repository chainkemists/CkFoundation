#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKGRID_API FProcessor_2dGridSystem_DebugDrawAll : public ck_exp::TProcessor<
            FProcessor_2dGridSystem_DebugDrawAll,
            FCk_Handle_2dGridSystem,
            ck::FFragment_2dGridSystem_Params,
            ck::FFragment_2dGridSystem_Current,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridSystem_Params& InParams,
            const FFragment_2dGridSystem_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
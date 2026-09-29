#pragma once

#include "CkChain/CkChain_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKCHAIN_API FProcessor_Chain_DebugDraw : public ck_exp::TProcessor<
        FProcessor_Chain_DebugDraw,
        FCk_Handle_Chain,
        TReadOnly<FFragment_Chain_Params>,
        TReadOnly<FFragment_Chain>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Camera;
        using TProcessor::TProcessor;

        auto DoTick(FCk_Time InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            const FFragment_Chain& InCurrent) -> void;
    };
}

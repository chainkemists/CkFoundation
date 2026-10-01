#pragma once

#include "CkGait/Bob/CkBob_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKGAIT_API FProcessor_Gait_DebugDraw : public ck_exp::TProcessor<
        FProcessor_Gait_DebugDraw,
        FCk_Handle_Bob,
        TReadOnly<FFragment_Bob_Tunables>,
        TReadOnly<FFragment_Bob>,
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
            const FFragment_Bob_Tunables& InTunables,
            const FFragment_Bob& InBob) -> void;
    };
}

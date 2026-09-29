#pragma once

#include "CkSway/CkSway_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKSWAY_API FProcessor_Sway_DebugDraw : public ck_exp::TProcessor<
        FProcessor_Sway_DebugDraw,
        FCk_Handle_Sway,
        TReadOnly<FFragment_Sway_Tunables>,
        TReadOnly<FFragment_Sway>,
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
            const FFragment_Sway_Tunables& InTunables,
            const FFragment_Sway& InSway) -> void;
    };
}

#pragma once

#include "CkRotateTowards/CkRotateTowards_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKROTATETOWARDS_API FProcessor_RotateTowards_DebugDraw : public ck_exp::TProcessor<
        FProcessor_RotateTowards_DebugDraw,
        FCk_Handle_RotateTowards,
        TReadOnly<FFragment_RotateTowards_Tunables>,
        TReadOnly<FFragment_RotateTowards>,
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
            const FFragment_RotateTowards_Tunables& InTunables,
            const FFragment_RotateTowards& InRotateTowards) -> void;
    };
}

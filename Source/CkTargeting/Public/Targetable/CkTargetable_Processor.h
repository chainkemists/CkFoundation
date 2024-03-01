#pragma once

#include "CkTargetable_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTARGETING_API FProcessor_Targetable_Setup : public ck_exp::TProcessor<
            FProcessor_Targetable_Setup,
            FCk_Handle_Targetable,
            FFragment_Targetable_Params,
            FFragment_Targetable_Current,
            FTag_Targetable_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Targetable_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        Tick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targetable_Params& InParams,
            FFragment_Targetable_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

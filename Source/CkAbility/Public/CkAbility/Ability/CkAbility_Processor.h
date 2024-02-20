#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_Ability_Teardown : public ck_exp::TProcessor<
            FProcessor_Ability_Teardown,
            FCk_Handle_Ability,
            FFragment_Ability_Current,
            CK_IF_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Ability_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

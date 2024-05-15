#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_Ability_AddReplicated : public ck_exp::TProcessor<
            FProcessor_Ability_AddReplicated,
            FCk_Handle_Ability,
            FCk_EntityReplicationDriver_AbilityData,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FCk_EntityReplicationDriver_AbilityData& InReplicatedAbility) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_Ability_Teardown : public ck_exp::TProcessor<
            FProcessor_Ability_Teardown,
            FCk_Handle_Ability,
            FFragment_Ability_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
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

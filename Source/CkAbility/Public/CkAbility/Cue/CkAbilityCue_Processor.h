#pragma once

#include "GameplayEffectTypes.h"

#include "CkAbility/Cue/CkAbilityCue_Fragment.h"
#include "CkAbility/Cue/CkAbilityCue_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKABILITY_API FProcessor_AbilityCue_Spawn : public TProcessor<FProcessor_AbilityCue_Spawn,
            FFragment_AbilityCue_SpawnRequest, FGameplayCueParameters, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FCk_Request_AbilityCue_Spawn;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityCue_SpawnRequest& InRequest,
            const FGameplayCueParameters& InParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

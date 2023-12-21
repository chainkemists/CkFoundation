#pragma once

#include "GameplayEffectTypes.h"

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKABILITY_API FProcessor_AbilityCue_Spawn : public TProcessor<FProcessor_AbilityCue_Spawn,
            FFragment_AbilityCue_SpawnRequest, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AbilityCue_SpawnRequest;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityCue_SpawnRequest& InRequest) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

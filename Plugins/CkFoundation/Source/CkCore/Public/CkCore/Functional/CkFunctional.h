#pragma once

#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::comparators
{
    struct GameplayTag_MatchesTagExact
    {
        auto operator()(const FGameplayTag& InA, const FGameplayTag& InB) const -> bool
        {
            return InA.MatchesTagExact(InB);
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

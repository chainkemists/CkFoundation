#include "CkAbility_Fragment.h"

#include "CkAbility/Ability/CkAbility_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        MatchesAnyAbilityActivationCancelledTags::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        const auto AbilityName = UCk_Utils_GameplayLabel_UE::Get_Label(InHandle);

        if (NOT UCk_Utils_Ability_UE::Has(InHandle, AbilityName))
        { return false; }

        return UCk_Utils_Ability_UE::Get_ActivationSettings(InHandle, AbilityName).Get_ActivationCancelledTags().HasAnyExact(_Tags);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAbility_Fragment.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

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

        if (NOT UCk_Utils_AbilityOwner_UE::Has_Ability(InHandle, AbilityName))
        { return false; }

        const auto CancelledByTags = [&]()
        {
            const auto& ActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InHandle);
            auto Tags = ActivationSettings.Get_ActivationSettingsOnOwner().Get_CancelledByTagsOnAbilityOwner();
            Tags.AppendTags(ActivationSettings.Get_ActivationSettingsOnSelf().Get_CancelledByTagsOnSelf());

            return Tags;
		}();

        return CancelledByTags.HasAnyExact(_Tags);
    }
}

// --------------------------------------------------------------------------------------------------------------------

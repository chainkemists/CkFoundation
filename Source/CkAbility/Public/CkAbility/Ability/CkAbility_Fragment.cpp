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
        const auto AbilityClass = UCk_Utils_Ability_UE::Get_ScriptClass(InHandle);

        if (UCk_Utils_Ability_UE::Get_ScriptClass(InHandle) != AbilityClass)
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

#include "CkAbility_Fragment.h"

#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        MatchesAnyAbilityActivationCancelledTagsOnSelf::
        operator()(
            const FCk_Handle& InTypeUnsafeHandle) const
        -> bool
    {
        const auto& Handle = UCk_Utils_Ability_UE::CastChecked(InTypeUnsafeHandle);

        const auto CancelledByTags = [&]()
        {
            const auto& ActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(Handle);
            auto Tags = ActivationSettings.Get_ActivationSettingsOnSelf().Get_CancelledByTagsOnSelf();

            return Tags;
        }();

        return CancelledByTags.HasAnyExact(_Tags);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MatchesAnyAbilityActivationCancelledTagsOnOwner::
        operator()(
            const FCk_Handle& InTypeUnsafeHandle) const
        -> bool
    {
        const auto& Handle = UCk_Utils_Ability_UE::CastChecked(InTypeUnsafeHandle);

        const auto CancelledByTags = [&]()
        {
            const auto& ActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(Handle);
            auto Tags = ActivationSettings.Get_ActivationSettingsOnOwner().Get_CancelledByTagsOnAbilityOwner();

            return Tags;
        }();

        return CancelledByTags.HasAnyExact(_Tags);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MatchesAbilityScriptClass::
        operator()(
            const FCk_Handle& InTypeUnsafeHandle) const
        -> bool
    {
        const auto& Handle = UCk_Utils_Ability_UE::CastChecked(InTypeUnsafeHandle);

        return UCk_Utils_Ability_UE::Get_ScriptClass(Handle) == _ScriptClass;
    }
}

// --------------------------------------------------------------------------------------------------------------------

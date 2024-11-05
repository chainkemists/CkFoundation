#include "CkAbilityOwner_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_AbilityOwner_ParamsData::Request_Append(
        const TArray<TSubclassOf<class UCk_Ability_Script_PDA>>& InAbilities)
    -> void
{
    _DefaultAbilities.Append(InAbilities);
}

auto
    FCk_Fragment_AbilityOwner_ParamsData::Request_Append(
        const TArray<UCk_Ability_Script_PDA*>& InAbilities)
    -> void
{
    _DefaultAbilities_Instanced.Append(InAbilities);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_RevokeAbility::
    FCk_Request_AbilityOwner_RevokeAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass)
    , _AbilityClass(InAbilityClass)
{
}

FCk_Request_AbilityOwner_RevokeAbility::
    FCk_Request_AbilityOwner_RevokeAbility(
        FCk_Handle_Ability InAbilityHandle)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
    , _AbilityHandle(std::move(InAbilityHandle))
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass)
    , _AbilityClass(InAbilityClass)
{
}

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        FCk_Handle_Ability InAbilityHandle)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
    , _AbilityHandle(std::move(InAbilityHandle))
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_DeactivateAbility::
    FCk_Request_AbilityOwner_DeactivateAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass)
        , _AbilityClass(InAbilityClass)
{ }

FCk_Request_AbilityOwner_DeactivateAbility::
    FCk_Request_AbilityOwner_DeactivateAbility(
        FCk_Handle_Ability InAbilityHandle)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
        , _AbilityHandle(std::move(InAbilityHandle))
{ }

// --------------------------------------------------------------------------------------------------------------------

#include "CkAbilityOwner_Fragment_Data.h"

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

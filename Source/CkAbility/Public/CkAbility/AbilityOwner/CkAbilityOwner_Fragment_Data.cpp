#include "CkAbilityOwner_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_GiveAbility::
    FCk_Request_AbilityOwner_GiveAbility(
        const UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig)
    : _AbilityEntityConfig(InAbilityEntityConfig)
{ }

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
        FCk_Handle InAbilityHandle)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
    , _AbilityHandle(InAbilityHandle)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass,
        FCk_Ability_ActivationPayload InActivationPayload)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass)
    , _AbilityClass(InAbilityClass)
    , _ActivationPayload(InActivationPayload)
{
}

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        const FCk_Handle_Ability& InAbilityHandle,
        FCk_Ability_ActivationPayload InActivationPayload)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
    , _AbilityHandle(InAbilityHandle)
    , _ActivationPayload(InActivationPayload)
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
        const FCk_Handle_Ability& InAbilityHandle)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
        , _AbilityHandle(InAbilityHandle)
{ }

// --------------------------------------------------------------------------------------------------------------------

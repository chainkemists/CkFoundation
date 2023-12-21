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
        FGameplayTag InAbilityName)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByName)
    , _AbilityName(InAbilityName)
{
}

FCk_Request_AbilityOwner_RevokeAbility::
    FCk_Request_AbilityOwner_RevokeAbility(
        FCk_Handle InAbilityHandle)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle)
    , _AbilityHandle(InAbilityHandle)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        FGameplayTag InAbilityName,
        FCk_Ability_ActivationPayload InActivationPayload)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByName)
    , _AbilityName(InAbilityName)
    , _ActivationPayload(InActivationPayload)
{
}

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        FCk_Handle InAbilityHandle,
        FCk_Ability_ActivationPayload InActivationPayload)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle)
    , _AbilityHandle(InAbilityHandle)
    , _ActivationPayload(InActivationPayload)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_DeactivateAbility::
    FCk_Request_AbilityOwner_DeactivateAbility(
        FGameplayTag InAbilityName)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByName)
        , _AbilityName(InAbilityName)
{ }

FCk_Request_AbilityOwner_DeactivateAbility::
    FCk_Request_AbilityOwner_DeactivateAbility(
        FCk_Handle InAbilityHandle)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle)
        , _AbilityHandle(InAbilityHandle)
{ }

// --------------------------------------------------------------------------------------------------------------------

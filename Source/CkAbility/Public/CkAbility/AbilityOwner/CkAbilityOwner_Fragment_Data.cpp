#include "CkAbilityOwner_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_GiveAbility::
    FCk_Request_AbilityOwner_GiveAbility(
        const UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig)
    : _AbilityEntityConfig(InAbilityEntityConfig)
{ }

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        FGameplayTag InAbilityName,
        FCk_Handle InActivationContextEntity)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByName)
    , _AbilityName(InAbilityName)
    , _ActivationContextEntity(InActivationContextEntity)
{
}

FCk_Request_AbilityOwner_ActivateAbility::
    FCk_Request_AbilityOwner_ActivateAbility(
        FCk_Handle InAbilityHandle,
        FCk_Handle InActivationContextEntity)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle)
    , _AbilityHandle(InAbilityHandle)
    , _ActivationContextEntity(InActivationContextEntity)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_EndAbility::
    FCk_Request_AbilityOwner_EndAbility(
        FGameplayTag InAbilityName)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByName)
        , _AbilityName(InAbilityName)
{ }

FCk_Request_AbilityOwner_EndAbility::
    FCk_Request_AbilityOwner_EndAbility(
        FCk_Handle InAbilityHandle)
        : _SearchPolicy(ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle)
        , _AbilityHandle(InAbilityHandle)
{ }

// --------------------------------------------------------------------------------------------------------------------

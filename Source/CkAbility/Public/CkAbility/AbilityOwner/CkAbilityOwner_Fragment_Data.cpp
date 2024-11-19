#include "CkAbilityOwner_Fragment_Data.h"

#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_AbilityOwner_ParamsData::
    Request_Append(
        const TArray<TSubclassOf<class UCk_Ability_Script_PDA>>& InAbilities)
    -> void
{
    _DefaultAbilities.Append(InAbilities);
}

auto
    FCk_Fragment_AbilityOwner_ParamsData::
    Request_Append(
        const TArray<UCk_Ability_Script_PDA*>& InAbilities)
    -> void
{
    _DefaultAbilities_Instanced.Append(InAbilities);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_AddAndGiveExistingAbility::
    FCk_Request_AbilityOwner_AddAndGiveExistingAbility(
        FCk_Handle_Ability InAbility,
        FCk_Handle InAbilitySource)
    : _Ability(InAbility)
    , _AbilitySource(InAbilitySource)
    , _ReplicationType(ck::IsValid(InAbility) ?
        UCk_Utils_Ability_UE::Get_NetworkSettings(InAbility).Get_ReplicationType() :
        ECk_Net_ReplicationType::All)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_AbilityOwner_RevokeAbility::
    FCk_Request_AbilityOwner_RevokeAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass)
    , _AbilityClass(InAbilityClass)
{
    if (const auto& AbilityCDO = Cast<UCk_Ability_Script_PDA>(_AbilityClass.GetDefaultObject());
        ck::IsValid(AbilityCDO))
    {
        _ReplicationType = AbilityCDO->Get_Data().Get_NetworkSettings().Get_ReplicationType();
    }
}

FCk_Request_AbilityOwner_RevokeAbility::
    FCk_Request_AbilityOwner_RevokeAbility(
        FCk_Handle_Ability InAbilityHandle)
    : _SearchPolicy(ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
    , _AbilityHandle(std::move(InAbilityHandle))
{
    if (ck::IsValid(InAbilityHandle))
    {
        _ReplicationType = UCk_Utils_Ability_UE::Get_NetworkSettings(InAbilityHandle).Get_ReplicationType();
    }
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

#include "CkAbilityOwner_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_BaseHandle::FCk_BaseHandle()
{
    int i = 0;
}

FCk_BaseHandle::FCk_BaseHandle(
    const FMyStruct& Other)
        : _Value(Other._Value)
{
}

FCk_Handle_AbilityOwner::FCk_Handle_AbilityOwner(
    EntityType InEntity,
    const RegistryType& InRegistry): FCk_Handle_TypeSafe(InEntity,
    InRegistry)
{
}

FCk_Handle_AbilityOwner::FCk_Handle_AbilityOwner(
    ThisType&& InOther) noexcept: FCk_Handle_TypeSafe(MoveTemp(InOther))
{
}

FCk_Handle_AbilityOwner::FCk_Handle_AbilityOwner(
    const ThisType& InHandle): FCk_Handle_TypeSafe(InHandle)
{
}

FCk_Handle_AbilityOwner::FCk_Handle_AbilityOwner(
    const FCk_Handle& InHandle): FCk_Handle_TypeSafe(InHandle)
{
}

auto
    FCk_Handle_AbilityOwner::operator=(
        const ThisType& InOther)
        -> ThisType&
{
    Super::operator=(InOther);
    return *this;
}

auto
    FCk_Handle_AbilityOwner::operator=(
        ThisType&& InOther) noexcept
        -> ThisType&
{
    _Entity   = InOther._Entity;
    _Registry = InOther._Registry;
    _Mapper   = InOther._Mapper;
    return *this;
}

auto
    FCk_Handle_AbilityOwner::operator=(
        const FCk_Handle& InOther)
        -> ThisType&
{
    Super::operator=(InOther);
    return *this;
}

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

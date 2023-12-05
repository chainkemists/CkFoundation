#include "CkAbilityOwner_Utils.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Add(
        FCk_Handle                                  InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_AbilityOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_AbilityOwner_Current>();
    InHandle.Add<ck::FTag_AbilityOwner_Setup>();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_AbilityOwner_Current, ck::FFragment_AbilityOwner_Params>();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have an Ability Owner"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTags(
        FCk_Handle InHandle)
    -> FGameplayTagContainer
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTags();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_AbilitiesWithStatus(
        FCk_Handle InHandle,
        ECk_Ability_Status InStatus)
    -> TArray<FGameplayTag>
{
    if (NOT Ensure(InHandle))
    { return {}; }

    TArray<FGameplayTag> ActiveAbilities;

    UCk_Utils_Ability_UE::RecordOfAbilities_Utils::ForEachEntry
    (
        InHandle,
        [&](const FCk_Handle& InAbilityEntity)
        {
            const auto& AbilityName = UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntity);
            if (UCk_Utils_Ability_UE::Get_Status(InAbilityEntity, AbilityName) == InStatus)
            {
                ActiveAbilities.Add(AbilityName);
            }
        }
    );

    return ActiveAbilities;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_Abilities(
        FCk_Handle InHandle)
    -> TArray<FGameplayTag>
{
    if (NOT Ensure(InHandle))
    { return {}; }

    const auto& AbilityEntities = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_AllRecordEntries(InHandle);

    return ck::algo::Transform<TArray<FGameplayTag>>(AbilityEntities, [&](FCk_Handle InAbilityEntity)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntity);
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveAbility(
        FCk_Handle                                  InHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility(
        FCk_Handle                                    InHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TryActivateAbility(
        FCk_Handle                                      InHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_EndAbility(
        FCk_Handle                                 InHandle,
        const FCk_Request_AbilityOwner_EndAbility& InRequest)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    InHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAbilityOwner_Utils.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_AbilityOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_AbilityOwner_Current>();
    InHandle.Add<ck::FTag_AbilityOwner_NeedsSetup>();
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
    Has_Ability(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> bool
{
    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return ck::IsValid(AbilityEntity);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Has_Any(
        FCk_Handle InAbilityOwnerEntity)
    -> bool
{
    return RecordOfAbilities_Utils::Has(InAbilityOwnerEntity);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Ensure_Ability(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Ability(InAbilityOwnerEntity, InAbilityName),
        TEXT("Handle [{}] does NOT have Ability [{}]"), InAbilityOwnerEntity, InAbilityName)
    { return false; }

    return true;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Ensure_Any(
        FCk_Handle InAbilityOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAbilityOwnerEntity), TEXT("Handle [{}] does NOT have any Ability"), InAbilityOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_Ability(
        FCk_Handle   InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> FCk_Handle
{
    if (NOT Ensure_Ability(InAbilityOwnerEntity, InAbilityName))
    { return {}; }

    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return AbilityEntity;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_AbilityCount(
        FCk_Handle InAbilityOwnerEntity)
    -> int32
{
    if (NOT Ensure_Any(InAbilityOwnerEntity))
    { return {}; }

    return RecordOfAbilities_Utils::Get_ValidEntriesCount(InAbilityOwnerEntity);
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability(
        FCk_Handle InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const FCk_Delegate_AbilityOwner_ForEachAbility& InDelegate)
    -> TArray<FCk_Handle>
{
    auto Abilities = TArray<FCk_Handle>{};

    ForEach_Ability(InAbilityOwnerEntity, InForEachAbilityPolicy, [&](FCk_Handle InAbility)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAbility); }
        else
        { Abilities.Emplace(InAbility); }
    });

    return Abilities;
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability(
        FCk_Handle InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT Ensure_Any(InAbilityOwnerEntity))
    { return; }

    RecordOfAbilities_Utils::ForEach_ValidEntry
    (
        InAbilityOwnerEntity,
        [&](const FCk_Handle& InAbilityEntity)
        {
            if (InForEachAbilityPolicy == ECk_AbilityOwner_ForEachAbilityPolicy::IgnoreSelf && InAbilityEntity == InAbilityOwnerEntity)
            { return; }

            InFunc(InAbilityEntity);
        }
    );
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_WithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const FCk_Delegate_AbilityOwner_ForEachAbility& InDelegate)
    -> TArray<FCk_Handle>
{
    auto Abilities = TArray<FCk_Handle>{};

    ForEach_Ability_WithStatus(InAbilityOwnerEntity, InStatus, InForEachAbilityPolicy, [&](FCk_Handle InAbility)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAbility); }
        else
        { Abilities.Emplace(InAbility); }
    });

    return Abilities;
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_WithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT Ensure_Any(InAbilityOwnerEntity))
    { return; }

    ForEach_Ability(InAbilityOwnerEntity, InForEachAbilityPolicy, [&](FCk_Handle InAbility)
    {
        if (InForEachAbilityPolicy == ECk_AbilityOwner_ForEachAbilityPolicy::IgnoreSelf && InAbility == InAbilityOwnerEntity)
        { return; }

        if (UCk_Utils_Ability_UE::Get_Status(InAbility) == InStatus)
        {
            InFunc(InAbility);
        }
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTags(
        FCk_Handle InAbilityOwnerHandle)
    -> FGameplayTagContainer
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return {}; }

    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTags();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTagsWithCount(
        FCk_Handle InAbilityOwnerHandle)
    -> TMap<FGameplayTag, int32>
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return {}; }

    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTagsWithCount();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TryActivateAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_DeactivateAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_SendEvent(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    auto& Events = InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Events>();
    Events._Events.Emplace(InRequest.Get_Event());
}

auto
    UCk_Utils_AbilityOwner_UE::
    BindTo_OnEvents(
        FCk_Handle InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    ck::UUtils_Signal_AbilityOwner_Events::Bind(InAbilityOwnerHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnEvents(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> void
{
    if (NOT Ensure(InAbilityOwnerHandle))
    { return; }

    ck::UUtils_Signal_AbilityOwner_Events::Unbind(InAbilityOwnerHandle, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

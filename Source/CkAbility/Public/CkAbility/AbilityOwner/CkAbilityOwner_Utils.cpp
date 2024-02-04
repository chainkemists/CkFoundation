#include "CkAbilityOwner_Utils.h"

#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"
#include "CkCore/SharedValues/CkSharedValues_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams)
    -> FCk_Handle_AbilityOwner
{
    InHandle.Add<ck::FFragment_AbilityOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_AbilityOwner_Current>();
    InHandle.Add<ck::FTag_AbilityOwner_NeedsSetup>();

    return Conv_HandleToAbilityOwner(InHandle);
}

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(AbilityOwner, UCk_Utils_AbilityOwner_UE, FCk_Handle_AbilityOwner, ck::FFragment_AbilityOwner_Current, ck::FFragment_AbilityOwner_Params);

auto
    UCk_Utils_AbilityOwner_UE::
    Has_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> bool
{
    return RecordOfAbilities_Utils::Get_HasValidEntry_If(InAbilityOwnerEntity,
    [InAbilityClass](const FCk_Handle& InHandle)
    {
        return UCk_Utils_Ability_UE::Get_ScriptClass(UCk_Utils_Ability_UE::Conv_HandleToAbility(InHandle)) == InAbilityClass;
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    Has_Any(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity)
    -> bool
{
    return RecordOfAbilities_Utils::Has(InAbilityOwnerEntity);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Ensure_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Ability(InAbilityOwnerEntity, InAbilityClass),
        TEXT("Handle [{}] does NOT have Ability [{}]"), InAbilityOwnerEntity, InAbilityClass)
    { return false; }

    return true;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Ensure_Any(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAbilityOwnerEntity), TEXT("Handle [{}] does NOT have any Ability"), InAbilityOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> FCk_Handle_Ability
{
    CK_ENSURE_IF_NOT(Has_Ability(InAbilityOwnerEntity, InAbilityClass),
        TEXT("AbilityOwner [{}] does NOT have the Ability [{}]"), InAbilityOwnerEntity, InAbilityClass)
    { return {}; }

    const auto& Handle = RecordOfAbilities_Utils::Get_ValidEntry_If(InAbilityOwnerEntity,
    [InAbilityClass](const FCk_Handle& InHandle)
    {
        return UCk_Utils_Ability_UE::Get_ScriptClass(UCk_Utils_Ability_UE::Conv_HandleToAbility(InHandle)) == InAbilityClass;
    });

    return Handle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Find_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> FCk_Handle_Ability
{
    const auto& AbilityEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Ability_UE,
        RecordOfAbilities_Utils>(InAbilityOwnerEntity, InAbilityName);

    return AbilityEntity;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_AbilityCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity)
    -> int32
{
    if (NOT Ensure_Any(InAbilityOwnerEntity))
    { return {}; }

    return RecordOfAbilities_Utils::Get_ValidEntriesCount(InAbilityOwnerEntity);
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbility_Policy InForEachAbilityPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability(InAbilityOwnerEntity, InForEachAbilityPolicy, [&](FCk_Handle_Ability& InAbility)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAbility, InOptionalPayload); }
        else
        { Abilities.Emplace(InAbility); }
    });

    return Abilities;
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbility_Policy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle_Ability&)>& InFunc)
    -> void
{
    if (NOT Has_Any(InAbilityOwnerEntity))
    { return; }

    RecordOfAbilities_Utils::ForEach_ValidEntry
    (
        InAbilityOwnerEntity,
        [&](FCk_Handle_Ability InAbilityEntity)
        {
            if (InForEachAbilityPolicy == ECk_AbilityOwner_ForEachAbility_Policy::IgnoreSelf && InAbilityEntity == InAbilityOwnerEntity)
            { return; }

            InFunc(InAbilityEntity);
        }
    );
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbility_Policy InForEachAbilityPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability_If
    (
        InAbilityOwnerEntity,
        InForEachAbilityPolicy,
        [&](const FCk_Handle_Ability& InAbility)
        {
            if (InDelegate.IsBound())
            { InDelegate.Execute(InAbility, InOptionalPayload); }
            else
            { Abilities.Emplace(InAbility); }
        },
        [&](const FCk_Handle_Ability& InAbility)  -> bool
        {
            const FCk_SharedBool PredicateResult;

            if (InPredicate.IsBound())
            {
                InPredicate.Execute(InAbility, PredicateResult, InOptionalPayload);
            }

            return *PredicateResult;

        }
    );

    return Abilities;
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbility_Policy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle_Ability)>& InFunc,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate)
    -> void
{
    if (NOT Has_Any(InAbilityOwnerEntity))
    { return; }

    RecordOfAbilities_Utils::ForEach_ValidEntry_If
    (
        InAbilityOwnerEntity,
        [&](FCk_Handle_Ability InAbilityEntity)
        {
            if (InForEachAbilityPolicy == ECk_AbilityOwner_ForEachAbility_Policy::IgnoreSelf && InAbilityEntity == InAbilityOwnerEntity)
            { return; }

            InFunc(InAbilityEntity);
        },
        InPredicate
    );
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_WithStatus(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        ECk_AbilityOwner_ForEachAbility_Policy InForEachAbilityPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability_WithStatus(InAbilityOwnerEntity, InStatus, InForEachAbilityPolicy, [&](FCk_Handle_Ability InAbility)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAbility, InOptionalPayload); }
        else
        { Abilities.Emplace(InAbility); }
    });

    return Abilities;
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_WithStatus(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        ECk_AbilityOwner_ForEachAbility_Policy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle_Ability)>& InFunc)
    -> void
{
    if (NOT Ensure_Any(InAbilityOwnerEntity))
    { return; }

    ForEach_Ability(InAbilityOwnerEntity, InForEachAbilityPolicy, [&](FCk_Handle_Ability InAbility)
    {
        if (InForEachAbilityPolicy == ECk_AbilityOwner_ForEachAbility_Policy::IgnoreSelf && InAbility == InAbilityOwnerEntity)
        { return; }

        if (UCk_Utils_Ability_UE::Get_Status(UCk_Utils_Ability_UE::Conv_HandleToAbility(InAbility)) == InStatus)
        {
            InFunc(InAbility);
        }
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTags(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> FGameplayTagContainer
{
    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTags();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTagsWithCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> TMap<FGameplayTag, int32>
{
    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTagsWithCount();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TryActivateAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_DeactivateAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_SendAbilityEvent(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest)
    -> FCk_Handle_AbilityOwner
{
    auto& Events = InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Events>();
    Events._Events.Emplace(InRequest.Get_Event());

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    BindTo_OnEvents(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    ck::UUtils_Signal_AbilityOwner_Events::Bind(InAbilityOwnerHandle, InDelegate, InBehavior);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnEvents(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    ck::UUtils_Signal_AbilityOwner_Events::Unbind(InAbilityOwnerHandle, InDelegate);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    BindTo_OnTagsUpdated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    ck::UUtils_Signal_AbilityOwner_OnTagsUpdated::Bind(InAbilityOwnerHandle, InDelegate, InBehavior);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnTagsUpdated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    ck::UUtils_Signal_AbilityOwner_OnTagsUpdated::Unbind(InAbilityOwnerHandle, InDelegate);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_ActivateAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA>  InAbilityScriptClass,
        FCk_Ability_ActivationPayload InActivationPayload)
    -> FCk_Request_AbilityOwner_ActivateAbility
{
    return FCk_Request_AbilityOwner_ActivateAbility{InAbilityScriptClass, InActivationPayload};
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_ActivateAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity,
        FCk_Ability_ActivationPayload InActivationPayload)
    -> FCk_Request_AbilityOwner_ActivateAbility
{
    return FCk_Request_AbilityOwner_ActivateAbility{InAbilityEntity, InActivationPayload};
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_DeactivateAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass)
    -> FCk_Request_AbilityOwner_DeactivateAbility
{
    return FCk_Request_AbilityOwner_DeactivateAbility{InAbilityScriptClass};
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_DeactivateAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Request_AbilityOwner_DeactivateAbility
{
    return FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntity};
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_RevokeAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass)
    -> FCk_Request_AbilityOwner_RevokeAbility
{
    return FCk_Request_AbilityOwner_RevokeAbility{InAbilityScriptClass};
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_RevokeAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity)
    -> FCk_Request_AbilityOwner_RevokeAbility
{
    return FCk_Request_AbilityOwner_RevokeAbility{InAbilityEntity};
}

// --------------------------------------------------------------------------------------------------------------------

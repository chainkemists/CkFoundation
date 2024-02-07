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
        const TArray<TSubclassOf<UCk_Ability_Script_PDA>>& InDefaultAbilities)
    -> FCk_Handle_AbilityOwner
{
    return Add(InHandle, FCk_Fragment_AbilityOwner_ParamsData{InDefaultAbilities});
}

auto
    UCk_Utils_AbilityOwner_UE::
    Add_SingleAbility(
        FCk_Handle& InHandle,
        TSubclassOf<UCk_Ability_Script_PDA> InDefaultAbility)
    -> FCk_Handle_AbilityOwner
{
    return Add(InHandle, FCk_Fragment_AbilityOwner_ParamsData{{InDefaultAbility}});
}

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

    UCk_Utils_Ability_UE::RecordOfAbilities_Utils::AddIfMissing(InHandle);

    return Conv_HandleToAbilityOwner(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(AbilityOwner, UCk_Utils_AbilityOwner_UE, FCk_Handle_AbilityOwner, ck::FFragment_AbilityOwner_Current, ck::FFragment_AbilityOwner_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Has_AbilityByClass(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> bool
{
    return ck::IsValid(TryGet_AbilityByClass(InAbilityOwnerEntity, InAbilityClass));
}

auto
    UCk_Utils_AbilityOwner_UE::
    Has_AbilityByName(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> bool
{
    return ck::IsValid(TryGet_AbilityByName(InAbilityOwnerEntity, InAbilityName));
}

auto
    UCk_Utils_AbilityOwner_UE::
    Has_Any(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity)
    -> bool
{
    return Get_AbilityCount(InAbilityOwnerEntity) > 0;
}

auto
    UCk_Utils_AbilityOwner_UE::
    TryGet_AbilityByClass(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> FCk_Handle_Ability
{
    return RecordOfAbilities_Utils::Get_ValidEntry_If(InAbilityOwnerEntity,
    [InAbilityClass](const FCk_Handle& InHandle)
    {
        return UCk_Utils_Ability_UE::Get_ScriptClass(UCk_Utils_Ability_UE::Conv_HandleToAbility(InHandle)) == InAbilityClass;
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    TryGet_AbilityByName(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FGameplayTag InAbilityName)
    -> FCk_Handle_Ability
{
    return RecordOfAbilities_Utils::Get_ValidEntry_If(InAbilityOwnerEntity,
    [InAbilityName](const FCk_Handle& InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(UCk_Utils_Ability_UE::Conv_HandleToAbility(InHandle)) == InAbilityName;
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_AbilityCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity)
    -> int32
{
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
    if (NOT Has_Any(InAbilityOwnerEntity))
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
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass)
    -> FCk_Handle_AbilityOwner
{
    return Request_GiveAbility(InAbilityOwnerHandle, FCk_Request_AbilityOwner_GiveAbility{InAbilityScriptClass});
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
    Request_RevokeAbility_ByEntity(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FCk_Handle InAbilityHandle)
    -> FCk_Handle_AbilityOwner
{
    return Request_RevokeAbility(InAbilityOwnerHandle, FCk_Request_AbilityOwner_RevokeAbility{InAbilityHandle});
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility_ByClass(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> FCk_Handle_AbilityOwner
{
    return Request_RevokeAbility(InAbilityOwnerHandle, FCk_Request_AbilityOwner_RevokeAbility{InAbilityClass});
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
    Request_TryActivateAbility_ByEntity(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FCk_Handle InAbilityHandle,
        FCk_Ability_ActivationPayload InOptionalActivationPayload)
    -> FCk_Handle_AbilityOwner
{
    return Request_TryActivateAbility(InAbilityOwnerHandle,
        FCk_Request_AbilityOwner_ActivateAbility{InAbilityHandle ,InOptionalActivationPayload});
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TryActivateAbility_ByClass(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass,
        FCk_Ability_ActivationPayload InOptionalActivationPayload)
    -> FCk_Handle_AbilityOwner
{
    return Request_TryActivateAbility(InAbilityOwnerHandle,
        FCk_Request_AbilityOwner_ActivateAbility{InAbilityClass ,InOptionalActivationPayload});
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
    Request_DeactivateAbility_ByEntity(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FCk_Handle InAbilityHandle)
    -> FCk_Handle_AbilityOwner
{
    return Request_DeactivateAbility(InAbilityOwnerHandle, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityHandle});
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_DeactivateAbility_ByClass(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> FCk_Handle_AbilityOwner
{
    return Request_DeactivateAbility(InAbilityOwnerHandle, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityClass});
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
        FGameplayTag InEventName,
        FCk_Handle InContextEntity,
        FInstancedStruct InEventData)
    -> FCk_Handle_AbilityOwner
{
    return Request_SendAbilityEvent(InAbilityOwnerHandle,
        FCk_Request_AbilityOwner_SendEvent{FCk_AbilityOwner_Event{InEventName}
            .Set_ContextEntity(InContextEntity)
            .Set_EventData(InEventData)});
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_SendAbilityEvent(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Events>()._Events.Emplace(InRequest.Get_Event());
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

// --------------------------------------------------------------------------------------------------------------------

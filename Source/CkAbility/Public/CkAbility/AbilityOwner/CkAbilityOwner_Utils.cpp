#include "CkAbilityOwner_Utils.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

#include "CkCore/SharedValues/CkSharedValues_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_AbilityOwner
{
    InHandle.Add<ck::FFragment_AbilityOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_AbilityOwner_Current>();
    InHandle.Add<ck::FTag_AbilityOwner_NeedsSetup>();

    UCk_Utils_Ability_UE::RecordOfAbilities_Utils::AddIfMissing(InHandle);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ability::VeryVerbose
        (
            TEXT("Skipping creation of Ability Owner Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InHandle);
    }

    return Cast(InHandle);
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
        return UCk_Utils_Ability_UE::Get_ScriptClass(UCk_Utils_Ability_UE::CastChecked(InHandle)) == InAbilityClass;
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
        return UCk_Utils_GameplayLabel_UE::Get_Label(UCk_Utils_Ability_UE::CastChecked(InHandle)) == InAbilityName;
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
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability(InAbilityOwnerEntity, [&](FCk_Handle_Ability InAbility)
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
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<void(FCk_Handle_Ability)>& InFunc)
    -> void
{
    RecordOfAbilities_Utils::ForEach_ValidEntry
    (
        InAbilityOwnerEntity,
        InFunc,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_If(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability_If
    (
        InAbilityOwnerEntity,
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
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<void(FCk_Handle_Ability)>& InFunc,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate)
    -> void
{
    RecordOfAbilities_Utils::ForEach_ValidEntry_If
    (
        InAbilityOwnerEntity,
        InFunc,
        InPredicate,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_WithStatus(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability_WithStatus(InAbilityOwnerEntity, InStatus, [&](FCk_Handle_Ability InAbility)
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
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const TFunction<void(FCk_Handle_Ability)>& InFunc)
    -> void
{
    ForEach_Ability(InAbilityOwnerEntity, [&](const FCk_Handle_Ability& InAbility)
    {
        if (UCk_Utils_Ability_UE::Get_Status(UCk_Utils_Ability_UE::CastChecked(InAbility)) == InStatus)
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
    Request_GiveAbility_Replicated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest)
    -> FCk_Handle_AbilityOwner
{
    Request_GiveAbility(InAbilityOwnerHandle, InRequest);

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAbilityOwnerHandle),
        TEXT("Cannot Give a REPLICATED Ability to Entity [{}] because it is NOT a Host"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_ENSURE_IF_NOT(UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InAbilityOwnerHandle),
        TEXT("Cannot Give a REPLICATED Ability to Entity [{}] because it's missing the AbilityOwner Replicated Fragment"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(
        InAbilityOwnerHandle, [&](UCk_Fragment_AbilityOwner_Rep* InRepComp)
    {
        InRepComp->_PendingGiveAbilityRequests.Emplace(InRequest);
    });

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
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_AbilityOwner_Events, InAbilityOwnerHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnEvents(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_AbilityOwner_Events, InAbilityOwnerHandle, InDelegate);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    BindTo_OnTagsUpdated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_AbilityOwner_OnTagsUpdated, InAbilityOwnerHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnTagsUpdated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_AbilityOwner_OnTagsUpdated, InAbilityOwnerHandle, InDelegate);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_ActivateAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        FCk_Ability_Payload_OnActivate InOptionalPayload)
    -> FCk_Request_AbilityOwner_ActivateAbility
{
    return FCk_Request_AbilityOwner_ActivateAbility{InAbilityScriptClass}.Set_OptionalPayload(InOptionalPayload);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_ActivateAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity,
        FCk_Ability_Payload_OnActivate InOptionalPayload)
    -> FCk_Request_AbilityOwner_ActivateAbility
{
    return FCk_Request_AbilityOwner_ActivateAbility{InAbilityEntity}.Set_OptionalPayload(InOptionalPayload);
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

#include "CkAbilityOwner_Utils.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/SharedValues/CkSharedValues_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityOwner_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_AbilityOwner
{
    auto AppendedParams = InParams;

    if (InHandle.Has<FCk_Fragment_AbilityOwner_ParamsData>())
    {
        const auto& ParamsToAppend = InHandle.Get<FCk_Fragment_AbilityOwner_ParamsData>();
        AppendedParams.Request_Append(ParamsToAppend.Get_DefaultAbilities());
        AppendedParams.Request_Append(ParamsToAppend.Get_DefaultAbilities_Instanced());
    }

    const auto& Params = InHandle.Add<ck::FFragment_AbilityOwner_Params>(AppendedParams);

    InHandle.Add<ck::FFragment_AbilityOwner_Current>();
    InHandle.Add<ck::FTag_AbilityOwner_NeedsSetup>();

    DoSet_ExpectedNumberOfDependentReplicationDrivers(InHandle, Params);

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

auto
    UCk_Utils_AbilityOwner_UE::
    Append_DefaultAbilities(
        FCk_Handle& InHandle,
        const TArray<TSubclassOf<class UCk_Ability_Script_PDA>>& InDefaultAbilities)
    -> FCk_Handle_AbilityOwner // TODO: [P0] Get rid of this return value since we can no longer guarantee the Handle is an AbilityOwner
{
    // if we do NOT have the AbilityOwner yet, store the Abilities to append
    if (NOT Has(InHandle))
    {
        auto& AbilitiesToAppend = InHandle.AddOrGet<FCk_Fragment_AbilityOwner_ParamsData>();
        AbilitiesToAppend.Request_Append(InDefaultAbilities);
        return {};
    }

    CK_ENSURE_IF_NOT(InHandle.Has<ck::FTag_AbilityOwner_NeedsSetup>(),
        TEXT("Cannot Append DefaultAbilities to Handle [{}] AFTER it's already gone through it's Setup. Call this only in the construction script"),
        InHandle)
    { return {}; }

    auto& Params = InHandle.Get<ck::FFragment_AbilityOwner_Params>();

    auto DefaultAbilities = Params.Get_Params().Get_DefaultAbilities();

    DefaultAbilities.Append(InDefaultAbilities);
    Params = ck::FFragment_AbilityOwner_Params{
        FCk_Fragment_AbilityOwner_ParamsData{DefaultAbilities}
            .Set_DefaultAbilities_Instanced(Params.Get_Params().Get_DefaultAbilities_Instanced())};

    DoSet_ExpectedNumberOfDependentReplicationDrivers(InHandle, Params);

    return Cast(InHandle);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Append_DefaultAbilities_Instanced(
        FCk_Handle& InHandle,
        const TArray<UCk_Ability_Script_PDA*>& InInstancedAbilities)
    -> void
{
    // if we do NOT have the AbilityOwner yet, store the Abilities to append
    if (NOT Has(InHandle))
    {
        auto& AbilitiesToAppend = InHandle.AddOrGet<FCk_Fragment_AbilityOwner_ParamsData>();
        AbilitiesToAppend.Request_Append(InInstancedAbilities);
        return;
    }

    CK_ENSURE_IF_NOT(InHandle.Has<ck::FTag_AbilityOwner_NeedsSetup>(),
        TEXT("Cannot Append DefaultAbilities to Handle [{}] AFTER it's already gone through it's Setup. Call this only in the construction script"),
        InHandle)
    { return; }

    auto& Params = InHandle.Get<ck::FFragment_AbilityOwner_Params>();

    auto DefaultAbilitiesInstanced = Params.Get_Params().Get_DefaultAbilities_Instanced();

    DefaultAbilitiesInstanced.Append(InInstancedAbilities);
    Params = ck::FFragment_AbilityOwner_Params{
        FCk_Fragment_AbilityOwner_ParamsData{Params.Get_Params().Get_DefaultAbilities()}
            .Set_DefaultAbilities_Instanced(DefaultAbilitiesInstanced)};

    DoSet_ExpectedNumberOfDependentReplicationDrivers(InHandle, Params);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AbilityOwner_UE, FCk_Handle_AbilityOwner, ck::FFragment_AbilityOwner_Current, ck::FFragment_AbilityOwner_Params)

auto
    UCk_Utils_AbilityOwner_UE::
    Has_AbilityByHandle(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FCk_Handle_Ability&      InAbility)
    -> bool
{
    return UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_ContainsEntry(InAbilityOwnerEntity, InAbility);
}

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
    TryGet_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> FCk_Handle_Ability
{
    return TryGet_Ability_If(InAbilityOwnerEntity, [&](const FCk_Handle_Ability& InAbility)  -> bool
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InAbility, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    TryGet_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate)
    -> FCk_Handle_Ability
{
    return RecordOfAbilities_Utils::Get_ValidEntry_If(InAbilityOwnerEntity, InPredicate);
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
    Get_AbilityCount_OfClass(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass)
    -> int32
{
    return Get_AbilityCount_If(InAbilityOwnerEntity, ck::algo::MatchesAbilityScriptClass{InAbilityClass});
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_AbilityCount_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> int32
{
    return Get_AbilityCount_If(InAbilityOwnerEntity, [&](const FCk_Handle_Ability& InAbility)  -> bool
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InAbility, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_AbilityCount_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate)
    -> int32
{
    return RecordOfAbilities_Utils::Get_ValidEntriesCount_If(InAbilityOwnerEntity, InPredicate);
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
    RecordOfAbilities_Utils::ForEach_ValidEntry(InAbilityOwnerEntity, InFunc);
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
    RecordOfAbilities_Utils::ForEach_ValidEntry_If(InAbilityOwnerEntity, InFunc, InPredicate);
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
        if (UCk_Utils_Ability_UE::Get_Status(InAbility) == InStatus)
        {
            InFunc(InAbility);
        }
    });
}

auto
    UCk_Utils_AbilityOwner_UE::
    ForEach_Ability_OfClass(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Ability>
{
    auto Abilities = TArray<FCk_Handle_Ability>{};

    ForEach_Ability_OfClass(InAbilityOwnerEntity, InAbilityClass, [&](FCk_Handle_Ability InAbility)
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
    ForEach_Ability_OfClass(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass,
        const TFunction<void(FCk_Handle_Ability)>& InFunc)
    -> void
{
    ForEach_Ability(InAbilityOwnerEntity, [&](const FCk_Handle_Ability& InAbility)
    {
        if (UCk_Utils_Ability_UE::Get_ScriptClass(InAbility) == InAbilityClass)
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
    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTags(InAbilityOwnerHandle);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_PreviousTags(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> FGameplayTagContainer
{
    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_PreviousTags(InAbilityOwnerHandle);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_ActiveTagsWithCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> TMap<FGameplayTag, int32>
{
    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTagsWithCount(InAbilityOwnerHandle);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_SpecificActiveTagCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FGameplayTag InTag)
    -> int32
{
    return InAbilityOwnerHandle.Get<ck::FFragment_AbilityOwner_Current>().Get_SpecificActiveTagCount(InAbilityOwnerHandle, InTag);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_IsBlockingSubAbilities(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> bool
{
    return InAbilityOwnerHandle.Has<ck::FTag_AbilityOwner_BlockSubAbilities>();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_AddAndGiveExistingAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_AddAndGiveExistingAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InAbilityOwnerHandle, InRequest.Get_ReplicationType()))
    {
        ck::ability::Verbose
        (
            TEXT("Skipping Revoking Ability [{}] because ReplicationType [{}] does not match for Entity [{}], meaning this ability shouldn't have have been added on this instance anyways"),
            InRequest.Get_Ability(),
            InRequest.Get_ReplicationType(),
            InAbilityOwnerHandle
        );
        return InAbilityOwnerHandle;
    }

    // Need to re-cast to make sure it all relevant fragments were replicated properly
    const auto AbilityHandle = UCk_Utils_Ability_UE::Cast(InRequest.Get_Ability());
    CK_ENSURE_IF_NOT(ck::IsValid(AbilityHandle), TEXT("AbilityHandle [{}] to Revoke on AbilityOwner [{}] is INVALID"),
        AbilityHandle, InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Ability()),
        TEXT("Unable to process AddAndGiveExistingAbility on Handle [{}] as the AbilityHandle [{}] is INVALID.{}"),
        InAbilityOwnerHandle, InRequest.Get_Ability(), ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot,
        InRequest.PopulateRequestHandle(InAbilityOwnerHandle), InDelegate);

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_AddAndGiveExistingAbility_Replicated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_AddAndGiveExistingAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    Request_AddAndGiveExistingAbility(InAbilityOwnerHandle, InRequest, InDelegate);

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAbilityOwnerHandle),
        TEXT("Cannot REPLICATE AddAndGive an EXISTING Ability to Entity [{}] because it is NOT a Host"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_ENSURE_IF_NOT(UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InAbilityOwnerHandle),
        TEXT("Cannot REPLICATE AddAndGive an existing Ability to Entity [{}] because it's missing the AbilityOwner Replicated Fragment\n."
             "Was the AbilityOwner feature set to Replicate when it was added?"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_EntityReplication(InRequest.Get_Ability()) == ECk_Replication::Replicates,
        TEXT("Cannot REPLICATE AddAndGive an existing Ability to Entity [{}] because the EXISTING Ability [{}] is NOT replicated."),
        InAbilityOwnerHandle, InRequest.Get_Ability())
    { return InAbilityOwnerHandle; }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(
        InAbilityOwnerHandle, [&](UCk_Fragment_AbilityOwner_Rep* InRepComp)
    {
        InRepComp->Request_AddAndGiveExistingAbility(InRequest);
    });

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_AbilityScriptClass()),
        TEXT("Unable to process GiveAbility on Handle [{}] as the AbilityScriptClass is INVALID.{}"), InAbilityOwnerHandle,
        ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot,
        InRequest.PopulateRequestHandle(InAbilityOwnerHandle), InDelegate);

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveAbility_Replicated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    Request_GiveAbility(InAbilityOwnerHandle, InRequest, InDelegate);

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAbilityOwnerHandle),
        TEXT("Cannot Give a REPLICATED Ability to Entity [{}] because it is NOT a Host"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_ENSURE_IF_NOT(UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InAbilityOwnerHandle),
        TEXT("Cannot Give a REPLICATED Ability to Entity [{}] because it's missing the AbilityOwner Replicated Fragment\n."
             "Was the AbilityOwner feature set to Replicate when it was added?"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(
        InAbilityOwnerHandle, [&](UCk_Fragment_AbilityOwner_Rep* InRepComp)
    {
        InRepComp->Request_GiveAbility(InRequest);
    });

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityRevokedOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InAbilityOwnerHandle, InRequest.Get_ReplicationType()))
    {
        if (InRequest.Get_SearchPolicy() == ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
        {
            ck::ability::Verbose
            (
                TEXT("Skipping Revoking Ability [{}] because ReplicationType [{}] does not match for Entity [{}], meaning this ability shouldn't have have been added on this instance anyways"),
                InRequest.Get_AbilityHandle(),
                InRequest.Get_ReplicationType(),
                InAbilityOwnerHandle
            );
        }
        else
        {
            ck::ability::Verbose
            (
                TEXT("Skipping Revoking Ability with class [{}] because ReplicationType [{}] does not match for Entity [{}], meaning this ability shouldn't have have been added on this instance anyways"),
                InRequest.Get_AbilityClass(),
                InRequest.Get_ReplicationType(),
                InAbilityOwnerHandle
            );
        }
        return InAbilityOwnerHandle;
    }

    if (InRequest.Get_SearchPolicy() == ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
    {
        // Need to re-cast to make sure it all relevant fragments were replicated properly
        const auto AbilityHandle = UCk_Utils_Ability_UE::Cast(InRequest.Get_AbilityHandle());
        CK_ENSURE_IF_NOT(ck::IsValid(AbilityHandle), TEXT("AbilityHandle [{}] to Revoke on AbilityOwner [{}] is INVALID"),
            AbilityHandle, InAbilityOwnerHandle)
        { return InAbilityOwnerHandle; }
    }

    CK_ENSURE_IF_NOT(Has_AbilityByClass(InAbilityOwnerHandle, InRequest.Get_AbilityClass()) ||
        Has_AbilityByHandle(InAbilityOwnerHandle, InRequest.Get_AbilityHandle()), TEXT("Ability [{}] does NOT exist on AbilityOwner [{}]"),
        InRequest.Get_SearchPolicy() == ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass
            ? ck::Format(TEXT("{}"), InRequest.Get_AbilityClass()) : ck::Format(TEXT("{}"), InRequest.Get_AbilityHandle()), InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_AbilityOwner_OnAbilityRevokedOrNot,
        InRequest.PopulateRequestHandle(InAbilityOwnerHandle), InDelegate);

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_RevokeAbility_Replicated(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityRevokedOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    Request_RevokeAbility(InAbilityOwnerHandle, InRequest, InDelegate);

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAbilityOwnerHandle),
        TEXT("Cannot Revoke a REPLICATED Ability from Entity [{}] because it is NOT a Host"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    CK_ENSURE_IF_NOT(UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InAbilityOwnerHandle),
        TEXT("Cannot Revoke a REPLICATED Ability from Entity [{}] because it's missing the AbilityOwner Replicated Fragment\n."
             "Was the AbilityOwner feature set to Replicate when it was added?"),
        InAbilityOwnerHandle)
    { return InAbilityOwnerHandle; }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(
        InAbilityOwnerHandle, [&](UCk_Fragment_AbilityOwner_Rep* InRepComp)
    {
        InRepComp->Request_RevokeAbility(InRequest);
    });

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TryActivateAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityActivatedOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot,
        InRequest.PopulateRequestHandle(InAbilityOwnerHandle), InDelegate);

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_DeactivateAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityDeactivatedOrNot& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot,
        InRequest.PopulateRequestHandle(InAbilityOwnerHandle), InDelegate);

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
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Events>()._Events.Emplace(InRequest.Get_Event());

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_BlockAllSubAbilities(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FTag_AbilityOwner_BlockSubAbilities>();
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_UnblockAllSubAbilities(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.Remove<ck::FTag_AbilityOwner_BlockSubAbilities>();
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_CancelAllSubAbilities(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle)
    -> FCk_Handle_AbilityOwner
{
    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(FCk_Request_AbilityOwner_CancelSubAbilities{});
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    BindTo_OnEvents(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FGameplayTagContainer InRelevantEvents,
        FGameplayTagContainer InExcludedEvents,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_BIND_WITH_CONDITION(ck::UUtils_Signal_AbilityOwner_Events, InAbilityOwnerHandle, InDelegate, InBindingPolicy, InPostFireBehavior,
    [InRelevantEvents COMMA InExcludedEvents](FCk_Handle_AbilityOwner InHandle, const TArray<FCk_AbilityOwner_Event>& InEvents)
    {
        const auto& IsWhitelistedEvent = [&InEvents COMMA InRelevantEvents]()
        {
            if (InRelevantEvents.IsEmpty())
            { return true; }

            return ck::algo::AnyOf(InEvents,[InRelevantEvents](const FCk_AbilityOwner_Event& InEvent){ return InRelevantEvents.HasTag(InEvent.Get_EventName()); });
        }();

        const auto& IsBlacklistedEvent = [&InEvents COMMA InExcludedEvents]()
        {
            if (InExcludedEvents.IsEmpty())
            { return false; }

            return ck::algo::AnyOf(InEvents,[InExcludedEvents](const FCk_AbilityOwner_Event& InEvent){ return InExcludedEvents.HasTag(InEvent.Get_EventName()); });
        }();

        return IsWhitelistedEvent && NOT IsBlacklistedEvent;
    });

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
    BindTo_OnSingleEvent(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FGameplayTag InEventName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_Event& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_BIND_WITH_CONDITION(ck::UUtils_Signal_AbilityOwner_SingleEvent, InAbilityOwnerHandle, InDelegate, InBindingPolicy, InPostFireBehavior,
    [InEventName](FCk_Handle_AbilityOwner InHandle, const FCk_AbilityOwner_Event& InEvent)
    {
        return InEvent.Get_EventName().MatchesTagExact(InEventName);
    });

    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    UnbindFrom_OnSingleEvent(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Event& InDelegate)
    -> FCk_Handle_AbilityOwner
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_AbilityOwner_SingleEvent, InAbilityOwnerHandle, InDelegate);
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
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        ECk_AbilityOwner_DestructionOnRevoke_Policy InDestructionPolicy)
    -> FCk_Request_AbilityOwner_RevokeAbility
{
    return FCk_Request_AbilityOwner_RevokeAbility{InAbilityScriptClass}.Set_DestructionPolicy(InDestructionPolicy);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_RevokeAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity,
        ECk_AbilityOwner_DestructionOnRevoke_Policy InDestructionPolicy)
    -> FCk_Request_AbilityOwner_RevokeAbility
{
    return FCk_Request_AbilityOwner_RevokeAbility{InAbilityEntity}.Set_DestructionPolicy(InDestructionPolicy);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Make_Request_AddAndGiveExistingAbility(
        FCk_Handle_Ability InAbility,
        FCk_Handle InAbilitySource,
        FCk_Ability_Payload_OnGranted InOptionalPayload)
    -> FCk_Request_AbilityOwner_AddAndGiveExistingAbility
{
    return FCk_Request_AbilityOwner_AddAndGiveExistingAbility{InAbility, InAbilitySource}.Set_OptionalPayload(InOptionalPayload);
}

auto
    UCk_Utils_AbilityOwner_UE::
    Get_DefaultAbilities(
        const FCk_Handle_AbilityOwner& InAbilityOwner)
    -> const TArray<TSubclassOf<UCk_Ability_Script_PDA>>&
{
    return InAbilityOwner.Get<ck::FFragment_AbilityOwner_Params>().Get_Params().Get_DefaultAbilities();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_TagsUpdated(
        FCk_Handle_AbilityOwner& InAbilityOwner)
    -> void
{
    InAbilityOwner.AddOrGet<ck::FTag_AbilityOwner_TagsUpdated>();
}

auto
    UCk_Utils_AbilityOwner_UE::
    Request_GiveReplicatedAbility(
        FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveReplicatedAbility& InRequest)
    -> FCk_Handle_AbilityOwner
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_AbilityScriptClass()),
        TEXT("Unable to process GiveAbility on Handle [{}] as the AbilityScriptClass is INVALID."), InAbilityOwnerHandle)
    { return {}; }

    InAbilityOwnerHandle.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Emplace(InRequest);
    return InAbilityOwnerHandle;
}

auto
    UCk_Utils_AbilityOwner_UE::
    SyncTagsWithAbilityOwner(
        FCk_Handle_Ability& InAbility,
        FCk_Handle_AbilityOwner& InAbilityOwner,
        const FGameplayTagContainer& InRelevantTags)
    -> void
{
    if (InRelevantTags.IsEmpty())
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAbility),
        TEXT("Ability handle [{}] is INVALID."), InAbility)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityOwner),
        TEXT("AbilityOwner handle [{}] is INVALID."), InAbilityOwner)
    { return; }

    auto AbilityHandleAsOwner = Cast(InAbility);

    if (ck::Is_NOT_Valid(AbilityHandleAsOwner))
    { return; }

    auto& RelevantTagsFromAbilityOwner = AbilityHandleAsOwner.Get<ck::FFragment_AbilityOwner_Current>()._RelevantTagsFromAbilityOwner;

    const auto& OwnerTags = InAbilityOwner.Get<ck::FFragment_AbilityOwner_Current>().Get_ActiveTags(InAbilityOwner);

    const auto Get_FilteredTags = [](const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTagsToMatch , const FGameplayTagContainer& InTagsToAvoid)
    {
        const auto& FilteredTags = ck::algo::Filter(InSourceTags.GetGameplayTagArray(), [&](const FGameplayTag& InTag) -> bool
        {
            return InTag.MatchesAny(InTagsToMatch) && NOT InTagsToAvoid.HasTagExact(InTag);
        });

        return FGameplayTagContainer::CreateFromArray(FilteredTags);
    };

    const auto& TagsToAppend = Get_FilteredTags(OwnerTags, InRelevantTags, RelevantTagsFromAbilityOwner);
    RelevantTagsFromAbilityOwner.AppendTags(TagsToAppend);

    const auto& TagsToRemove = Get_FilteredTags(RelevantTagsFromAbilityOwner, InRelevantTags, OwnerTags);
    RelevantTagsFromAbilityOwner.RemoveTags(TagsToRemove);

    AbilityHandleAsOwner.Get<ck::FFragment_AbilityOwner_Current>().AppendTags(AbilityHandleAsOwner, TagsToAppend);
    AbilityHandleAsOwner.Get<ck::FFragment_AbilityOwner_Current>().RemoveTags(AbilityHandleAsOwner, TagsToRemove);
}

auto
    UCk_Utils_AbilityOwner_UE::
    DoSet_ExpectedNumberOfDependentReplicationDrivers(
        FCk_Handle& InHandle,
        const ck::FFragment_AbilityOwner_Params& InParams)
    -> void
{
    if (NOT InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
    { return; }

    const auto& ReplicationDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    // The expected number of dependent drivers must be cleared to avoid issues when multiple sub-CSs are executed.
    // Each sub-CS can append additional abilities, causing the expected driver count to compound unnecessarily.
    // This would inflate the expected number of drivers, leading to discrepancies with the actual count.
    ReplicationDriver->Set_ExpectedNumberOfDependentReplicationDrivers(0);

    const auto TrySettingTheExpectedNumber = [&](const UCk_Ability_Script_PDA* InScript)
    {
        // this has ensured on top level already
        if (ck::Is_NOT_Valid(InScript))
        { return; }

        if (InScript->Get_Data().Get_NetworkSettings().Get_FeatureReplicationPolicy() == ECk_Ability_FeatureReplication_Policy::ReplicateAbilityFeatures)
        {
            ReplicationDriver->Set_ExpectedNumberOfDependentReplicationDrivers(ReplicationDriver->Get_ExpectedNumberOfDependentReplicationDrivers() + 1);
        }
    };

    for (const auto Ability : InParams.Get_Params().Get_DefaultAbilities())
    {
        const auto AbilityScript = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ability_Script_PDA>(Ability);
        TrySettingTheExpectedNumber(AbilityScript);
    }

    for (const auto Ability : InParams.Get_Params().Get_DefaultAbilities_Instanced())
    {
        const auto AbilityScript = Ability;
        TrySettingTheExpectedNumber(AbilityScript);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Delegates/CkDelegates.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkAbilityOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { struct FFragment_AbilityOwner_Params; }

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityOwner_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

    friend class UCk_Ability_Script_PDA;

public:
    struct RecordOfAbilities_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities> {};

public:
    CK_GENERATED_BODY(UCk_Utils_AbilityOwner_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AbilityOwner);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Add Feature")
    static FCk_Handle_AbilityOwner
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::DoesNotReplicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Append Default Abilities")
    static FCk_Handle_AbilityOwner
    Append_DefaultAbilities(
        UPARAM(ref) FCk_Handle& InHandle,
        const TArray<TSubclassOf<class UCk_Ability_Script_PDA>>& InDefaultAbilities);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Append Default Abilities (Instanced)")
    static void
    Append_DefaultAbilities_Instanced(
        UPARAM(ref) FCk_Handle& InHandle,
        const TArray<UCk_Ability_Script_PDA*>& InInstancedAbilities);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AbilityOwner",
        DisplayName="[Ck][AbilityOwner] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AbilityOwner
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AbilityOwner",
        DisplayName="[Ck][AbilityOwner] Handle -> AbilityOwner Handle",
        meta = (CompactNodeTitle = "<AsAbilityOwner>", BlueprintAutocast))
    static FCk_Handle_AbilityOwner
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Ability (By Handle)")
    static bool
    Has_AbilityByHandle(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FCk_Handle_Ability& InAbility);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Ability (By Class)")
    static bool
    Has_AbilityByClass(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Ability (By Name)")
    static bool
    Has_AbilityByName(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Any Ability")
    static bool
    Has_Any(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Try Get Ability (By Class)")
    static FCk_Handle_Ability
    TryGet_AbilityByClass(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Try Get Ability (By Name)")
    static FCk_Handle_Ability
    TryGet_AbilityByName(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Try Get Ability If")
    static FCk_Handle_Ability
    TryGet_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    TryGet_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate) -> FCk_Handle_Ability;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Ability Count")
    static int32
    Get_AbilityCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Ability Count (Of Class)")
    static int32
    Get_AbilityCount_OfClass(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Ability Count If",
              meta=(AutoCreateRefTerm="InOptionalPayload"))
    static int32
    Get_AbilityCount_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    Get_AbilityCount_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate) -> int32;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<void(FCk_Handle_Ability)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability_If(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_Ability_If(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<void(FCk_Handle_Ability)>& InFunc,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability (With Status)",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability_WithStatus(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability_WithStatus(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const TFunction<void(FCk_Handle_Ability)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability (Of Class)",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability_OfClass(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability_OfClass(
        FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass,
        const TFunction<void(FCk_Handle_Ability)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Active Tags")
    static FGameplayTagContainer
    Get_ActiveTags(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Previous Tags")
    static FGameplayTagContainer
    Get_PreviousTags(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Active Tags With Count")
    static TMap<FGameplayTag, int32>
    Get_ActiveTagsWithCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Specific Active Tag Count")
    static int32
    Get_SpecificActiveTagCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FGameplayTag InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Is Blocking SubAbilities")
    static bool
    Get_IsBlockingSubAbilities(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle);
public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Add and Give Existing Ability",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_AddAndGiveExistingAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_AddAndGiveExistingAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Add and Give Existing Ability (Replicated)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_AddAndGiveExistingAbility_Replicated(
            UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
            const FCk_Request_AbilityOwner_AddAndGiveExistingAbility& InRequest,
            const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Give Ability",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_GiveAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Give Ability (Replicated)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_GiveAbility_Replicated(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Revoke Ability",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_RevokeAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityRevokedOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Revoke Ability (Replicated)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_RevokeAbility_Replicated(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityRevokedOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Try Activate Ability",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AbilityOwner
    Request_TryActivateAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityActivatedOrNot& InDelegate);

    // NOTE: This is for development only. Use 'Request_SendEvent' to trigger Deactivation of Abilities
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Deactivate Ability",
              meta = (AutoCreateRefTerm = "InDelegate", DevelopmentOnly))
    static FCk_Handle_AbilityOwner
    Request_DeactivateAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest,
        const FCk_Delegate_AbilityOwner_OnAbilityDeactivatedOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][AbilityOwner] Request Send Ability Event")
    static FCk_Handle_AbilityOwner
    Request_SendAbilityEvent(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AbilityOwner",
              DisplayName="[Ck][AbilityOwner] Request Block All SubAbilities")
    static FCk_Handle_AbilityOwner
    Request_BlockAllSubAbilities(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AbilityOwner",
              DisplayName="[Ck][AbilityOwner] Request Unblock All SubAbilities")
    static FCk_Handle_AbilityOwner
    Request_UnblockAllSubAbilities(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AbilityOwner",
              DisplayName="[Ck][AbilityOwner] Request Cancel All SubAbilities")
    static FCk_Handle_AbilityOwner
    Request_CancelAllSubAbilities(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Bind To OnEvents")
    static FCk_Handle_AbilityOwner
    BindTo_OnEvents(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FGameplayTagContainer InRelevantEvents,
        FGameplayTagContainer InExcludedEvents,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Unbind From OnEvents")
    static FCk_Handle_AbilityOwner
    UnbindFrom_OnEvents(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Bind To OnSingleEvent")
    static FCk_Handle_AbilityOwner
    BindTo_OnSingleEvent(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        FGameplayTag InEventName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Unbind From OnSingleEvent")
    static FCk_Handle_AbilityOwner
    UnbindFrom_OnSingleEvent(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Event& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Bind To OnTagsUpdated")
    static FCk_Handle_AbilityOwner
    BindTo_OnTagsUpdated(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Unbind From OnTagsUpdated")
    static FCk_Handle_AbilityOwner
    UnbindFrom_OnTagsUpdated(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_ActivateAbility
    Make_Request_ActivateAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        FCk_Ability_Payload_OnActivate InOptionalPayload);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_ActivateAbility
    Make_Request_ActivateAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity,
        FCk_Ability_Payload_OnActivate InOptionalPayload);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_DeactivateAbility
    Make_Request_DeactivateAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_DeactivateAbility
    Make_Request_DeactivateAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_RevokeAbility
    Make_Request_RevokeAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        ECk_AbilityOwner_DestructionOnRevoke_Policy InDestructionPolicy = ECk_AbilityOwner_DestructionOnRevoke_Policy::DestroyOnRevoke);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_RevokeAbility
    Make_Request_RevokeAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity,
        ECk_AbilityOwner_DestructionOnRevoke_Policy InDestructionPolicy = ECk_AbilityOwner_DestructionOnRevoke_Policy::DestroyOnRevoke);

public:
    static auto
    Get_DefaultAbilities(
        const FCk_Handle_AbilityOwner& InAbilityOwner) -> const TArray<TSubclassOf<class UCk_Ability_Script_PDA>>&;

    static auto
    Request_TagsUpdated(
        FCk_Handle_AbilityOwner& InAbilityOwner) -> void;

    static auto
    Request_GiveReplicatedAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveReplicatedAbility& InRequest) -> FCk_Handle_AbilityOwner;

private:
    static auto
    SyncTagsWithAbilityOwner(
        FCk_Handle_Ability& InAbility,
        FCk_Handle_AbilityOwner& InAbilityOwner,
        const FGameplayTagContainer& InRelevantTags) -> void;

    static auto
    DoSet_ExpectedNumberOfDependentReplicationDrivers(
        FCk_Handle& InHandle,
        const ck::FFragment_AbilityOwner_Params& InParams) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

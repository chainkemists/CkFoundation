#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Delegates/CkDelegates.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkAbilityOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityOwner_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    struct RecordOfAbilities_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities> {};

public:
    CK_GENERATED_BODY(UCk_Utils_AbilityOwner_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Add Feature")
    static FCk_Handle_AbilityOwner
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AbilityOwner",
        DisplayName="[Ck][AbilityOwner] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AbilityOwner
    Cast(
        const FCk_Handle&    InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|AbilityOwner",
        DisplayName="[Ck][AbilityOwner] Handle -> AbilityOwner Handle",
        meta = (CompactNodeTitle = "As AbilityOwnerHandle", BlueprintAutocast))
    static FCk_Handle_AbilityOwner
    Conv_HandleToAbilityOwner(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Ability (By Class)",
              meta=(DevelopmentOnly))
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
              DisplayName="[Ck][AbilityOwner] Try Get Ability (By Class)",
              meta=(DevelopmentOnly))
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
              DisplayName="[Ck][AbilityOwner] Get Ability Count")
    static int32
    Get_AbilityCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<void(UPARAM(ref) FCk_Handle_Ability&)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_Ability_If(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        const TFunction<void(FCk_Handle_Ability)>& InFunc,
        const TFunction<bool(FCk_Handle_Ability)>& InPredicate) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability with Status",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Ability>
    ForEach_Ability_WithStatus(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability_WithStatus(
        const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
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
              DisplayName="[Ck][AbilityOwner] Get Active Tags With Count")
    static TMap<FGameplayTag, int32>
    Get_ActiveTagsWithCount(
        const FCk_Handle_AbilityOwner& InAbilityOwnerHandle);
public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Give Ability")
    static FCk_Handle_AbilityOwner
    Request_GiveAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Revoke Ability")
    static FCk_Handle_AbilityOwner
    Request_RevokeAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest);

    // NOTE: This is for development only. Use 'Request_SendEvent' to trigger Activation of Abilities
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Try Activate Ability")
    static FCk_Handle_AbilityOwner
    Request_TryActivateAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest);

    // NOTE: This is for development only. Use 'Request_SendEvent' to trigger Deactivation of Abilities
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][AbilityOwner] Request Deactivate Ability")
    static FCk_Handle_AbilityOwner
    Request_DeactivateAbility(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][AbilityOwner] Request Send Ability Event")
    static FCk_Handle_AbilityOwner
    Request_SendAbilityEvent(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Bind To OnEvents")
    static FCk_Handle_AbilityOwner
    BindTo_OnEvents(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
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
              DisplayName = "[Ck][AbilityOwner] Bind To OnTagsUpdated")
    static FCk_Handle_AbilityOwner
    BindTo_OnTagsUpdated(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Unbind From OnTagsUpdated")
    static FCk_Handle_AbilityOwner
    UnbindFrom_OnTagsUpdated(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_OnTagsUpdated& InDelegate);

public:
    UFUNCTION(BlueprintCallable)
    static FMyStruct MakeMyStruct(float Value);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_ActivateAbility
    Make_Request_ActivateAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass,
        FCk_Ability_ActivationPayload InActivationPayload);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_ActivateAbility
    Make_Request_ActivateAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity,
        FCk_Ability_ActivationPayload InActivationPayload);

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
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_RevokeAbility
    Make_Request_RevokeAbility_ByEntity(
        const FCk_Handle_Ability& InAbilityEntity);
};

// --------------------------------------------------------------------------------------------------------------------

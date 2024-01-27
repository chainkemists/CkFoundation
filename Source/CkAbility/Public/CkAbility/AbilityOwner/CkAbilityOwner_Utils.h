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
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Add Feature")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Ability")
    static bool
    Has_Ability(
        FCk_Handle InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Has Any Ability")
    static bool
    Has_Any(
        FCk_Handle InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Ensure Has Ability")
    static bool
    Ensure_Ability(
        FCk_Handle InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Ensure Has Any Ability")
    static bool
    Ensure_Any(
        FCk_Handle InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Ability",
              meta=(DevelopmentOnly))
    static FCk_Handle
    Get_Ability(
        FCk_Handle InAbilityOwnerEntity,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Find Ability")
    static FCk_Handle
    Find_Ability(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Ability Count")
    static int32
    Get_AbilityCount(
        FCk_Handle InAbilityOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_Ability(
        FCk_Handle InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability(
        FCk_Handle InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_Ability_If(
        FCk_Handle InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_Ability_If(
        FCk_Handle InAbilityOwnerEntity,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] For Each Ability with Status",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_Ability_WithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Ability_WithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        ECk_AbilityOwner_ForEachAbilityPolicy InForEachAbilityPolicy,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Active Tags")
    static FGameplayTagContainer
    Get_ActiveTags(
        FCk_Handle InAbilityOwnerHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Get Active Tags With Count")
    static TMap<FGameplayTag, int32>
    Get_ActiveTagsWithCount(
        FCk_Handle InAbilityOwnerHandle);
public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Request Give Ability")
    static void
    Request_GiveAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Request Revoke Ability")
    static void
    Request_RevokeAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest);

    // NOTE: This is for development only. Use 'Request_SendEvent' to trigger Activation of Abilities
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Request Try Activate Ability",
              meta = (DevelopmentOnly))
    static void
    Request_TryActivateAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest);

    // NOTE: This is for development only. Use 'Request_SendEvent' to trigger Deactivation of Abilities
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="[Ck][AbilityOwner] Request Deactivate Ability",
              meta = (DevelopmentOnly))
    static void
    Request_DeactivateAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Request Send Ability Event")
    static void
    Request_SendAbilityEvent(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Bind To OnEvents")
    static void
    BindTo_OnEvents(
        FCk_Handle InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "[Ck][AbilityOwner] Unbind From OnEvents")
    static void
    UnbindFrom_OnEvents(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate);

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
        FCk_Handle                    InAbilityEntity,
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
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_RevokeAbility
    Make_Request_RevokeAbility_ByClass(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner")
    static FCk_Request_AbilityOwner_RevokeAbility
    Make_Request_RevokeAbility_ByEntity(
        FCk_Handle InAbilityEntity);
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkAbilityOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_AbilityOwner_ForEachAbility,
    FCk_Handle, InAbilityHandle);

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
              DisplayName="Add Ability Owner")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AbilityOwner_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Is Ability Owner")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Ensure Is Ability Owner")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Has Ability")
    static bool
    Has_Ability(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Has Any Ability")
    static bool
    Has_Any(
        FCk_Handle InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Ensure Has Ability")
    static bool
    Ensure_Ability(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Ensure Has Any Ability")
    static bool
    Ensure_Any(
        FCk_Handle InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Get Ability")
    static FCk_Handle
    Get_Ability(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="For Each Ability",
              meta=(AutoCreateRefTerm="InDelegate"))
    static TArray<FCk_Handle>
    ForEach_Ability(
        FCk_Handle InAbilityOwnerEntity,
        const FCk_Delegate_AbilityOwner_ForEachAbility& InDelegate);
    static auto
    ForEach_Ability(
        FCk_Handle InAbilityOwnerEntity,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="For Each Ability with Status",
              meta=(AutoCreateRefTerm="InDelegate"))
    static TArray<FCk_Handle>
    ForEach_Ability_WithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const FCk_Delegate_AbilityOwner_ForEachAbility& InDelegate);
    static auto
    ForEach_Ability_WithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Get All Ability Owner Active Tags")
    static FGameplayTagContainer
    Get_ActiveTags(
        FCk_Handle InAbilityOwnerHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Get All Ability Owner Active Tags With Count")
    static TMap<FGameplayTag, int32>
    Get_ActiveTagsWithCount(
        FCk_Handle InAbilityOwnerHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_GiveAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_RevokeAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_TryActivateAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_DeactivateAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_DeactivateAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "Send Event on AbilityOwner")
    static void
    Request_SendEvent(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_SendEvent& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "Bind To AbilityOwner Events")
    static void
    BindTo_OnEvents(
        FCk_Handle InAbilityOwnerHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_AbilityOwner_Events& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName = "Unbind From AbilityOwner Events")
    static void
    UnbindFrom_OnEvents(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Delegate_AbilityOwner_Events& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkAbilityOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityOwner_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

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
              DisplayName="Has Ability Owner")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Ensure Has Ability Owner")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability|Owner",
              DisplayName="Get All Ability Owner Active Tags")
    static FGameplayTagContainer
    Get_ActiveTags(
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
    Request_EndAbility(
        FCk_Handle InAbilityOwnerHandle,
        const FCk_Request_AbilityOwner_EndAbility& InRequest);

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

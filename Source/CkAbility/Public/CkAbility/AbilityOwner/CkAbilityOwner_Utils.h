#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

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
              Category = "Ck|Utils|Ability",
              DisplayName="Get All Ability Owner Active Tags")
    static FGameplayTagContainer
    Get_ActiveTags(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get All Abilities With Status")
    static TArray<FGameplayTag>
    Get_AbilitiesWithStatus(
        FCk_Handle InHandle,
        ECk_Ability_Status InStatus);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get All Abilities")
    static TArray<FGameplayTag>
    Get_Abilities(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_GiveAbility(
        FCk_Handle InHandle,
        const FCk_Request_AbilityOwner_GiveAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_RevokeAbility(
        FCk_Handle InHandle,
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_TryActivateAbility(
        FCk_Handle InHandle,
        const FCk_Request_AbilityOwner_ActivateAbility& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability|Owner")
    static void
    Request_EndAbility(
        FCk_Handle InHandle,
        const FCk_Request_AbilityOwner_EndAbility& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------

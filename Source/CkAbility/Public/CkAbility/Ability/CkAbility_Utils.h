#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkNet/CkNet_Common.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAbility_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_AbilityOwner_Setup;
    class FProcessor_AbilityOwner_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_Ability_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ability_UE);

public:
    struct RecordOfAbilities_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;
    friend class UCk_Ability_ConstructionScript_PDA;
    friend class ck::FProcessor_AbilityOwner_Setup;
    friend class ck::FProcessor_AbilityOwner_HandleRequests;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName="Add New Ability")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName="Add Multiple New Abilities")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleAbility_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Has Ability")
    static bool
    Has(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Has Any Ability")
    static bool
    Has_Any(
        FCk_Handle InAbilityOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Ensure Has Ability")
    static bool
    Ensure(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Ensure Has Any Ability")
    static bool
    Ensure_Any(
        FCk_Handle InAbilityOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get All Abilities With Status")
    static TArray<FGameplayTag>
    Get_AllWithStatus(
        FCk_Handle InAbilityOwnerEntity,
        ECk_Ability_Status InStatus);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get All Abilities")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InAbilityOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Activation Settings")
    static FCk_Ability_ActivationSettings
    Get_ActivationSettings(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Network Settings")
    static FCk_Ability_NetworkSettings
    Get_NetworkSettings(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Status")
    static ECk_Ability_Status
    Get_Status(
        FCk_Handle InAbilityOwnerEntity,
        FGameplayTag InAbilityName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Status from Handle")
    static ECk_Ability_Status
    Get_Status_FromHandle(
        FCk_Handle InAbilityHandle);

private:
    static auto
    DoAdd(
        FCk_Handle InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams) -> void;

// TODO: Move these back to the processor ?
private:
    static auto
    DoActivate(
        FCk_Handle InHandle) -> void;

    static auto
    DoEnd(
        FCk_Handle InHandle) -> void;

    static auto
    DoGet_CanActivate(
        FCk_Handle InHandle) -> bool;

    static auto
    DoGive(
        FCk_Handle InAbilityOwner,
        FCk_Handle InAbility) -> void;

    static auto
    DoRevoke(
        FCk_Handle InAbilityOwner,
        FCk_Handle InAbility) -> void;

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

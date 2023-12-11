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

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Info
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Info);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Activation",
              meta = (AllowPrivateAccess = true))
    FCk_Handle _AbilityOwnerEntity;

public:
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY_GET(_AbilityOwnerEntity);

    CK_DEFINE_CONSTRUCTORS(FCk_Ability_Info, _AbilityName, _AbilityOwnerEntity);
};

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

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Has Ability")
    static bool
    Has(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Ensure Has Ability")
    static bool
    Ensure(
        FCk_Handle InAbilityEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
			  meta=(CompactNodeTitle="."))
    static FCk_Ability_Info
    Get_Info(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Activation Settings")
    static FCk_Ability_ActivationSettings
    Get_ActivationSettings(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Network Settings")
    static FCk_Ability_NetworkSettings
    Get_NetworkSettings(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="Get Ability Status")
    static ECk_Ability_Status
    Get_Status(
        FCk_Handle InAbilityEntity);

private:
    static auto
    DoAdd(
        FCk_Handle InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams) -> void;

// TODO: Move these back to the processor ?
private:
    static auto
    DoActivate(
        FCk_Handle InAbilityEntity) -> void;

    static auto
    DoEnd(
        FCk_Handle InAbilityEntity) -> void;

    static auto
    DoGet_CanActivate(
        FCk_Handle InAbilityEntity) -> bool;

    static auto
    DoGive(
        FCk_Handle InAbilityOwner,
        FCk_Handle InAbility) -> void;

    static auto
    DoRevoke(
        FCk_Handle InAbilityOwner,
        FCk_Handle InAbility) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

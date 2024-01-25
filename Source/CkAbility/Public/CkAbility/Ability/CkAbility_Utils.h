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
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Has Feature")
    static bool
    Has(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InAbilityEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Info",
              meta=(CompactNodeTitle="AbilityInfo"))
    static FCk_Ability_Info
    Get_Info(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Activation Settings")
    static FCk_Ability_ActivationSettings
    Get_ActivationSettings(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Network Settings")
    static FCk_Ability_NetworkSettings
    Get_NetworkSettings(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Status")
    static ECk_Ability_Status
    Get_Status(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Script Class")
    static TSubclassOf<UCk_Ability_Script_PDA>
    Get_ScriptClass(
        FCk_Handle InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Can Activate")
    static ECk_Ability_ActivationRequirementsResult
    Get_CanActivate(
        FCk_Handle InAbilityEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Bind To OnActivated")
    static void
    BindTo_OnAbilityActivated(
        FCk_Handle InAbilityHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Ability_OnActivated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Unbind From OnActivated")
    static void
    UnbindFrom_OnAbilityActivated(
        FCk_Handle InAbilityHandle,
        const FCk_Delegate_Ability_OnActivated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Bind To OnDeactivated")
    static void
    BindTo_OnAbilityDeactivated(
        FCk_Handle InAbilityHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Unbind From OnDeactivated")
    static void
    UnbindFrom_OnAbilityDeactivated(
        FCk_Handle InAbilityHandle,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate);

private:
    static auto
    DoAdd(
        FCk_Handle InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams) -> void;

// TODO: Move these back to the processor ?
private:
    static auto
    DoActivate(
        FCk_Handle InAbilityEntity,
        const FCk_Ability_ActivationPayload& InActivationPayload) -> void;

    static auto
    DoDeactivate(
        FCk_Handle InAbilityOwnerEntity,
        FCk_Handle InAbilityEntity) -> void;

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

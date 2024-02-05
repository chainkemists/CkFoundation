#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

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
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Has Feature")
    static bool
    Has(
        const FCk_Handle& InAbilityEntity);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Ability",
        DisplayName="[Ck][Ability] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Ability
    Cast(
        const FCk_Handle&    InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Ability",
        DisplayName="[Ck][Ability] Handle -> Ability Handle",
        meta = (CompactNodeTitle = "As AbilityHandle", BlueprintAutocast))
    static FCk_Handle_Ability
    Conv_HandleToAbility(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Create Ability Entity Config",
              meta = (AllowAbstract = false, DefaultToSelf = "InOuter"))
    static UCk_Ability_EntityConfig_PDA*
    Create_AbilityEntityConfig(
        UObject* InOuter,
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityScriptClass);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Create Multiple Ability Entity Config",
              meta = (AllowAbstract = false, DefaultToSelf = "InOuter"))
    static TArray<UCk_Ability_EntityConfig_PDA*>
    Create_MultipleAbilityEntityConfigs(
        UObject* InOuter,
        const TArray<TSubclassOf<UCk_Ability_Script_PDA>> InAbilityScriptClasses);

public:
              DisplayName="[Ck][Ability] Get Display Name")
    static FName
    Get_DisplayName(
        const FCk_Handle_Ability& InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Activation Settings")
    static FCk_Ability_ActivationSettings
    Get_ActivationSettings(
        const FCk_Handle_Ability& InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Network Settings")
    static FCk_Ability_NetworkSettings
    Get_NetworkSettings(
        const FCk_Handle_Ability& InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Status")
    static ECk_Ability_Status
    Get_Status(
        const FCk_Handle_Ability& InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Script Class")
    static TSubclassOf<class UCk_Ability_Script_PDA>
    Get_ScriptClass(
        const FCk_Handle_Ability& InAbilityEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ability",
              DisplayName="[Ck][Ability] Get Can Activate")
    static ECk_Ability_ActivationRequirementsResult
    Get_CanActivate(
        const FCk_Handle_Ability& InAbilityEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Bind To OnActivated")
    static FCk_Handle_Ability
    BindTo_OnAbilityActivated(
        UPARAM(ref) FCk_Handle_Ability& InAbilityHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Ability_OnActivated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Unbind From OnActivated")
    static FCk_Handle_Ability
    UnbindFrom_OnAbilityActivated(
        UPARAM(ref) FCk_Handle_Ability& InAbilityHandle,
        const FCk_Delegate_Ability_OnActivated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Bind To OnDeactivated")
    static FCk_Handle_Ability
    BindTo_OnAbilityDeactivated(
        UPARAM(ref) FCk_Handle_Ability& InAbilityHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ability",
              DisplayName = "[Ck][Ability] Unbind From OnDeactivated")
    static FCk_Handle_Ability
    UnbindFrom_OnAbilityDeactivated(
        UPARAM(ref) FCk_Handle_Ability& InAbilityHandle,
        const FCk_Delegate_Ability_OnDeactivated& InDelegate);

private:
    static auto
    DoAdd(
        FCk_Handle& InHandle,
        const FCk_Fragment_Ability_ParamsData& InParams) -> void;

// TODO: Move these back to the processor ?
private:
    static auto
    DoActivate(
        UPARAM(ref) FCk_Handle_Ability& InAbilityEntity,
        const FCk_Ability_ActivationPayload& InActivationPayload) -> void;

    static auto
    DoDeactivate(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
        UPARAM(ref) FCk_Handle_Ability& InAbilityEntity) -> void;

    static auto
    DoGive(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwner,
        UPARAM(ref) FCk_Handle_Ability& InAbility) -> void;

    static auto
    DoRevoke(
        UPARAM(ref) FCk_Handle_AbilityOwner& InAbilityOwner,
        UPARAM(ref) FCk_Handle_Ability& InAbility) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

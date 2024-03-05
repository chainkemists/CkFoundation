#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkAbility_Script.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// This Object itself is NOT replicated. It may be 'implicitly' replicated through the Ability's replicated fragment
// The interface is purposefully similar to GAS' Gameplay Ability.
// We chose to use the term 'Activate/Deactivate Ability' as opposed to 'Start/End Ability' since it evokes a more immediate process
UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_Script_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    friend class UCk_Utils_Ability_UE;

public:
    CK_GENERATED_BODY(UCk_Ability_Script_PDA);

public:
    auto
    OnGiveAbility(
        const FCk_Ability_Payload_OnGranted& InOptionalPayload) -> void;

    auto
    OnRevokeAbility() -> void;

    auto
    OnActivateAbility(
        const FCk_Ability_Payload_OnActivate& InOptionalPayload) -> void;

    auto
    OnDeactivateAbility() -> void;

    auto
    Get_CanBeGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const -> bool;

protected:
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnActivateAbility"))
    void
    DoOnActivateAbility(
        const FCk_Ability_Payload_OnActivate& InActivationPayload);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnDeactivateAbility"))
    void
    DoOnDeactivateAbility();

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnGiveAbility"))
    void
    DoOnGiveAbility(
        const FCk_Ability_Payload_OnGranted& InOptionalPayload);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnRevokeAbility"))
    void
    DoOnRevokeAbility();

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "CanBeGiven"))
    bool
    DoGet_CanBeGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const;

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Activate This Ability",
              meta = (CompactNodeTitle="TRYACTIVATE_ThisAbility", HideSelfPin = true))
    void
    DoRequest_ActivateAbility(
        FCk_Ability_Payload_OnActivate InActivationPayload);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Deactivate This Ability",
              meta = (CompactNodeTitle="DEACTIVATE_ThisAbility", HideSelfPin = true))
    void
    DoRequest_DeactivateAbility();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Cost (On Self)",
              meta = (CompactNodeTitle="ApplyCost", HideSelfPin = true))
    void
    DoRequest_ApplyCost();

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Custom Cost (On Self)",
              meta = (CompactNodeTitle="ApplyCustomCost", HideSelfPin = true))
    void
    DoRequest_ApplyCustomCost(
        FGameplayTag InAbilityCostTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Cost (On Owner)",
              meta = (CompactNodeTitle="ApplyCost_OnOwner", HideSelfPin = true))
    void
    DoRequest_ApplyCost_OnOwner();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Custom Cost (On Owner)",
              meta = (CompactNodeTitle="ApplyCustomCost_OnOwner", HideSelfPin = true))
    void
    DoRequest_ApplyCustomCost_OnOwner(
        FGameplayTag InAbilityCostTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Cooldown (On Self)",
              meta = (CompactNodeTitle="ApplyCooldown", HideSelfPin = true))
    void
    DoRequest_ApplyCooldown();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Custom Cooldown (On Self)",
              meta = (CompactNodeTitle="ApplyCustomCooldown", HideSelfPin = true))
    void
    DoRequest_ApplyCustomCooldown(
        FGameplayTag InAbilityCooldownTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply Cooldown (On Owner)",
              meta = (CompactNodeTitle="ApplyCooldown_OnOwner", HideSelfPin = true))
    void
    DoRequest_ApplyCooldown_OnOwner();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Apply CustomCooldown (On Owner)",
              meta = (CompactNodeTitle="ApplyCustomCooldown_OnOwner", HideSelfPin = true))
    void
    DoRequest_ApplyCustomCooldown_OnOwner(
        FGameplayTag InAbilityCooldownTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Track Task",
              meta = (CompactNodeTitle="TRACK_Task", HideSelfPin = true))
    void
    DoRequest_TrackTask(
        UObject* InTask);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Spawn Ability Cue",
              meta = (CompactNodeTitle="SpawnAbilityCue", HideSelfPin = true))
    void
    DoRequest_SpawnAbilityCue(
        const FCk_AbilityCue_Params& InReplicatedParams,
        FGameplayTag InAbilityCueName);

private:
    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Status",
              meta = (CompactNodeTitle="STATUS_ThisAbility", HideSelfPin = true))
    ECk_Ability_Status
    DoGet_Status();

    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Entity",
              meta = (CompactNodeTitle="AbilityEntity", HideSelfPin = true))
    FCk_Handle_Ability
    DoGet_AbilityEntity();

    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Owner Entity",
              meta = (CompactNodeTitle="AbilityOwnerEntity", HideSelfPin = true))
    FCk_Handle_AbilityOwner
    DoGet_AbilityOwnerEntity();

private:
    UPROPERTY(EditDefaultsOnly,
              meta = (ShowOnlyInnerProperties))
    FCk_Ability_Script_Data _Data;

private:
    UPROPERTY(Transient)
    FCk_Handle_Ability _AbilityHandle;

    UPROPERTY(Transient)
    FCk_Handle_AbilityOwner _AbilityOwnerHandle;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> _Tasks;

private:
#if NOT CK_DISABLE_ABILITY_SCRIPT_DEBUGGING
    enum class EActivatedDeactivated
    {
        None,
        Activated,
        Deactivated
    };

    enum class EGivenRevoked
    {
        None,
        Given,
        Revoked
    };

    EActivatedDeactivated _ActivateDeactivate = EActivatedDeactivated::None;
    EGivenRevoked _GiveRevoke = EGivenRevoked::None;
#endif

    auto
    DoDebugSet_Activated() -> void;

    auto
    DoDebugSet_Deactivated() -> void;

    auto
    DoDebugSet_Given() -> void;

    auto
    DoDebugSet_Revoked() -> void;

public:
    CK_PROPERTY_GET(_Data);
    CK_PROPERTY_GET(_AbilityHandle);
    CK_PROPERTY_GET(_AbilityOwnerHandle);
};

// --------------------------------------------------------------------------------------------------------------------

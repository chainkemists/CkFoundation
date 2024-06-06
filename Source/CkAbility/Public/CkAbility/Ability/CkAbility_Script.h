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
    OnAbilityNotActivated(
        const FCk_Ability_NotActivated_Info& InInfo) -> void;

    auto
    OnActivateAbility(
        const FCk_Ability_Payload_OnActivate& InOptionalPayload) -> void;

    auto
    OnReactivateAbility(
        const FCk_Ability_Payload_OnActivate& InOptionalPayload) -> void;

    auto
    OnDeactivateAbility() -> void;

    auto
    Get_CanBeGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const -> bool;

    auto
    OnNotGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const -> void;

protected:
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DevelopmentOnly, DisplayName = "OnAbilityNotActivated (DEVELOPMENT ONLY)"))
    void
    DoOnAbilityNotActivated(
        const FCk_Ability_NotActivated_Info& InInfo);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnActivateAbility"))
    void
    DoOnActivateAbility(
        const FCk_Ability_Payload_OnActivate& InActivationPayload);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnReactivateAbility"))
    void
    DoOnReactivateAbility(
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

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnNotGiven"))
    void
    DoOnNotGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const;

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "Get_AdditionalSubAbilities"))
    TArray<UCk_Ability_Script_PDA*>
    DoGet_AdditionalSubAbilities() const;

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
              BlueprintAuthorityOnly,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Spawn Ability Cue",
              meta = (CompactNodeTitle="SpawnCue", HideSelfPin = true))
    void
    DoRequest_SpawnAbilityCue(
        const FCk_AbilityCue_Params& InReplicatedParams,
        FGameplayTag InAbilityCueName);

    UFUNCTION(BlueprintCallable,
              BlueprintCosmetic,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Request Spawn Ability Cue (Local)",
              meta = (CompactNodeTitle="SpawnLocalCue", HideSelfPin = true))
    void
    DoRequest_SpawnAbilityCue_Local(
        const FCk_AbilityCue_Params& InReplicatedParams,
        FGameplayTag InAbilityCueName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Add Task To Deactivate On Ability Revoke",
              meta = (CompactNodeTitle="TaskToDeactivate_OnRevoke", HideSelfPin = true, Keywords = "Register, Track"))
    void
    DoRequest_AddTaskToDeactivateOnRevoke(
        class UBlueprintTaskTemplate* InTask);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Add Task To Deactivate On Ability Deactivate",
              meta = (CompactNodeTitle="TaskToDeactivate_OnDeactivate", HideSelfPin = true, Keywords = "Register, Track"))
    void
    DoRequest_AddTaskToDeactivateOnDeactivate(
        class UBlueprintTaskTemplate* InTask);

private:
    // Non-const by design to avoid errors where this is called on the CDO and the Ability Handle is Invalid
    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Status",
              meta = (CompactNodeTitle="STATUS_ThisAbility", HideSelfPin = true))
    ECk_Ability_Status
    DoGet_Status();

    // Non-const by design to avoid errors where this is called on the CDO and the Ability Handle is Invalid
    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Entity",
              meta = (CompactNodeTitle="AbilityEntity", HideSelfPin = true))
    FCk_Handle_Ability
    DoGet_AbilityEntity();

    // Non-const by design to avoid errors where this is called on the CDO and the Ability Handle is Invalid
    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Owner Entity",
              meta = (CompactNodeTitle="AbilityOwnerEntity", HideSelfPin = true))
    FCk_Handle_AbilityOwner
    DoGet_AbilityOwnerEntity();

public:
    UFUNCTION(BlueprintCallable,
              BlueprintPure = true,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][AbilityScript] Get Ability Config",
              meta = (Keywords = "params, data"))
    const FCk_Ability_Script_Data&
    Get_Data() const;

private:
    UPROPERTY(EditDefaultsOnly,
              meta = (AllowPrivateAccess, ShowOnlyInnerProperties))
    FCk_Ability_Script_Data _Data;

private:
    UPROPERTY(Transient)
    FCk_Handle_Ability _AbilityHandle;

    UPROPERTY(Transient)
    FCk_Handle_AbilityOwner _AbilityOwnerHandle;

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

private:
    TArray<TWeakObjectPtr<UBlueprintTaskTemplate>> _TasksToDeactivateOnRevoke;
    TArray<TWeakObjectPtr<UBlueprintTaskTemplate>> _TasksToDeactivateOnDeactivate;

public:
    CK_PROPERTY_GET(_AbilityHandle);
    CK_PROPERTY_GET(_AbilityOwnerHandle);
};

// --------------------------------------------------------------------------------------------------------------------

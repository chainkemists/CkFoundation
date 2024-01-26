#pragma once

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"
#include "CkCore/Public/CkCore/Format/CkFormat.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkNet/Public/CkNet/CkNet_Common.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include <InstancedStruct.h>

#include "CkAbility_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_ActivationPolicy : uint8
{
    ActivateManually,
    ActivateOnGranted
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_ActivationPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_ActivationRequirementsResult : uint8
{
    RequirementsMet,

    RequirementsMet_ButAlreadyActive,

    // Requirements were not on the Ability itself
    RequirementsNotMet_OnSelf,

    // Requirements were not on the Ability Owner
    RequirementsNotMet_OnOwner,

    // Requirements were not on the Ability Owner and the Ability itself
    RequirementsNotMet_OnOwnerAndSelf
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_ActivationRequirementsResult);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_Status : uint8
{
    NotActive,
    Active
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Status);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_RecyclingPolicy : uint8
{
    // Recycle the existing Ability Script for the next Activation. Instead of creating a new instance of the UObject,
    // we reset all of its properties to the CDO value
    Recycle,

    // Create a new instance of the Ability Script UObject in between activations
    DoNotRecycle
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_RecyclingPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_InstancingPolicy : uint8
{
    // This Ability is never instanced. Anything that executes the Ability is operating on the CDO
    NotInstanced UMETA(DisplayName = "Not Instanced (uses CDO)"),

    // A new instance of the Ability is made every time it is deactivated.
    InstancedPerAbilityActivation
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_InstancingPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationSettings_OnOwner
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings_OnOwner);

private:
    // While this Ability is executing, the owner of the Ability will be granted this set of Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _GrantTagsOnAbilityOwner;

    // The Ability will be cancelled as soon as the AbilityOwner has any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _CancelledByTagsOnAbilityOwner;

    // The Ability can only be activated if the AbilityOwner has all of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _RequiredTagsOnAbilityOwner;

    // The Ability can only be activated if the AbilityOwner does not have any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _BlockedByTagsOnAbilityOwner;

public:
    CK_PROPERTY(_GrantTagsOnAbilityOwner);
    CK_PROPERTY(_CancelledByTagsOnAbilityOwner);
    CK_PROPERTY(_RequiredTagsOnAbilityOwner);
    CK_PROPERTY(_BlockedByTagsOnAbilityOwner);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationSettings_OnSelf
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings_OnSelf);

private:
    // If this Ability is also an AbilityOwner, then this Ability will be cancelled as soon as the Ability has any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _CancelledByTagsOnSelf;

    // If this Ability is also an AbilityOwner, then this Ability can only be activated if the Ability does not have any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _BlockedByTagsOnSelf;

public:
    CK_PROPERTY(_CancelledByTagsOnSelf);
    CK_PROPERTY(_BlockedByTagsOnSelf);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ability_ActivationPolicy _ActivationPolicy = ECk_Ability_ActivationPolicy::ActivateManually;

    // Tags on AbilityOwner that affect this Ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings_OnOwner _ActivationSettingsOnOwner;

    // Tags on Ability itself that affect this Ability (only applicable if Ability itself is an AbilityOwner)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings_OnSelf _ActivationSettingsOnSelf;

public:
    CK_PROPERTY(_ActivationPolicy);
    CK_PROPERTY(_ActivationSettingsOnOwner);
    CK_PROPERTY(_ActivationSettingsOnSelf);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationPayload
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationPayload);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ContextEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, AutoCreateRefTerm="_Data"))
    FInstancedStruct _Data;

public:
    CK_PROPERTY(_ContextEntity);
    CK_PROPERTY(_Data);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_NetworkSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_NetworkSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
     ECk_Net_ReplicationType _ReplicationType = ECk_Net_ReplicationType::LocalAndHost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Net_NetExecutionPolicy _ExecutionPolicy = ECk_Net_NetExecutionPolicy::PreferHost;

public:
    CK_PROPERTY(_ReplicationType);
    CK_PROPERTY(_ExecutionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Script_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Script_Data);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Name",
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Activation",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings _ActivationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Replication",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_NetworkSettings _NetworkSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Advanced",
              meta = (AllowPrivateAccess = true))
    ECk_Ability_InstancingPolicy _InstancingPolicy = ECk_Ability_InstancingPolicy::InstancedPerAbilityActivation;

public:
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY(_ActivationSettings);
    CK_PROPERTY(_NetworkSettings);
    CK_PROPERTY(_InstancingPolicy);

    CK_DEFINE_CONSTRUCTORS(FCk_Ability_Script_Data, _AbilityName);
};

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
    OnGiveAbility() -> void;

    auto
    OnRevokeAbility() -> void;

    auto
    OnActivateAbility(
        const FCk_Ability_ActivationPayload& InActivationPayload) -> void;

    auto
    OnDeactivateAbility() -> void;

protected:
    auto
    PreSave(
        FObjectPreSaveContext InObjectSaveContext) -> void override;

    auto
    PostLoad() -> void override;


protected:
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnActivateAbility"))
    void
    DoOnActivateAbility(
        const FCk_Ability_ActivationPayload& InActivationPayload);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnDeactivateAbility"))
    void
    DoOnDeactivateAbility();

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnGiveAbility"))
    void
    DoOnGiveAbility();

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnRevokeAbility"))
    void
    DoOnRevokeAbility();

private:
    UFUNCTION(CallInEditor, Category = "Ck|Ability|Validation")
    void ValidateAssetData();

private:
    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Activate This Ability",
              meta = (CompactNodeTitle="ACTIVATE_ThisAbility", HideSelfPin = true))
    void
    DoRequest_ActivateAbility(
        FCk_Ability_ActivationPayload InActivationPayload) const;

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Deactivate This Ability",
              meta = (CompactNodeTitle="DEACTIVATE_ThisAbility", HideSelfPin = true))
    void
    DoRequest_DeactivateAbility() const;

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Apply Cost",
              meta = (CompactNodeTitle="ApplyCost", HideSelfPin = true))
    void
    DoRequest_ApplyCost() const;

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Apply Cost On Owner",
              meta = (CompactNodeTitle="ApplyCost_OnOwner", HideSelfPin = true))
    void
    DoRequest_ApplyCost_OnOwner() const;

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Apply Cooldown",
              meta = (CompactNodeTitle="ApplyCooldown", HideSelfPin = true))
    void
    DoRequest_ApplyCooldown() const;

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Apply Cooldown On Owner",
              meta = (CompactNodeTitle="ApplyCooldown_OnOwner", HideSelfPin = true))
    void
    DoRequest_ApplyCooldown_OnOwner() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Track Task",
              meta = (CompactNodeTitle="TRACK_Task", HideSelfPin = true))
    void
    DoRequest_TrackTask(
        UObject* InTask);

    UFUNCTION(BlueprintCallable,
              BlueprintPure = false,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Request Spawn Ability Cue",
              meta = (CompactNodeTitle="SpawnAbilityCue", HideSelfPin = true))
    void
    DoRequest_SpawnAbilityCue(
        const FCk_AbilityCue_Params& InReplicatedParams,
        FGameplayTag InAbilityCueName) const;

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Get Ability Status",
              meta = (CompactNodeTitle="STATUS_ThisAbility", HideSelfPin = true))
    ECk_Ability_Status
    DoGet_Status() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Get Ability Entity",
              meta = (CompactNodeTitle="AbilityEntity", HideSelfPin = true))
    FCk_Handle
    DoGet_AbilityEntity() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[AbilityScript] Get Ability Owner Entity",
              meta = (CompactNodeTitle="AbilityOwnerEntity", HideSelfPin = true))
    FCk_Handle
    DoGet_AbilityOwnerEntity() const;

private:
    UPROPERTY(EditDefaultsOnly,
              meta = (ShowOnlyInnerProperties))
    FCk_Ability_Script_Data _Data;

private:
    UPROPERTY(Transient)
    FCk_Handle _AbilityHandle;

    UPROPERTY(Transient)
    FCk_Handle _AbilityOwnerHandle;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> _Tasks;

public:
    CK_PROPERTY_GET(_Data);
    CK_PROPERTY_GET(_AbilityHandle);
    CK_PROPERTY_GET(_AbilityOwnerHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Fragment_Ability_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Ability_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Script",
              meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSubclassOf<UCk_Ability_Script_PDA> _AbilityScriptClass;

public:
    CK_PROPERTY_GET(_AbilityScriptClass);

public:
    [[nodiscard]]
    auto
    Get_Data() const -> const FCk_Ability_Script_Data&;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Ability_ParamsData, _AbilityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Fragment_MultipleAbility_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleAbility_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Ability_ParamsData> _AbilityParams;

public:
    CK_PROPERTY_GET(_AbilityParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleAbility_ParamsData, _AbilityParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_ConstructionScript_PDA : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_ConstructionScript_PDA);

public:
    auto DoConstruct_Implementation(
        const FCk_Handle& InHandle) -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (ExposeOnSpawn, AllowPrivateAccess = true))
    FCk_Fragment_Ability_ParamsData _AbilityParams;

public:
    CK_PROPERTY_GET(_AbilityParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_EntityConfig_PDA : public UCk_EntityBridge_Config_Base_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_EntityConfig_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const -> UCk_Entity_ConstructionScript_PDA* override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<UCk_Ability_ConstructionScript_PDA> _EntityConstructionScript;

public:
    CK_PROPERTY_GET(_EntityConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Ability_OnEnded,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Ability_OnEnded_MC,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnActivated,
    FCk_Handle, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnActivated_MC,
    FCk_Handle, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated,
    FCk_Handle, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated_MC,
    FCk_Handle, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------
// Formatters and IsValid

CK_DEFINE_CUSTOM_IS_VALID(FCk_Fragment_Ability_ParamsData, ck::IsValid_Policy_Default,
[=](const FCk_Fragment_Ability_ParamsData& InAbilityParams)
{
    if (ck::Is_NOT_Valid(InAbilityParams.Get_AbilityScriptClass()) ||
        ck::Is_NOT_Valid(InAbilityParams.Get_Data().Get_AbilityName()))
    { return false; }

    return true;
});

// --------------------------------------------------------------------------------------------------------------------

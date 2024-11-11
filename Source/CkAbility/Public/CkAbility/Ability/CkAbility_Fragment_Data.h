#pragma once


#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"
#include "CkCore/Public/CkCore/Format/CkFormat.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Net/CkNet_Common.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include <InstancedStruct.h>
#include <NativeGameplayTags.h>

#include "CkAbility_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_Cue_Interface : public UInterface { GENERATED_BODY() };
class CKABILITY_API ICk_Ability_Cue_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_Cue_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_Cost_Interface : public UInterface { GENERATED_BODY() };
class CKABILITY_API ICk_Ability_Cost_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_Cost_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_Cooldown_Interface : public UInterface { GENERATED_BODY() };
class CKABILITY_API ICk_Ability_Cooldown_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_Cooldown_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_Condition_Interface : public UInterface { GENERATED_BODY() };
class CKABILITY_API ICk_Ability_Condition_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_Condition_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_Activation_Policy : uint8
{
    ActivateManually,
    ActivateOnGranted
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Activation_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_Reactivation_Policy : uint8
{
    OnlyActivateIfCurrentlyDeactivated,
    AllowActivationIfAlreadyActive UMETA(DisplayName="Allow Reactivation (Deactivate First)"),
    AllowActivationIfAlreadyActiveDoNotDeactivate UMETA(DisplayName="Allow Reactivation (Do NOT Deactivate First)"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Reactivation_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_ActivationRequirementsResult : uint8
{
    RequirementsMet,

    RequirementsMet_ButAlreadyActive,

    // Requirements were not met on the Ability itself
    RequirementsNotMet_OnSelf,

    // Requirements were not met on the Ability Owner
    RequirementsNotMet_OnOwner,

    // Requirements were not met on the Ability Owner and the Ability itself
    RequirementsNotMet_OnOwnerAndSelf,

    // Requirements were not met because the Ability is blocked by the Owner
    RequirementsNotMet_BlockedByOwner
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
    InstancedPerAbilityActivation,

    // A new instance of the Ability is made on granted. Each Activation uses the same instance.
    InstancedOnce UMETA(DisplayName = "Instanced Once (when Granted)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_InstancingPolicy);


// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_FeatureReplication_Policy : uint8
{
    // Ability may still be Given/Activated on Client/Server but do NOT replicate the Features of the Ability (e.g. Attributes)
    DoNotReplicateAbilityFeatures,

    // Ability may still be Given/Activated on Client/Server but DO replicate the Features of the Ability (e.g. Attributes)
    ReplicateAbilityFeatures,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_FeatureReplication_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKABILITY_API FCk_Handle_Ability : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Ability); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Ability);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_OnGiveSettings_OnOwner
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_OnGiveSettings_OnOwner);

private:
    // When this Ability is given, the owner of the Ability will be granted this set of Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _GrantTagsOnAbilityOwner;

public:
    CK_PROPERTY(_GrantTagsOnAbilityOwner);
};

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
struct CKABILITY_API FCk_Ability_OnGiveSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_OnGiveSettings);

private:
    // Tags on AbilityOwner that affect this Ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_OnGiveSettings_OnOwner _OnGiveSettingsOnOwner;

public:
    CK_PROPERTY(_OnGiveSettingsOnOwner);
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
    ECk_Ability_Activation_Policy _ActivationPolicy = ECk_Ability_Activation_Policy::ActivateManually;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ability_Reactivation_Policy _ReactivationPolicy = ECk_Ability_Reactivation_Policy::OnlyActivateIfCurrentlyDeactivated;

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
    CK_PROPERTY(_ReactivationPolicy);
    CK_PROPERTY(_ActivationSettingsOnOwner);
    CK_PROPERTY(_ActivationSettingsOnSelf);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_NotActivated_Info
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_NotActivated_Info);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ability_ActivationRequirementsResult _ActivationRequirementResult = ECk_Ability_ActivationRequirementsResult::RequirementsMet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActiveTagsOnOwner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActiveTagsOnSelf;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActivationTriggers;

public:
    CK_PROPERTY_GET(_ActivationRequirementResult);
    CK_PROPERTY_GET(_ActiveTagsOnOwner);
    CK_PROPERTY_GET(_ActiveTagsOnSelf);
    CK_PROPERTY_GET(_ActivationTriggers);

public:
    FCk_Ability_NotActivated_Info() = default;
    FCk_Ability_NotActivated_Info(
        const FCk_Handle_Ability& InAbility,
        ECk_Ability_ActivationRequirementsResult InActivationRequirementsResult);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Payload_OnActivate
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Payload_OnActivate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ContextEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FInstancedStruct _Data;

public:
    CK_PROPERTY(_ContextEntity);
    CK_PROPERTY(_Data);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Payload_OnGranted
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Payload_OnGranted);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _ContextEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
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
        meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition="_ReplicationType == ECk_Net_ReplicationType::LocalAndHost"))
    ECk_Net_NetExecutionPolicy _ExecutionPolicy = ECk_Net_NetExecutionPolicy::PreferHost;

    /* An Ability that is marked 'LocalAndHost' OR 'All' does not necessarily require replication to work. Set this to Replicate ONLY
     if your Ability has Features that require Replication (e.g. Replicated Attributes) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay,
              meta = (AllowPrivateAccess = true, EditConditionHides,
                  EditCondition="_ReplicationType == ECk_Net_ReplicationType::LocalAndHost || _ReplicationType == ECk_Net_ReplicationType::All"))
    ECk_Ability_FeatureReplication_Policy _FeatureReplicationPolicy = ECk_Ability_FeatureReplication_Policy::DoNotReplicateAbilityFeatures;

public:
    CK_PROPERTY(_ReplicationType);
    CK_PROPERTY(_ExecutionPolicy);
    CK_PROPERTY(_FeatureReplicationPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ConditionSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ConditionSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        DisplayName = "Blocking Abilities",
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Condition_Interface"))
    TArray<TSubclassOf<class UCk_Ability_Script_PDA>> _ConditionAbilities;

public:
    CK_PROPERTY(_ConditionAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ConditionSettings_Instanced
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ConditionSettings_Instanced);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
        DisplayName = "Blocking Abilities",
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Condition_Interface"))
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _ConditionAbilities;

public:
    CK_PROPERTY(_ConditionAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_CooldownSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_CooldownSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Cooldown_Interface"))
    TArray<TSubclassOf<class UCk_Ability_Script_PDA>> _CooldownAbilities;

public:
    CK_PROPERTY(_CooldownAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_CooldownSettings_Instanced
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_CooldownSettings_Instanced);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Cooldown_Interface"))
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _CooldownAbilities;

public:
    CK_PROPERTY(_CooldownAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_OtherAbilitySettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_OtherAbilitySettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, AllowAbstract = false,
                DisallowedClasses = "Ck_Ability_Condition_Interface, Ck_Ability_Cost_Interface, Ck_Ability_Cooldown_Interface, Ck_Ability_Cue_Interface, Ck_Ability_Effect_Interface"))
    TArray<TSubclassOf<class UCk_Ability_Script_PDA>> _OtherAbilities;

public:
    CK_PROPERTY(_OtherAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_OtherAbilitySettings_Instanced
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_OtherAbilitySettings_Instanced);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
        meta = (AllowPrivateAccess = true, AllowAbstract = false,
                DisallowedClasses = "Ck_Ability_Condition_Interface, Ck_Ability_Cost_Interface, Ck_Ability_Cooldown_Interface, Ck_Ability_Cue_Interface, Ck_Ability_Effect_Interface"))
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _OtherAbilities;

public:
    CK_PROPERTY(_OtherAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_CostSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_CostSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Cost_Interface"))
    TArray<TSubclassOf<class UCk_Ability_Script_PDA>> _CostAbilities;

public:
    CK_PROPERTY(_CostAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_CostSettings_Instanced
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_CostSettings_Instanced);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Cost_Interface"))
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _CostAbilities;

public:
    CK_PROPERTY(_CostAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Script_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Script_Data);

public:
    friend class UCk_Utils_Ability_UE;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Name",
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Name",
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasDisplayName = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Name",
              meta = (AllowPrivateAccess = true, EditCondition = "_HasDisplayName"))
    FName _DisplayName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "OnGive",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_OnGiveSettings _OnGiveSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Activation",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings _ActivationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Blocked By",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ConditionSettings _ConditionSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities_(Instanced)",
              DisplayName = "Blocked By",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ConditionSettings_Instanced _ConditionSettings_Instanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Costs",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_CostSettings _CostSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities_(Instanced)",
              DisplayName = "Costs",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_CostSettings_Instanced _CostSettings_Instanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Cooldowns",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_CooldownSettings _CooldownSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities_(Instanced)",
              DisplayName = "Cooldowns",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_CooldownSettings_Instanced _CooldownSettings_Instanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Other",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_OtherAbilitySettings _OtherAbilitySettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities_(Instanced)",
              DisplayName = "Other",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_OtherAbilitySettings_Instanced _OtherAbilitySettings_Instanced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Replication",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_NetworkSettings _NetworkSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Advanced",
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideAbilityConstructionScript = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Advanced",
              DisplayName = "Overriden Ability Construction Script",
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideAbilityConstructionScript", AllowAbstract = false))
    TSubclassOf<class UCk_Ability_ConstructionScript_PDA> _AbilityConstructionScript;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Advanced",
              meta = (AllowPrivateAccess = true))
    ECk_Ability_InstancingPolicy _InstancingPolicy = ECk_Ability_InstancingPolicy::InstancedPerAbilityActivation;

public:
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY(_HasDisplayName);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_OnGiveSettings);
    CK_PROPERTY(_ActivationSettings);
    CK_PROPERTY(_ConditionSettings);
    CK_PROPERTY(_ConditionSettings_Instanced);
    CK_PROPERTY(_OtherAbilitySettings);
    CK_PROPERTY(_OtherAbilitySettings_Instanced);
    CK_PROPERTY(_CostSettings);
    CK_PROPERTY(_CostSettings_Instanced);
    CK_PROPERTY(_CooldownSettings);
    CK_PROPERTY(_CooldownSettings_Instanced);
    CK_PROPERTY(_NetworkSettings);
    CK_PROPERTY(_InstancingPolicy);
    CK_PROPERTY(_OverrideAbilityConstructionScript);
    CK_PROPERTY(_AbilityConstructionScript);

    CK_DEFINE_CONSTRUCTORS(FCk_Ability_Script_Data, _AbilityName);
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
    TSubclassOf<class UCk_Ability_Script_PDA> _AbilityScriptClass;

    UPROPERTY(Transient)
    TObjectPtr<UCk_Ability_Script_PDA> _AbilityArchetype;

public:
    CK_PROPERTY_GET(_AbilityScriptClass);
    CK_PROPERTY(_AbilityArchetype);

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
        meta = (AllowPrivateAccess = true, TitleProperty = "_AbilityScriptClass"))
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
    friend class UCk_Utils_Ability_UE;

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const -> void override;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_Ability_Script_PDA>> _DefaultAbilities_Instanced;

    UPROPERTY(Transient)
    FCk_Fragment_Ability_ParamsData _AbilityParams;

public:
    CK_PROPERTY(_DefaultAbilities_Instanced);
    CK_PROPERTY(_AbilityParams);
};

// --------------------------------------------------------------------------------------------------------------------

// NOTE: Class is NOT Abstract by design since we need to create an instance of it inside UCk_Utils_Ability_UE::Create_AbilityEntityConfig
UCLASS(EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_EntityConfig_PDA : public UCk_EntityBridge_Config_Base_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_EntityConfig_PDA);

public:
    friend class UCk_Utils_Ability_UE;

private:
    auto
    DoGet_EntityConstructionScript() const -> UCk_Entity_ConstructionScript_PDA* override;

protected:
#if WITH_EDITOR
    auto IsDataValid(
        class FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<UCk_Ability_ConstructionScript_PDA> _EntityConstructionScript;

public:
    CK_PROPERTY_GET(_EntityConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Ability_OnActivated,
    FCk_Handle_Ability, InAbilityHandle,
    const FCk_Ability_Payload_OnActivate&, InActivationPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Ability_OnActivated_MC,
    FCk_Handle_Ability, InAbilityHandle,
    const FCk_Ability_Payload_OnActivate&, InActivationPayload);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated,
    FCk_Handle_Ability, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated_MC,
    FCk_Handle_Ability, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------

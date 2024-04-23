#pragma once

#include "NativeGameplayTags.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"
#include "CkAttribute/CkAttribute_Fragment_Data.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkCore/Public/CkCore/Format/CkFormat.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Net/CkNet_Common.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include <InstancedStruct.h>

#include "CkAbility_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_Effect_Interface : public UInterface { GENERATED_BODY() };
class CKABILITY_API ICk_Ability_Effect_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_Effect_Interface);
};

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
enum class ECk_Ability_ActivationTrigger_Policy : uint8
{
    // Attempt to Activate the Ability based on AbilityEvents received by the Ability Owner
    TriggeredByEvent,

    // Attempt to Activate the Ability any time tags are added or removed
    TriggeredAutomatically
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_ActivationTrigger_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_EventsForwarding_Policy : uint8
{
    // Forward all the ability events received
    All,

    // Forward all ability events received except specific ones
    AllExcept,

    // Only forward specific ability events received
    Specific
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_EventsForwarding_Policy);

// --------------------------------------------------------------------------------------------------------------------

// How this Ability stacks with other instances of this same Ability
UENUM(BlueprintType)
enum class ECk_Ability_Stacking_Policy : uint8
{
    DoesNotStack,

    // Each caster has it's own stack
    Stack_AggregateBySource UMETA(DisplayName = "Aggregate By Source"),

    // Each recipient has it's own stack
    Stack_AggregateByTarget UMETA(DisplayName = "Aggregate By Target"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Stacking_Policy);

// --------------------------------------------------------------------------------------------------------------------

// How this Ability stacks with other instances of this same Ability
UENUM(BlueprintType)
enum class ECk_Ability_CueStacking_Policy : uint8
{
    // Each new application of a stacking ability will spawn its associated Ability Cue(s)
    SpawnCueForEachStack,

    // If true, AbilityCues will only be triggered for the first instance in a stacking Ability
    SuppressStackingCues,
};

// --------------------------------------------------------------------------------------------------------------------

// How the Ability duration should be refreshed or not) while stacking
UENUM(BlueprintType)
enum class ECk_Ability_StackDurationRefresh_Policy : uint8
{
    // The duration of this Ability will be refreshed from any stack application, even if unsuccessful
    RefreshOnAnyApplication,

    // The duration of this Ability will be refreshed from any successful stack application
    RefreshOnSuccessfulApplication,

    // The duration of this Ability will never be refreshed
    NeverRefresh,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_StackDurationRefresh_Policy);

// --------------------------------------------------------------------------------------------------------------------

// How the Ability period should be reset (or not) while stacking
UENUM(BlueprintType)
enum class ECk_Ability_StackPeriodReset_Policy : uint8
{
    // Any progress toward the next tick of a periodic application is discarded upon any stack application, even if unsuccessful
    ResetOnAnyApplication,

    // Any progress toward the next tick of a periodic application is discarded upon any successful stack application
    ResetOnSuccessfulApplication,

    // The progress toward the next tick of a periodic application will never be reset, regardless of stack applications
    NeverReset,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_StackPeriodReset_Policy);

// --------------------------------------------------------------------------------------------------------------------

// How the Ability stack should be cleared (or not) when it overflows
UENUM(BlueprintType)
enum class ECk_Ability_StackOverflow_Policy : uint8
{
    DoNothing,

    // The entire Ability stack will be cleared once it overflows
    ClearEntireStackOnOverflow
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_StackOverflow_Policy);

// --------------------------------------------------------------------------------------------------------------------

// How to handle duration expiring on this Ability
UENUM(BlueprintType)
enum class ECk_Ability_StackExpiration_Policy : uint8
{
    // The entire stack is cleared when the active Ability expires
    ClearEntireStack,

    // The current stack count will decrement by 1 and the duration is refreshed
    RemoveSingleStackAndRefreshDuration,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_StackExpiration_Policy);

// --------------------------------------------------------------------------------------------------------------------

// How to handle requirements not being met on this Ability
UENUM(BlueprintType)
enum class ECk_Ability_ActivationRequirementsNotMet_Policy
{
    // Any ongoing duration & period timers are left ticking
    DoNothing,

    // Period timer (if valid) is reset and paused. Resumed once the requirements are met again.
    // Does NOT trigger if the requirements were previously met already
    PauseAndResetPeriod,

    // Period timer (if valid) is reset and paused. Resumed once the requirements are met.
    // Triggers event if the requirements were previously met already
    PauseAndResetPeriodAlways
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_Duration_Policy : uint8
{
    Instant,
    Duration,
    Infinite
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Duration_Policy);

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
    InstancedPerAbilityActivation,

    // A new instance of the Ability is made on granted. Each Activation uses the same instance.
    InstancedOnce
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_InstancingPolicy);

// --------------------------------------------------------------------------------------------------------------------

// Type of calculation to perform to derive the magnitude
UENUM(BlueprintType)
enum class ECk_Ability_Modifier_MagnitudeCalculation_Policy : uint8
{
    DiscreteValue,

    // Perform a calculation based upon an Attribute
    AttributeBased,

    // The magnitude will be set explicitly by the source that creates/apply this ability
    SetByCaller,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Modifier_MagnitudeCalculation_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKABILITY_API FCk_Handle_Ability : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Ability); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Ability);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Modifier_Magnitude
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Modifier_Magnitude);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ability_Modifier_MagnitudeCalculation_Policy _MagnitudeCalculation = ECk_Ability_Modifier_MagnitudeCalculation_Policy::DiscreteValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::AttributeBased"))
    ECk_SourceOrTarget _AttributeSource = ECk_SourceOrTarget::Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::AttributeBased"))
    FCk_FloatAttribute_Magnitude _AttributeBasedMagnitude;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::AttributeBased"))
    bool _Snapshot = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::DiscreteValue"))
    float _DiscreteValueMagnitude;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::SetByCaller"))
    FGameplayTag _SetByCallerMagnitude_DataTag;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Modifier
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Modifier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AttributeToModify;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MinMaxCurrent _AttributeComponent = ECk_MinMaxCurrent::Current;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AttributeModifier_Operation _ModifierOperation = ECk_AttributeModifier_Operation::Add;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_Modifier_Magnitude _ModifierMagnitude;
};

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
        meta = (AllowPrivateAccess = true))
    ECk_Net_NetExecutionPolicy _ExecutionPolicy = ECk_Net_NetExecutionPolicy::PreferHost;

public:
    CK_PROPERTY(_ReplicationType);
    CK_PROPERTY(_ExecutionPolicy);
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
        meta = (AllowPrivateAccess = true, AllowAbstract = false, MustImplement = "/Script/CkAbility.Ck_Ability_Condition_Interface"))
     TArray<TSubclassOf<class UCk_Ability_Script_PDA>> _ConditionAbilities;

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
              Category = "Name",
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasDisplayName = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Name",
              meta = (AllowPrivateAccess = true, EditCondition = "_HasDisplayName"))
    FName _DisplayName;

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
              DisplayName = "Conditions",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ConditionSettings _ConditionSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Costs",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_CostSettings _CostSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Cooldowns",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_CooldownSettings _CooldownSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "SubAbilities",
              DisplayName = "Other",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_OtherAbilitySettings _OtherAbilitySettings;

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
    CK_PROPERTY(_OtherAbilitySettings);
    CK_PROPERTY(_CostSettings);
    CK_PROPERTY(_CooldownSettings);
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
    friend class UCk_Utils_Ability_UE;

public:
    auto DoConstruct_Implementation(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const -> void override;

private:
    UPROPERTY(Transient)
    TArray<TSubclassOf<UCk_Ability_Script_PDA>> _DefaultAbilities;

    UPROPERTY(Transient)
    FCk_Fragment_Ability_ParamsData _AbilityParams;

public:
    CK_PROPERTY(_DefaultAbilities);
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

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<UCk_Ability_ConstructionScript_PDA> _EntityConstructionScript;

public:
    CK_PROPERTY_GET(_EntityConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnActivated,
    FCk_Handle_Ability, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnActivated_MC,
    FCk_Handle_Ability, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated,
    FCk_Handle_Ability, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated_MC,
    FCk_Handle_Ability, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------

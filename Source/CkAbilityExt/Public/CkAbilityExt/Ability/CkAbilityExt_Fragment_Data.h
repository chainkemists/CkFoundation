#pragma once

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

#include "CkAbilityExt_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_TraitCondition_Interface : public UInterface { GENERATED_BODY() };
class CKABILITYEXT_API ICk_Ability_TraitCondition_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_TraitCondition_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_TraitPayoff_Interface : public UInterface { GENERATED_BODY() };
class CKABILITYEXT_API ICk_Ability_TraitPayoff_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_TraitPayoff_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_Ability_Effect_Interface : public UInterface { GENERATED_BODY() };
class CKABILITYEXT_API ICk_Ability_Effect_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_Ability_Effect_Interface);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_ActivationTrigger_Policy : uint8
{
    // Attempt to Activate the Ability based on AbilityEvents received by the Ability Owner
    TriggeredByEvent,

    // Attempt to Activate the Ability any time tags are added or removed
    TriggeredAutomatically,

    // Attempt to Activate the Ability based on AbilityEvents received by the Ability Owner OR any time tags are added or removed
    TriggeredByEventOrAutomatically
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

UENUM(BlueprintType)
enum class ECk_Ability_StackLimit_Policy : uint8
{
    HasStackLimit,
    NoStackLimit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_StackLimit_Policy);

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

// How to handle requirements not being met on this Ability
UENUM(BlueprintType)
enum class ECk_Ability_ActivationRequirementsNotMet_Policy : uint8
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

USTRUCT(BlueprintType)
struct CKABILITYEXT_API FCk_Ability_Modifier_Magnitude
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
    float _DiscreteValueMagnitude = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::SetByCaller", Categories = "FloatAttribute"))
    FGameplayTag _SetByCallerMagnitude_DataTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
          meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MagnitudeCalculation == ECk_Ability_Modifier_MagnitudeCalculation_Policy::SetByCaller"))
    float _SetByCallerMagnitude_DefaultValue = 0.f;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITYEXT_API FCk_Ability_Modifier
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Modifier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "FloatAttribute"))
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
struct CKABILITYEXT_API FCk_Ability_TraitPayoff_Instances
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_TraitPayoff_Instances);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
        meta = (AllowPrivateAccess = true, AllowAbstract = false,
                MustImplement = "/Script/CkAbilityExt.Ck_Ability_TraitPayoff_Interface"))
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _Payoffs;

public:
    CK_PROPERTY_GET(_Payoffs);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITYEXT_API FCk_Ability_TraitCondition_Instances
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_TraitCondition_Instances);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
        meta = (AllowPrivateAccess = true, AllowAbstract = false,
                MustImplement = "/Script/CkAbilityExt.Ck_Ability_TraitCondition_Interface"))
    TArray<TObjectPtr<class UCk_Ability_Script_PDA>> _Conditions;

public:
    CK_PROPERTY_GET(_Conditions);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITYEXT_API FCk_Ability_Effect_CueData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Effect_CueData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FGameplayTag _AbilityName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TSubclassOf<UCk_Ability_Script_PDA> _AbilityClass;

public:
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY_GET(_AbilityClass);
};

// --------------------------------------------------------------------------------------------------------------------
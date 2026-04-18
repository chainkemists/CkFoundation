#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap_Fragment_Data.generated.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;
class UCk_GoapGoal_EntityScript;

// ====================================================================================================================
// HANDLES
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap);

// ====================================================================================================================
// PLAN STATUS
// ====================================================================================================================

UENUM(BlueprintType)
enum class ECk_GoapPlanStatus : uint8
{
	Idle,
	Planning,
	PlanFound,
	PlanFailed,
	CostThresholdReached
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GoapPlanStatus);

// ====================================================================================================================
// SIGNAL PAYLOADS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlanComplete
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnPlanComplete);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _TotalCost = 0.0f;

public:
	CK_PROPERTY_GET(_Actions);
	CK_PROPERTY_GET(_TotalCost);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Payload_OnPlanComplete, _Actions, _TotalCost);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlanFailed
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnPlanFailed);
};

// ====================================================================================================================
// DELEGATES
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlanComplete,
	FCk_Handle_Goap, InHandle,
	FCk_Goap_Payload_OnPlanComplete, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlanFailed,
	FCk_Handle_Goap, InHandle,
	FCk_Goap_Payload_OnPlanFailed, InPayload);

// ====================================================================================================================
// DIAGNOSTICS
// ====================================================================================================================

// A (tag, required-value) pair — describes a goal condition or an
// unreachable requirement in diagnostic output.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_GoapDiagnostic_ConditionPair
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_GoapDiagnostic_ConditionPair);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_GoapDiagnostic_ConditionPair, _Key, _Value);
};

// A strongly-connected component in the action dependency graph. Every action
// listed transitively depends on every other via an effect→precondition chain,
// so none of their effects can be produced from empty state. If the starting
// world state doesn't seed at least one condition in the cycle, actions inside
// it are unreachable.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_GoapDiagnostic_DependencyCycle
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_GoapDiagnostic_DependencyCycle);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _ActionsInCycle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TArray<FGameplayTag> _CycleConditions;

public:
	CK_PROPERTY_GET(_ActionsInCycle);
	CK_PROPERTY_GET(_CycleConditions);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_GoapDiagnostic_DependencyCycle, _ActionsInCycle, _CycleConditions);
};

// ====================================================================================================================
// REQUESTS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Plan
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_Plan);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TSubclassOf<UCk_GoapGoal_EntityScript> _SpecificGoalClass;

public:
	CK_PROPERTY(_SpecificGoalClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetWorldState
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetWorldState);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetWorldState, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_CancelPlan
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_CancelPlan);
};

// ====================================================================================================================

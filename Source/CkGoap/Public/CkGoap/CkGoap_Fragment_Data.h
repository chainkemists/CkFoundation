#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap_Fragment_Data.generated.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;

// ====================================================================================================================
// HANDLE — the Bundle/Tier root container. One per gameplay entity; holds
// FFragment_RecordOfGoapActionSets in its fragment set.
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
// REPLAN POLICY
// ====================================================================================================================

UENUM(BlueprintType)
enum class ECk_Goap_ReplanPolicy : uint8
{
	// Re-plan only when consumer explicitly calls Request_Plan.
	Explicit,

	// Re-plan when any registered WS key value changes.
	OnWorldStateDirty,

	// Re-plan when any action's cost changes.
	OnCostDirty,

	// Re-plan on either trigger.
	OnEitherDirty
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Goap_ReplanPolicy);

// ====================================================================================================================
// AUTHORED CONDITION — BlueprintType-friendly (tag, bool) pair for declaring
// goal / initial-state entries from editor or AngelScript. The Setup processor
// resolves these into the internal FCk_GoapKey-indexed form against the
// WorldState registry.
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_GoapWS_Condition_Authored
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_GoapWS_Condition_Authored);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap"))
	FGameplayTag _Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_GoapWS_Condition_Authored, _Key, _Value);
};

// ====================================================================================================================
// ROOT PARAMS — placeholder for future global tuning. Empty in v1.
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_RootParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_RootParamsData);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — reused by tier-scoped signals declared in Tier/CkGoap_Tier_Fragment_Data.h
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

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlanFailed
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnPlanFailed);
};

// ====================================================================================================================
// DIAGNOSTICS
// ====================================================================================================================

// A (tag, required-value) pair — describes a goal condition or an unreachable
// requirement in diagnostic output.
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
// listed transitively depends on every other via an effect→precondition chain.
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

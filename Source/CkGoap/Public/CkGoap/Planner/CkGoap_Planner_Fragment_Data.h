#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

// Need FCk_Handle_Goap_Action for the OnActiveChainChanged payload's TArray<>.
// Also pulls FCk_GoapWS_Condition_Authored for the Planner's _Goal field.
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapWS_Condition_Authored

#include "CkGoap_Planner_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLE
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Planner : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Planner);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Planner);

// ====================================================================================================================
// ACTIONSET PARAMS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_PlannerParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_PlannerParamsData);

private:
	// ActionSet identity within a Goap root. Unique per root entity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap.Planner"))
	FGameplayTag _PlannerTag;

	// Initial enable/disable state. Disabled ActionSets skip per-Action
	// planning and chain-update. Toggle at runtime via Request_SetEnableToggle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_EnableDisable _InitialToggle = ECk_EnableDisable::Enable;

	// U11.1: the goal this Planner plans toward. Independent of any Action role
	// effects this entity may carry. May be empty at construction and set later
	// via Request_SetGoal. Stamped onto the Planner's FFragment_Goap_Planner_Goal
	// at construction (and propagated to the root Action's planner-role goal via
	// SetRootAction).
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_GoapWS_Condition_Authored> _Goal;

public:
	CK_PROPERTY(_PlannerTag);
	CK_PROPERTY(_InitialToggle);
	CK_PROPERTY(_Goal);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Goap_PlannerParamsData, _PlannerTag);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — ActionSet-scoped
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnActiveChainChanged
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnActiveChainChanged);

private:
	// Snapshot of the chain BEFORE this frame's mutation. Current chain is
	// readable via UCk_Utils_Goap_Planner_UE::Get_ActiveChain in the handler.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_Handle_Goap_Action> _OldChain;

public:
	CK_PROPERTY_GET(_OldChain);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Payload_OnActiveChainChanged, _OldChain);
};

// ====================================================================================================================
// DELEGATES — ActionSet-scoped
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnActiveChainChanged,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnActiveChainChanged, InPayload);


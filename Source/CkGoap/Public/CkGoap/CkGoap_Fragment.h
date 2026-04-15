#pragma once

#include "CkGoap_Fragment_Data.h"

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/Algorithm/CkGoap_Types.h"
#include "CkGoap/Algorithm/CkGoap_Graph.h"

#include "CkAStar/CkAStar_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// TAGS
// ====================================================================================================================

CK_DEFINE_ECS_TAG(FTag_Goap_RequiresSetup);
CK_DEFINE_ECS_TAG(FTag_Goap_PlanRequested);

// ====================================================================================================================
// WORLD STATE FRAGMENT
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_WorldState
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_HandleRequests;

private:
	goap::FWorldState _WorldState;

public:
	CK_PROPERTY_GET(_WorldState);
};

// ====================================================================================================================
// ACTION CLASSES FRAGMENT — Registered action EntityScript classes (set during Add)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_ActionClasses
{
public:
	CK_GENERATED_BODY(FFragment_Goap_ActionClasses);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_Setup;

private:
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Classes;

public:
	CK_PROPERTY_GET(_Classes);
};

// ====================================================================================================================
// ACTIONS FRAGMENT — Extracted lightweight action data (populated by setup processor from CDOs)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Actions
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Actions);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_Setup;
	friend class FProcessor_Goap_HandleRequests;

private:
	TArray<goap::FActionDef> _ActionDefs;

public:
	CK_PROPERTY_GET(_ActionDefs);
};

// ====================================================================================================================
// GOAL CLASSES FRAGMENT — Registered goal EntityScript classes (set during Add)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_GoalClasses
{
public:
	CK_GENERATED_BODY(FFragment_Goap_GoalClasses);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_Setup;

private:
	TArray<TSubclassOf<UCk_GoapGoal_EntityScript>> _Classes;

public:
	CK_PROPERTY_GET(_Classes);
};

// ====================================================================================================================
// GOALS FRAGMENT — Extracted lightweight goal data (populated by setup processor from CDOs)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Goals
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Goals);

	friend class FProcessor_Goap_Setup;
	friend class FProcessor_Goap_HandleRequests;

private:
	TArray<goap::FGoalDef> _GoalDefs;

public:
	CK_PROPERTY_GET(_GoalDefs);
};

// ====================================================================================================================
// CURRENT FRAGMENT — Plan status, active goal, plan result
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Current
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Current);

	friend class FProcessor_Goap_HandleRequests;
	friend class FProcessor_Goap_HandleResult;

private:
	ECk_GoapPlanStatus _PlanStatus = ECk_GoapPlanStatus::Idle;
	TSubclassOf<UCk_GoapGoal_EntityScript> _ActiveGoalClass;
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Plan;
	float _PlanCost = 0.0f;

public:
	CK_PROPERTY_GET(_PlanStatus);
	CK_PROPERTY_GET(_ActiveGoalClass);
	CK_PROPERTY_GET(_Plan);
	CK_PROPERTY_GET(_PlanCost);
};

// ====================================================================================================================
// REQUESTS FRAGMENT — Request queue
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Requests
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Requests);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_HandleRequests;

	using RequestType = std::variant<
		FCk_Request_Goap_Plan,
		FCk_Request_Goap_SetWorldState,
		FCk_Request_Goap_CancelPlan>;

private:
	TArray<RequestType> _Requests;

public:
	CK_PROPERTY_GET(_Requests);
};

// ====================================================================================================================
// PLAN CONTEXT FRAGMENT — Holds graph reference for plan extraction after search completes
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_PlanContext
{
public:
	CK_GENERATED_BODY(FFragment_Goap_PlanContext);

	friend class FProcessor_Goap_HandleRequests;
	friend class FProcessor_Goap_HandleResult;

private:
	goap::FGoapGraph _Graph;

public:
	CK_PROPERTY_GET(_Graph);
};

// ====================================================================================================================
// TEMPLATE INSTANTIATIONS — Concrete A* fragment types for GOAP
// ====================================================================================================================

using FFragment_Goap_SearchState = TFragment_AStar_SearchState<int32, goap::FGoapGraph>;
using FFragment_Goap_Result = TFragment_AStar_Result<int32>;

// ====================================================================================================================
// SIGNALS
// ====================================================================================================================

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
	CKGOAP_API,
	OnGoapPlanComplete,
	FCk_Delegate_Goap_OnPlanComplete,
	FCk_Handle_Goap,
	FCk_Goap_Payload_OnPlanComplete);

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
	CKGOAP_API,
	OnGoapPlanFailed,
	FCk_Delegate_Goap_OnPlanFailed,
	FCk_Handle_Goap,
	FCk_Goap_Payload_OnPlanFailed);

// ====================================================================================================================

} // namespace ck

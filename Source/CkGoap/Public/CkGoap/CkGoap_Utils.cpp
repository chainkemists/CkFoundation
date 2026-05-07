#include "CkGoap_Utils.h"

#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/EntityScripts/CkGoapGoal_EntityScript.h"

#include "CkGoap/CkGoap_Fragment.h"
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// ====================================================================================================================
// RECORD — owner's record of GOAP planner children, populated by Create.
// ====================================================================================================================
//
// Defined in this .cpp (not the public Fragment.h) on purpose: declaring the record in a public
// header would drag CkRecord_Fragment.h into every CkGoap consumer, which forces a transitive
// reference to FCk_Handle_EntityExtension (CkRecord publicly defines a record of those for
// layering reasons) and breaks link in dependents that don't list CkEntityExtension. Only this
// translation unit ever touches the record — processors don't need it.

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfGoapPlanners, FCk_Handle_Goap);
}

namespace
{
	struct FRecordOfGoapPlanners_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapPlanners> {};
}

// ====================================================================================================================
// CREATION
// ====================================================================================================================

namespace
{
	auto
		DoStampGoapFragments(
			FCk_Handle& InTargetEntity,
			const FCk_Fragment_Goap_ParamsData& InParams)
		-> void
	{
		InTargetEntity.Add<ck::FTag_Goap_RequiresSetup>();
		InTargetEntity.Add<ck::FFragment_Goap_KeyRegistry>();
		InTargetEntity.Add<ck::FFragment_Goap_WorldState>();
		InTargetEntity.Add<ck::FFragment_Goap_Current>();
		InTargetEntity.Add<ck::FFragment_Goap_ActionClasses>();
		InTargetEntity.Add<ck::FFragment_Goap_Actions>();
		InTargetEntity.Add<ck::FFragment_Goap_GoalClasses>();
		InTargetEntity.Add<ck::FFragment_Goap_Goals>();
		InTargetEntity.Add<ck::FFragment_Goap_SearchState>();
		InTargetEntity.Add<ck::FFragment_Goap_Result>();
		InTargetEntity.Add<ck::FFragment_Goap_PlanContext>();
		InTargetEntity.Add<ck::FFragment_Goap_Diagnostics>();
		InTargetEntity.Add<ck::FFragment_Goap_ReplanThrottle>();
		InTargetEntity.Add<ck::FFragment_Goap_Params>(InParams);

		if (InParams.Get_PlanOnStart())
		{
			InTargetEntity.Add<ck::FTag_Goap_RequiresInitialPlan>();
		}

		// A* params mirror the GOAP params. Consumers who need to tune A* knobs
		// not surfaced via GOAP (e.g. MaxIterationsPerTick) can still reach the
		// FFragment_AStar_Params directly — but the common knobs (budget + cost
		// threshold) flow from GOAP params through this wiring.
		auto AStarParams = ck::FFragment_AStar_Params{};
		AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
		AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
		InTargetEntity.Add<ck::FFragment_AStar_Params>(AStarParams);
		InTargetEntity.Add<ck::FFragment_AStar_Debug>();
	}
}

auto
	UCk_Utils_Goap_UE::
	Add(
		FCk_Handle& InOwner,
		const FCk_Fragment_Goap_ParamsData& InParams)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when adding GOAP planner"))
	{ return {}; }

	// Add stamps GOAP fragments DIRECTLY on InOwner — the owner *is* the planner.
	// Calling Add twice on the same owner would obliterate live plan state and
	// corrupt the AStar search; bail with an ensure instead. Callers that need
	// multiple planners (or that already have a standalone AStar feature on
	// InOwner) must use Create.
	CK_ENSURE_IF_NOT(NOT Has(InOwner),
		TEXT("Owner [{}] already has a GOAP planner. Use Create for additional named planners."),
		InOwner)
	{ return Cast(InOwner); }

	DoStampGoapFragments(InOwner, InParams);

	return Cast(InOwner);
}

auto
	UCk_Utils_Goap_UE::
	Create(
		FCk_Handle& InOwner,
		FGameplayTag InName,
		const FCk_Fragment_Goap_ParamsData& InParams)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when creating GOAP planner"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InName.IsValid(),
		TEXT("Invalid name passed to Create for GOAP planner under owner [{}]"),
		InOwner)
	{ return {}; }

	auto GoapEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap>(InOwner);

	UCk_Utils_Handle_UE::Set_DebugName(GoapEntity, InName.GetTagName());

	DoStampGoapFragments(GoapEntity, InParams);

	UCk_Utils_GameplayLabel_UE::Add(GoapEntity, InName);

	// Register the planner in the owner's RecordOfGoapPlanners so Find_Goap /
	// Find_GoapByName can locate it later. DisallowDuplicateNames enforces that
	// two Create calls with the same InName under the same owner are an error
	// (the framework's record system will refuse the second connect).
	FRecordOfGoapPlanners_Utils::AddIfMissing(InOwner,
		ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
	FRecordOfGoapPlanners_Utils::Request_Connect(InOwner, GoapEntity);

	return GoapEntity;
}

// ====================================================================================================================
// ACTION & GOAL REGISTRATION
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	AddAction(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle when adding action"))
	{ return InGoap; }
	CK_ENSURE_IF_NOT(ck::IsValid(InActionClass), TEXT("Invalid action class when adding to GOAP"))
	{ return InGoap; }

	auto& ActionClasses = InGoap.Get<ck::FFragment_Goap_ActionClasses>();
	ActionClasses._Classes.AddUnique(InActionClass);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	AddGoal(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapGoal_EntityScript> InGoalClass)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle when adding goal"))
	{ return InGoap; }
	CK_ENSURE_IF_NOT(ck::IsValid(InGoalClass), TEXT("Invalid goal class when adding to GOAP"))
	{ return InGoap; }

	auto& GoalClasses = InGoap.Get<ck::FFragment_Goap_GoalClasses>();
	GoalClasses._Classes.AddUnique(InGoalClass);
	return InGoap;
}

// ====================================================================================================================
// WORLD STATE
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Set_WorldStateValue(FCk_Handle_Goap& InGoap, FGameplayTag InKey, bool InValue)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetWorldState{InKey, InValue});
}

auto
	UCk_Utils_Goap_UE::
	Get_WorldStateValue(const FCk_Handle_Goap& InGoap, FGameplayTag InKey)
	-> bool
{
	if (NOT ck::IsValid(InGoap)) { return false; }
	if (NOT InGoap.Has<ck::FFragment_Goap_KeyRegistry>()) { return false; }

	const auto& Registry = InGoap.Get<ck::FFragment_Goap_KeyRegistry>().Get_Registry();
	const auto Key = Registry.Find(InKey);
	if (Key == ck::goap::InvalidGoapKey) { return false; }

	const auto& WS = InGoap.Get<ck::FFragment_Goap_WorldState>().Get_WorldState();
	return WS.Get(Key);
}

auto
	UCk_Utils_Goap_UE::
	Has_WorldStateKey(const FCk_Handle_Goap& InGoap, FGameplayTag InKey)
	-> bool
{
	if (NOT ck::IsValid(InGoap)) { return false; }
	if (NOT InGoap.Has<ck::FFragment_Goap_KeyRegistry>()) { return false; }
	const auto& Registry = InGoap.Get<ck::FFragment_Goap_KeyRegistry>().Get_Registry();
	return Registry.Find(InKey) != ck::goap::InvalidGoapKey;
}

// ====================================================================================================================
// DYNAMIC COST UPDATE
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Set_ActionCost(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InNewCost)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetActionCost{InActionClass, InNewCost});
}

auto
	UCk_Utils_Goap_UE::
	Get_ActionCost(
		const FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass)
	-> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Get_ActionCost"))
	{ return 0.0f; }

	const auto& Actions = InGoap.Get<ck::FFragment_Goap_Actions>();
	for (const auto& ActionDef : Actions.Get_ActionDefs())
	{
		if (ActionDef.ActionClass == InActionClass) { return ActionDef.Cost; }
	}
	return 0.0f;
}

// ====================================================================================================================
// PLANNING
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Request_Plan(FCk_Handle_Goap& InGoap)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_Plan{});
}

auto
	UCk_Utils_Goap_UE::
	Request_PlanForGoal(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapGoal_EntityScript> InGoalClass)
	-> FCk_Handle_Goap
{
	auto Request = FCk_Request_Goap_Plan{};
	Request.Set_SpecificGoalClass(InGoalClass);
	return DoAddRequest(InGoap, Request);
}

auto
	UCk_Utils_Goap_UE::
	Request_CancelPlan(FCk_Handle_Goap& InGoap)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_CancelPlan{});
}

// ====================================================================================================================
// RUNTIME TUNING
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Request_SetReplanInterval(
		FCk_Handle_Goap& InGoap,
		float InMinReplanIntervalSeconds)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetReplanInterval{InMinReplanIntervalSeconds});
}

auto
	UCk_Utils_Goap_UE::
	Request_SetReplanPolicy(
		FCk_Handle_Goap& InGoap,
		ECk_Goap_ReplanPolicy InPolicy)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetReplanPolicy{InPolicy});
}

auto
	UCk_Utils_Goap_UE::
	Request_SetSearchBudget(
		FCk_Handle_Goap& InGoap,
		int64 InSearchBudgetMicroseconds)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetSearchBudget{InSearchBudgetMicroseconds});
}

auto
	UCk_Utils_Goap_UE::
	Request_SetCostThreshold(
		FCk_Handle_Goap& InGoap,
		float InCostThreshold)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetCostThreshold{InCostThreshold});
}

// ====================================================================================================================
// QUERY
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Has(const FCk_Handle& InHandle)
	-> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Current>();
}

auto
	UCk_Utils_Goap_UE::
	Find_Goap(const FCk_Handle& InHandle)
	-> FCk_Handle_Goap
{
	if (NOT ck::IsValid(InHandle))
	{ return {}; }

	// Self path — InHandle IS the GOAP planner (Add-on-owner case). Cast and return.
	if (InHandle.Has<ck::FFragment_Goap_Current>())
	{ return CastChecked(InHandle); }

	// Owner path — InHandle owns one or more GOAP children via Create(). Walk the
	// record and return the first valid entry. For multi-planner owners callers
	// should use Find_GoapByName so the lookup is unambiguous.
	if (NOT FRecordOfGoapPlanners_Utils::Has(InHandle))
	{ return {}; }

	const auto Entries = FRecordOfGoapPlanners_Utils::Get_ValidEntries(InHandle);
	if (Entries.IsEmpty())
	{ return {}; }

	return Entries[0];
}

auto
	UCk_Utils_Goap_UE::
	Find_GoapByName(const FCk_Handle& InHandle, FGameplayTag InName)
	-> FCk_Handle_Goap
{
	if (NOT ck::IsValid(InHandle))
	{ return {}; }

	if (NOT FRecordOfGoapPlanners_Utils::Has(InHandle))
	{ return {}; }

	return FRecordOfGoapPlanners_Utils::Get_ValidEntry_ByTag(InHandle, InName);
}

auto
	UCk_Utils_Goap_UE::
	Get_PlanStatus(const FCk_Handle_Goap& InGoap)
	-> ECk_GoapPlanStatus
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Get_PlanStatus"))
	{ return ECk_GoapPlanStatus::Idle; }
	return InGoap.Get<ck::FFragment_Goap_Current>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_UE::
	Get_Plan(const FCk_Handle_Goap& InGoap)
	-> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Get_Plan"))
	{ return {}; }
	return InGoap.Get<ck::FFragment_Goap_Current>().Get_Plan();
}

auto
	UCk_Utils_Goap_UE::
	Get_PlanCost(const FCk_Handle_Goap& InGoap)
	-> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Get_PlanCost"))
	{ return 0.0f; }
	return InGoap.Get<ck::FFragment_Goap_Current>().Get_PlanCost();
}

// ====================================================================================================================
// SIGNAL BINDING
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	BindTo_OnPlanComplete(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoapPlanComplete, InGoap, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	UnbindFrom_OnPlanComplete(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoapPlanComplete, InGoap, InDelegate);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	BindTo_OnPlanFailed(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoapPlanFailed, InGoap, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	UnbindFrom_OnPlanFailed(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoapPlanFailed, InGoap, InDelegate);
	return InGoap;
}

// ====================================================================================================================
// DIAGNOSTICS
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Get_DependencyCycles(const FCk_Handle_Goap& InGoap)
	-> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Get_DependencyCycles"))
	{ return {}; }
	if (NOT InGoap.Has<ck::FFragment_Goap_Diagnostics>()) { return {}; }
	return InGoap.Get<ck::FFragment_Goap_Diagnostics>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_UE::
	Get_LastUnreachableGoalConditions(const FCk_Handle_Goap& InGoap)
	-> TArray<FCk_GoapDiagnostic_ConditionPair>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Get_LastUnreachableGoalConditions"))
	{ return {}; }
	if (NOT InGoap.Has<ck::FFragment_Goap_Diagnostics>()) { return {}; }
	return InGoap.Get<ck::FFragment_Goap_Diagnostics>().Get_LastUnreachableGoalConditions();
}

auto
	UCk_Utils_Goap_UE::
	Has_DiagnosticWarnings(const FCk_Handle_Goap& InGoap)
	-> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle in Has_DiagnosticWarnings"))
	{ return false; }
	if (NOT InGoap.Has<ck::FFragment_Goap_Diagnostics>()) { return false; }
	const auto& Diag = InGoap.Get<ck::FFragment_Goap_Diagnostics>();
	return Diag.Get_DependencyCycles().Num() > 0
		|| Diag.Get_LastUnreachableGoalConditions().Num() > 0;
}

// ====================================================================================================================
// CAST
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	DoCast(FCk_Handle& InHandle, ECk_SucceededFailed& OutResult)
	-> FCk_Handle_Goap
{
	if (Has(InHandle))
	{
		OutResult = ECk_SucceededFailed::Succeeded;
		return Cast(InHandle);
	}
	OutResult = ECk_SucceededFailed::Failed;
	return {};
}

auto
	UCk_Utils_Goap_UE::
	DoCastChecked(FCk_Handle InHandle)
	-> FCk_Handle_Goap
{
	return CastChecked(InHandle);
}

// ====================================================================================================================
// INTERNALS
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	DoAddRequest(FCk_Handle_Goap& InGoap, const auto& InRequest)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap), TEXT("Invalid GOAP handle when adding request"))
	{ return InGoap; }

	auto& Requests = InGoap.AddOrGet<ck::FFragment_Goap_Requests>();
	Requests._Requests.Add(InRequest);
	return InGoap;
}

// ====================================================================================================================

#include "CkGoap/Tier/CkGoap_Tier_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Bundle/CkGoap_Bundle_Fragment.h"
#include "CkGoap/Tier/CkGoap_Tier_Fragment.h"
#include "CkGoap/Tier/CkGoap_Tier_Record_Internal.h"  // FFragment_RecordOfGoapTiers + utils struct
#include "CkGoap/Bundle/CkGoap_Bundle_Utils.h"  // Find_Tier (uniqueness check)
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"  // Request_AddSubscriber
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================
// CONSTRUCTION
// ====================================================================================================================

auto
	UCk_Utils_Goap_Tier_UE::
	AddTier(
		FCk_Handle_Goap_Bundle& InBundle,
		const FCk_Fragment_Goap_TierParamsData& InParams)
	-> FCk_Handle_Goap_Tier
{
	CK_ENSURE_IF_NOT(ck::IsValid(InBundle),
		TEXT("Invalid bundle handle when adding tier"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_TierTag().IsValid(),
		TEXT("Tier params has invalid _TierTag (bundle [{}])"), InBundle)
	{ return {}; }

	// Diagnostic: tier-tag uniqueness within bundle.
	if (auto Existing = UCk_Utils_Goap_Bundle_UE::Find_Tier(InBundle, InParams.Get_TierTag());
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("Tier with tag [{}] already exists in bundle [{}]; AddTier rejected."),
			InParams.Get_TierTag(), InBundle);
		return {};
	}

	auto TierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Tier>(InBundle);

	// Records of tiers require GameplayLabels — label the tier with its
	// declared tag.
	UCk_Utils_GameplayLabel_UE::Add(TierEntity, InParams.Get_TierTag());

	TierEntity.Add<ck::FFragment_Goap_Tier_Params>(InParams);
	TierEntity.Add<ck::FFragment_Goap_Tier_Current>();
	TierEntity.Add<ck::FFragment_Goap_Tier_ActionClasses>();
	TierEntity.Add<ck::FFragment_Goap_Tier_Actions>();
	TierEntity.Add<ck::FFragment_Goap_Tier_Requests>();

	auto& Throttle = TierEntity.AddOrGet<ck::FFragment_Goap_Tier_ReplanThrottle>();
	(void)Throttle;  // throttle's interval is read from params at Setup time

	TierEntity.Add<ck::FFragment_Goap_Tier_SearchState>();
	TierEntity.Add<ck::FFragment_Goap_Tier_Result>();
	TierEntity.Add<ck::FFragment_Goap_Tier_PlanContext>();

	// A* params mirror the tier's planning knobs.
	auto AStarParams = ck::FFragment_AStar_Params{};
	AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
	AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
	TierEntity.Add<ck::FFragment_AStar_Params>(AStarParams);
	TierEntity.Add<ck::FFragment_AStar_Debug>();

	// Mark for one-shot setup.
	TierEntity.AddOrGet<ck::FTag_Goap_Tier_RequiresSetup>();

	if (InParams.Get_PlanOnStart())
	{
		TierEntity.AddOrGet<ck::FTag_Goap_Tier_RequiresInitialPlan>();
	}

	// Register in the bundle's catalog record + tag→tier index.
	ck::goap::internal_tier::FRecordOfGoapTiers_Utils::AddIfMissing(InBundle);
	ck::goap::internal_tier::FRecordOfGoapTiers_Utils::Request_Connect(InBundle, TierEntity);

	auto& Index = InBundle.Get<ck::FFragment_Goap_Bundle_TierCatalogIndex>();
	Index._TagToTier.Add(InParams.Get_TierTag(), TierEntity);

	// Catalog mutated → re-run bundle setup (cycle detection).
	InBundle.AddOrGet<ck::FTag_Goap_Bundle_RequiresSetup>();

	// First AddTier on a bundle = the root tier. Seed the active chain.
	auto& ActiveTiers = InBundle.Get<ck::FFragment_Goap_Bundle_ActiveTiers>();
	if (ActiveTiers._Tiers.IsEmpty())
	{
		// Validate root has a WS override (no parent to inherit from).
		if (NOT ck::IsValid(InParams.Get_WorldStateSource_Override()))
		{
			ck::goap::Warning(
				TEXT("Root tier [{}] in bundle [{}] has no _WorldStateSource_Override; planning will not run until one is set."),
				InParams.Get_TierTag(), InBundle);
		}
		else
		{
			// Resolve WS source synchronously for the root.
			auto& Current = TierEntity.Get<ck::FFragment_Goap_Tier_Current>();
			Current._WorldStateSource_Resolved = InParams.Get_WorldStateSource_Override();

			// Subscribe root tier to its WS so value-changes flip the dirty
			// tag and AutoReplan fires. Non-root tiers get this hook-up in
			// the bundle ChainUpdate processor at activation time.
			auto WS = Current._WorldStateSource_Resolved;
			UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, TierEntity);
		}

		ActiveTiers._Tiers.Add(TierEntity);
	}

	return TierEntity;
}

auto
	UCk_Utils_Goap_Tier_UE::
	AddAction(
		FCk_Handle_Goap_Tier& InTier,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass) -> FCk_Handle_Goap_Tier
{
	CK_ENSURE_IF_NOT(ck::IsValid(InTier),
		TEXT("Invalid tier handle when adding action"))
	{ return InTier; }

	CK_ENSURE_IF_NOT(ck::IsValid(InActionClass),
		TEXT("Invalid action class on tier [{}]"), InTier)
	{ return InTier; }

	auto& Classes = InTier.Get<ck::FFragment_Goap_Tier_ActionClasses>();
	Classes._Classes.AddUnique(InActionClass);

	// Mark for re-setup so the new action's CDO gets extracted.
	InTier.AddOrGet<ck::FTag_Goap_Tier_RequiresSetup>();

	return InTier;
}

// ====================================================================================================================
// QUERY
// ====================================================================================================================

auto
	UCk_Utils_Goap_Tier_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Tier_Params>();
}

auto
	UCk_Utils_Goap_Tier_UE::
	Get_PlanStatus(const FCk_Handle_Goap_Tier& InTier) -> ECk_GoapPlanStatus
{
	if (NOT ck::IsValid(InTier)) { return ECk_GoapPlanStatus::Idle; }
	return InTier.Get<ck::FFragment_Goap_Tier_Current>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_Tier_UE::
	Get_Plan(const FCk_Handle_Goap_Tier& InTier) -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	if (NOT ck::IsValid(InTier)) { return {}; }
	return InTier.Get<ck::FFragment_Goap_Tier_Current>().Get_Plan();
}

auto
	UCk_Utils_Goap_Tier_UE::
	Get_PlanCost(const FCk_Handle_Goap_Tier& InTier) -> float
{
	if (NOT ck::IsValid(InTier)) { return 0.0f; }
	return InTier.Get<ck::FFragment_Goap_Tier_Current>().Get_PlanCost();
}

auto
	UCk_Utils_Goap_Tier_UE::
	Get_WorldStateSource(const FCk_Handle_Goap_Tier& InTier) -> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InTier)) { return {}; }
	return InTier.Get<ck::FFragment_Goap_Tier_Current>().Get_WorldStateSource_Resolved();
}

auto
	UCk_Utils_Goap_Tier_UE::
	Get_ActiveParentAction(const FCk_Handle_Goap_Tier& InTier) -> TSubclassOf<UCk_GoapAction_EntityScript>
{
	if (NOT ck::IsValid(InTier)) { return nullptr; }
	return InTier.Get<ck::FFragment_Goap_Tier_Current>().Get_ActiveParentAction();
}

auto
	UCk_Utils_Goap_Tier_UE::
	Get_InvalidGoal(const FCk_Handle_Goap_Tier& InTier) -> TArray<FCk_GoapWS_Condition_Authored>
{
	if (NOT ck::IsValid(InTier)) { return {}; }
	return InTier.Get<ck::FFragment_Goap_Tier_Current>().Get_InvalidGoal();
}

// ====================================================================================================================
// REQUESTS — append to per-tier request queue, processors drain.
// Inlined per-verb (UCk_Utils_Goap_Tier_UE is friended to access _Requests,
// but a free helper in another namespace isn't).
// ====================================================================================================================

auto
	UCk_Utils_Goap_Tier_UE::
	Request_SetGoalWorldState(
		FCk_Handle_Goap_Tier& InTier,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_SetGoal{InGoal});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_Plan(FCk_Handle_Goap_Tier& InTier) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_Plan{});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_CancelPlan(FCk_Handle_Goap_Tier& InTier) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_CancelPlan{});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_SetActionCost(
		FCk_Handle_Goap_Tier& InTier,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InCost) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_SetActionCost{InActionClass, InCost});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_SetReplanInterval(FCk_Handle_Goap_Tier& InTier, float InSeconds) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_SetReplanInterval{InSeconds});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_SetReplanPolicy(FCk_Handle_Goap_Tier& InTier, ECk_Goap_ReplanPolicy InPolicy) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_SetReplanPolicy{InPolicy});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_SetSearchBudget(FCk_Handle_Goap_Tier& InTier, int64 InMicroseconds) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_SetSearchBudget{InMicroseconds});
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	Request_SetCostThreshold(FCk_Handle_Goap_Tier& InTier, float InThreshold) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InTier)) { return InTier; }
	auto& Reqs = InTier.AddOrGet<ck::FFragment_Goap_Tier_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Tier_SetCostThreshold{InThreshold});
	return InTier;
}

// ====================================================================================================================
// SIGNAL BINDING
// ====================================================================================================================

auto
	UCk_Utils_Goap_Tier_UE::
	BindTo_OnPlanComplete(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Tier_PlanComplete,
		InTier, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	UnbindFrom_OnPlanComplete(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanComplete& InDelegate) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Tier_PlanComplete, InTier, InDelegate);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	BindTo_OnPlanFailed(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Tier_PlanFailed,
		InTier, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	UnbindFrom_OnPlanFailed(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanFailed& InDelegate) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Tier_PlanFailed, InTier, InDelegate);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	BindTo_OnTierActivated(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Tier_Activated,
		InTier, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	UnbindFrom_OnTierActivated(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierActivated& InDelegate) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Tier_Activated, InTier, InDelegate);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	BindTo_OnTierDeactivated(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Tier_Deactivated,
		InTier, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InTier;
}

auto
	UCk_Utils_Goap_Tier_UE::
	UnbindFrom_OnTierDeactivated(
		FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierDeactivated& InDelegate) -> FCk_Handle_Goap_Tier
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Tier_Deactivated, InTier, InDelegate);
	return InTier;
}

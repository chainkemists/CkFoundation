#include "CkGoap/Planner/CkGoap_Planner_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Processor.h"
#include "CkGoap/Planner/CkGoap_Planner_Record_Internal.h"
#include "CkGoap/Planner/CkGoap_Planner_Internal.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Utils.h"
#include "CkGoap/Action/CkGoap_Action_Record_Internal.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::goap::internal_planner::
    DoCreateOrFindActionEntity(
        FCk_Handle_Goap_Planner& InPlanner,
        const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
        TEXT("Invalid ActionSet handle in DoCreateOrFindActionEntity"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
        TEXT("Invalid _ActionClass in DoCreateOrFindActionEntity (ActionSet [{}])"), InPlanner)
    { return {}; }

    const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InParams.Get_ActionClass());
    CK_ENSURE_IF_NOT(ActionTag.IsValid(),
        TEXT("Could not derive a valid action tag for class [{}]"), InParams.Get_ActionClass())
    { return {}; }

    if (auto Existing = UCk_Utils_Goap_Planner_UE::Find_Action(InPlanner, ActionTag);
        ck::IsValid(Existing))
    {
        ck::goap::Warning(
            TEXT("Action with tag [{}] (class [{}]) already exists in ActionSet [{}]; returning existing handle."),
            ActionTag, InParams.Get_ActionClass(), InPlanner);
        return Existing;
    }

    auto ActionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Action>(InPlanner);

    // Records of Actions require a GameplayLabel.
    UCk_Utils_GameplayLabel_UE::Add(ActionEntity, ActionTag);

    ActionEntity.Add<ck::FFragment_Goap_Action_Params>(InParams);
    ActionEntity.Add<ck::FFragment_Goap_Action_Current>();
    ActionEntity.Add<ck::FFragment_Goap_Action_Definition>();
    ActionEntity.Add<ck::FFragment_Goap_Action_Tree>();

    // Even a lean atomic Action needs this Planner-role fragment: FProcessor_Goap_Action_Setup
    // resolves its preconditions/effects against the WS registry it points at.
    ActionEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>();

    ActionEntity.AddOrGet<ck::FTag_Goap_Action_RequiresSetup>();

    ck::goap::internal_action::FRecordOfGoapActions_Utils::AddIfMissing(InPlanner);
    ck::goap::internal_action::FRecordOfGoapActions_Utils::Request_Connect(InPlanner, ActionEntity);

    auto& Index = InPlanner.Get<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
    Index.AddEntry(ActionTag, ActionEntity);

    // Catalog mutated → re-run Planner setup (cycle detection).
    InPlanner.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

    return ActionEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Planner_UE::
	DoStampTopLevelPlannerRole(
		FCk_Handle& InPlannerEntity,
		const FCk_Fragment_Goap_PlannerParamsData& InParams)
	-> void
{
	InPlannerEntity.Add<ck::FFragment_Goap_Planner_Params>(InParams);
	InPlannerEntity.Add<ck::FFragment_Goap_Planner_Current>();

	auto& Current = InPlannerEntity.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	InPlannerEntity.Add<ck::FFragment_Goap_Planner_ActionCatalogIndex>();

	// A top-level Planner must supply a WS source, and _Resolved is seeded here (rather than at
	// activation, which only ever runs for promoted mid-tier Planners) so Setup and HandleRequests
	// can read it on the very first tick.
	{
		auto WSFrag = ck::FFragment_Goap_Planner_WorldStateSource{};
		WSFrag._WorldStateSource = InParams.Get_WorldStateSource();
		WSFrag._Resolved          = InParams.Get_WorldStateSource();
		InPlannerEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>(WSFrag);
	}

	InPlannerEntity.Add<ck::FFragment_Goap_Planner_PlanState>();
	{
		auto GoalFrag = ck::FFragment_Goap_Planner_Goal{};
		GoalFrag._GoalAuthored = InParams.Get_Goal();
		InPlannerEntity.Add<ck::FFragment_Goap_Planner_Goal>(GoalFrag);
	}

	// Top-level Planners are active by construction — no parent exists to activate them.
	{
		auto ActivationFrag = ck::FFragment_Goap_Planner_Activation{};
		ActivationFrag._IsActive = true;
		InPlannerEntity.Add<ck::FFragment_Goap_Planner_Activation>(ActivationFrag);
	}

	// Seeded from PlannerParams — otherwise the A* budget falls back to FFragment_AStar_Params'
	// own 500us default instead of the Planner's 50000us.
	{
		auto AStarParams = ck::FFragment_AStar_Params{};
		AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
		AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
		InPlannerEntity.Add<ck::FFragment_AStar_Params>(AStarParams);
	}
	InPlannerEntity.Add<ck::FFragment_AStar_Debug>();

	// These Planner aliases resolve to the Action-side types (CkGoap_Planner_Fragment.h); a Planner
	// and an Action are distinct entities, so each carries its own copy.
	InPlannerEntity.Add<ck::FFragment_Goap_Planner_SearchState>();
	InPlannerEntity.Add<ck::FFragment_Goap_Planner_Result>();
	InPlannerEntity.Add<ck::FFragment_Goap_Planner_PlanContext>();
	InPlannerEntity.Add<ck::FFragment_Goap_Planner_Requests>();
	InPlannerEntity.AddOrGet<ck::FFragment_Goap_Planner_ReplanThrottle>();

	InPlannerEntity.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	// Subscribing is what makes WS-dirty tags land on the Planner for AutoReplan to pick up.
	if (ck::IsValid(InParams.Get_WorldStateSource()))
	{
		auto WS = InParams.Get_WorldStateSource();
		UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, InPlannerEntity, {});
	}

	if (InParams.Get_PlanOnStart())
	{
		InPlannerEntity.AddOrGet<ck::FTag_Goap_Planner_RequiresInitialPlan>();
	}
}

auto
	UCk_Utils_Goap_Planner_UE::
	Add(
		FCk_Handle& InOwner,
		const FCk_Fragment_Goap_PlannerParamsData& InParams)
	-> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when adding Planner"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_PlannerTag().IsValid(),
		TEXT("Planner params has invalid _PlannerTag (owner [{}])"), InOwner)
	{ return {}; }

	if (Has(InOwner))
	{
		ck::goap::Warning(
			TEXT("Owner [{}] already carries the Planner role; Add rejected. Use Create for a second Planner on the same owner."),
			InOwner);
		return {};
	}

	DoStampTopLevelPlannerRole(InOwner, InParams);

	return Cast(InOwner);
}

auto
	UCk_Utils_Goap_Planner_UE::
	Create(
		FCk_Handle& InOwner,
		FGameplayTag InPlannerTag,
		const FCk_Fragment_Goap_PlannerParamsData& InParams)
	-> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when creating Planner"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InPlannerTag.IsValid(),
		TEXT("Invalid _PlannerTag passed to Create (owner [{}])"), InOwner)
	{ return {}; }

	auto ParamsCopy = InParams;
	ParamsCopy.Set_PlannerTag(InPlannerTag);

	if (auto Existing = Find_Planner(InOwner, InPlannerTag);
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("Planner with tag [{}] already exists on owner [{}]; Create rejected."),
			InPlannerTag, InOwner);
		return {};
	}

	auto PlannerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Planner>(InOwner);

	// Records of Planners require a GameplayLabel. Add() keeps InOwner's own label instead.
	UCk_Utils_GameplayLabel_UE::Add(PlannerEntity, InPlannerTag);

	auto PlannerAsGeneric = static_cast<FCk_Handle>(PlannerEntity);
	DoStampTopLevelPlannerRole(PlannerAsGeneric, ParamsCopy);

	ck::goap::internal_planner_record::FRecordOfGoapPlanners_Utils::AddIfMissing(InOwner);
	ck::goap::internal_planner_record::FRecordOfGoapPlanners_Utils::Request_Connect(InOwner, PlannerEntity);

	return PlannerEntity;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Find_Planner(
		const FCk_Handle& InOwner,
		FGameplayTag InPlannerTag) -> FCk_Handle_Goap_Planner
{
	auto Result = FCk_Handle_Goap_Planner{};
	if (NOT ck::IsValid(InOwner)) { return Result; }
	if (NOT InPlannerTag.IsValid()) { return Result; }

	auto MutableOwner = InOwner;
	if (NOT MutableOwner.Has<ck::FFragment_RecordOfGoapPlanners>()) { return Result; }

	ck::goap::internal_planner_record::FRecordOfGoapPlanners_Utils::ForEach_ValidEntry(
		MutableOwner,
		[&](FCk_Handle_Goap_Planner InPlanner)
		{
			if (NOT ck::IsValid(InPlanner)) { return; }
			const auto& Params = InPlanner.Get<ck::FFragment_Goap_Planner_Params>();
			if (Params.Get_PlannerTag() == InPlannerTag)
			{
				Result = InPlanner;
			}
		});

	return Result;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Planner_Params>();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Find_Action(
		const FCk_Handle_Goap_Planner& InPlanner,
		FGameplayTag InActionTag) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	if (NOT InActionTag.IsValid()) { return {}; }

	const auto& Index = InPlanner.Get<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
	const auto* Found = Index.Get_TagToAction().Find(InActionTag);
	return Found ? *Found : FCk_Handle_Goap_Action{};
}

auto
	UCk_Utils_Goap_Planner_UE::
	Find_ActionByClass(
		const FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	if (NOT ck::IsValid(InActionClass)) { return {}; }

	const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InActionClass);
	if (NOT ActionTag.IsValid()) { return {}; }

	return Find_Action(InPlanner, ActionTag);
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_ActiveChain(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_Handle_Goap_Action>
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }

	auto Result = TArray<FCk_Handle_Goap_Action>{};

	const auto& PlannerPlanState = InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>();
	const auto& InitialPlan = PlannerPlanState.Get_Plan();
	if (InitialPlan.IsEmpty()) { return Result; }

	auto Curr = InitialPlan[0];
	if (NOT ck::IsValid(Curr)) { return Result; }

	// A sub-Planner first entry must be active; an atomic Action (no Activation fragment) is
	// always included as the leaf chain step.
	if (Curr.Has<ck::FFragment_Goap_Planner_Activation>())
	{
		const auto& CurrActivation = Curr.Get<ck::FFragment_Goap_Planner_Activation>();
		if (NOT CurrActivation.Get_IsActive()) { return Result; }
	}

	auto Seen = TSet<FCk_Handle_Goap_Action>{};
	Seen.Add(Curr);
	Result.Add(Curr);

	constexpr auto MaxDepth = 64;
	for (auto Depth = 0; Depth < MaxDepth; ++Depth)
	{
		// Only a composite AND active sub-Planner contributes a further step.
		if (NOT Curr.Has<ck::FFragment_Goap_Action_Tree>()) { break; }
		const auto& CurrTree = Curr.Get<ck::FFragment_Goap_Action_Tree>();
		if (CurrTree.Get_ChildActions().IsEmpty()) { break; }

		if (NOT Curr.Has<ck::FFragment_Goap_Planner_Activation>()) { break; }
		const auto& CurrActivation = Curr.Get<ck::FFragment_Goap_Planner_Activation>();
		if (NOT CurrActivation.Get_IsActive()) { break; }

		if (NOT Curr.Has<ck::FFragment_Goap_Planner_PlanState>()) { break; }
		const auto& PlanState = Curr.Get<ck::FFragment_Goap_Planner_PlanState>();
		const auto& Plan = PlanState.Get_Plan();
		if (Plan.IsEmpty()) { break; }

		const auto Next = Plan[0];
		if (NOT ck::IsValid(Next)) { break; }
		if (Seen.Contains(Next)) { break; }

		Seen.Add(Next);
		Result.Add(Next);
		Curr = Next;
	}

	return Result;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_EnableToggle(const FCk_Handle_Goap_Planner& InPlanner) -> ECk_EnableDisable
{
	if (NOT ck::IsValid(InPlanner)) { return ECk_EnableDisable::Disable; }
	return InPlanner.Get<ck::FFragment_Goap_Planner_Current>().Get_EnableToggle();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_DependencyCycles(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	return InPlanner.Get<ck::FFragment_Goap_Planner_Current>().Get_DependencyCycles();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanStatus(const FCk_Handle_Goap_Planner& InPlanner) -> ECk_GoapPlanStatus
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanStatus"))
	{ return ECk_GoapPlanStatus::Idle; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_Plan(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_Handle_Goap_Action>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_Plan"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_Plan();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanClasses(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanClasses"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanClasses();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanCost(const FCk_Handle_Goap_Planner& InPlanner) -> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanCost"))
	{ return 0.0f; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanCost();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanAttemptCount(const FCk_Handle_Goap_Planner& InPlanner) -> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanAttemptCount"))
	{ return 0; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanAttemptCount();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_WorldStateSource(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_WorldStateSource"))
	{ return {}; }

	const auto& WSSource = InPlanner.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
	const auto Resolved = WSSource.Get_Resolved();
	if (ck::IsValid(Resolved)) { return Resolved; }
	return WSSource.Get_WorldStateSource();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_InvalidGoal(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_GoapWS_Condition_Authored>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_InvalidGoal"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Goal>().Get_InvalidGoal();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlannerTag(const FCk_Handle_Goap_Planner& InPlanner) -> FGameplayTag
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlannerTag"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_PlannerTag();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_ReplanPolicy(const FCk_Handle_Goap_Planner& InPlanner) -> ECk_Goap_ReplanPolicy
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_ReplanPolicy"))
	{ return ECk_Goap_ReplanPolicy::Explicit; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_ReplanPolicy();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_MinReplanInterval(const FCk_Handle_Goap_Planner& InPlanner) -> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_MinReplanInterval"))
	{ return 0.0f; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_MinReplanIntervalSeconds();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_SearchBudgetMicroseconds(const FCk_Handle_Goap_Planner& InPlanner) -> int64
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_SearchBudgetMicroseconds"))
	{ return 0; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_SearchBudgetMicroseconds();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_CostThreshold(const FCk_Handle_Goap_Planner& InPlanner) -> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_CostThreshold"))
	{ return 0.0f; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_CostThreshold();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanOnStart(const FCk_Handle_Goap_Planner& InPlanner) -> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanOnStart"))
	{ return false; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_PlanOnStart();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_AllowPlanFailed(const FCk_Handle_Goap_Planner& InPlanner) -> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_AllowPlanFailed"))
	{ return false; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Params>().Get_AllowPlanFailed();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_HasUnconditionalFallback(const FCk_Handle_Goap_Planner& InPlanner) -> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_HasUnconditionalFallback"))
	{ return false; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Current>().Get_HasUnconditionalFallback();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_LastReplanCause(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Goap_ReplanCauseInfo
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_LastReplanCause"))
	{ return {}; }

	if (NOT InPlanner.Has<ck::FFragment_Goap_Planner_ReplanCause>()) { return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_ReplanCause>().Get_Info();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_LastSearchStats(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Goap_SearchStats
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_LastSearchStats"))
	{ return {}; }

	const auto& PlanState = InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>();
	const auto& Result = InPlanner.Get<ck::FFragment_Goap_Planner_Result>();

	const auto StatePoolSize = InPlanner.Has<ck::FFragment_Goap_Planner_PlanContext>()
		? InPlanner.Get<ck::FFragment_Goap_Planner_PlanContext>().Get_Graph().Get_StatePoolSize()
		: 0;

	return FCk_Goap_SearchStats{
		PlanState.Get_PlanStatus(),
		Result._TotalIterations,
		Result._TotalTimeMicroseconds,
		StatePoolSize,
		PlanState.Get_Plan().Num(),
		PlanState.Get_PlanCost()};
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_LastSearchDebug(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_Goap_SearchDebugRow>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_LastSearchDebug"))
	{ return {}; }

	if (NOT InPlanner.Has<ck::FFragment_Goap_Planner_PlanContext>()) { return {}; }

	const auto& Graph = InPlanner.Get<ck::FFragment_Goap_Planner_PlanContext>().Get_Graph();
	const auto PoolSize = Graph.Get_StatePoolSize();
	if (PoolSize <= 0) { return {}; }

	const auto WSSource = Get_WorldStateSource(InPlanner);
	CK_ENSURE_IF_NOT(ck::IsValid(WSSource),
		TEXT("Planner [{}] has search state but no resolved WorldState source"), InPlanner)
	{ return {}; }

	const auto& Registry = WSSource.Get<ck::FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();

	// First edge that discovered each node: unpack (from<<32|to) → to.
	auto ViaActionByNode = TMap<int32, int32>{};
	for (const auto& [EdgeKey, ActionIndex] : Graph.Get_EdgeActions())
	{
		const auto ToNode = static_cast<int32>(static_cast<uint32>(EdgeKey & 0xFFFFFFFF));
		if (NOT ViaActionByNode.Contains(ToNode))
		{ ViaActionByNode.Add(ToNode, ActionIndex); }
	}

	const auto& Actions = Graph.Get_Actions();
	const auto& SeedWorldState = Graph.Get_CurrentWorldState();
	const auto& StatePool = Graph.Get_StatePool();

	auto Rows = TArray<FCk_Goap_SearchDebugRow>{};
	Rows.Reserve(PoolSize);

	for (auto NodeIndex = 0; NodeIndex < PoolSize; ++NodeIndex)
	{
		const auto& ConstraintSet = StatePool[NodeIndex];

		auto Row = FCk_Goap_SearchDebugRow{};

		for (auto Key = 0; Key < Registry.Num(); ++Key)
		{
			const auto Constraint = ConstraintSet.Get(Key);
			if (Constraint == ck::goap::EConstraint::None) { continue; }

			Row._Conditions.Add(FCk_GoapWS_Condition_Authored{
				Registry.GetTag(Key), Constraint == ck::goap::EConstraint::MustBeTrue});
		}

		if (const auto* ViaAction = ViaActionByNode.Find(NodeIndex);
			ViaAction != nullptr && Actions.IsValidIndex(*ViaAction))
		{
			Row._ViaActionClass = Actions[*ViaAction].ActionClass;
		}

		Row._UnsatisfiedCount = ConstraintSet.CountUnsatisfied(SeedWorldState);
		Row._SatisfiedByWorldState = ConstraintSet.IsSatisfiedBy(SeedWorldState);

		Rows.Add(MoveTemp(Row));
	}

	return Rows;
}

// --------------------------------------------------------------------------------------------------------------------

// Befriended on FFragment_Goap_Planner_WorldStateSource so it can write _Resolved directly.
auto
	ck::goap::internal_planner::
	DoResolveChildWorldStateFromParent(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParentAction) -> void
{
	auto& ChildWSSource = InChild.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
	if (ck::IsValid(ChildWSSource.Get_Resolved())) { return; }

	const auto& ChildParams = InChild.Get<ck::FFragment_Goap_Action_Params>();
	const auto Override = ChildParams.Get_WorldStateSource_Override();
	if (ck::IsValid(Override))
	{
		ChildWSSource._Resolved = Override;
		return;
	}

	if (ck::IsValid(InParentAction))
	{
		const auto& ParentWSSource = InParentAction.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
		const auto ParentWS = ParentWSSource.Get_Resolved();
		if (ck::IsValid(ParentWS))
		{
			ChildWSSource._Resolved = ParentWS;
			return;
		}
	}

	auto OwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InChild);
	if (OwnerEntity.Has<ck::FFragment_Goap_Planner_WorldStateSource>())
	{
		const auto& OwnerWS = OwnerEntity.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
		if (ck::IsValid(OwnerWS.Get_Resolved()))
		{
			ChildWSSource._Resolved = OwnerWS.Get_Resolved();
			return;
		}
		if (ck::IsValid(OwnerWS.Get_WorldStateSource()))
		{
			ChildWSSource._Resolved = OwnerWS.Get_WorldStateSource();
		}
	}
}

auto
	UCk_Utils_Goap_Planner_UE::
	AddAction(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in AddAction"))
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
		TEXT("Invalid _ActionClass in AddAction (Planner [{}])"), InPlanner)
	{ return {}; }

	auto ActionEntity = ck::goap::internal_planner::DoCreateOrFindActionEntity(InPlanner, InParams);

	if (NOT ck::IsValid(ActionEntity))
	{ return {}; }

	const auto IsPromotedMidTier = InPlanner.Has<ck::FFragment_Goap_Action_Tree>();

	auto& ChildTree = ActionEntity.Get<ck::FFragment_Goap_Action_Tree>();
	if (ck::IsValid(ChildTree.Get_ParentAction()))
	{
		// DoCreateOrFindActionEntity returned an existing entry — keep its edges intact.
		return ActionEntity;
	}

	if (IsPromotedMidTier)
	{
		auto HostAsAction = UCk_Utils_Goap_Action_UE::CastChecked(InPlanner);
		ChildTree._ParentAction = HostAsAction;

		auto& HostTree = InPlanner.Get<ck::FFragment_Goap_Action_Tree>();
		HostTree._ChildActions.AddUnique(ActionEntity);

		ck::goap::internal_planner::DoResolveChildWorldStateFromParent(ActionEntity, HostAsAction);
		return ActionEntity;
	}

	// A top-level Planner is not an Action, so the child keeps an invalid _ParentAction and the
	// Planner's ActionCatalogIndex is the candidate set.
	{
		auto InvalidParent = FCk_Handle_Goap_Action{};
		ck::goap::internal_planner::DoResolveChildWorldStateFromParent(ActionEntity, InvalidParent);
	}

	return ActionEntity;
}

auto
	UCk_Utils_Goap_Planner_UE::
	PromoteActionToPlanner(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Fragment_Goap_PlannerParamsData& InParams) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InAction),
		TEXT("Invalid Action handle in PromoteActionToPlanner"))
	{ return {}; }

	CK_ENSURE_IF_NOT(UCk_Utils_Goap_Action_UE::Has(InAction),
		TEXT("Handle [{}] does not have the Action role; PromoteActionToPlanner requires an Action entity."), InAction)
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_PlannerTag().IsValid(),
		TEXT("Promote params has invalid _PlannerTag (Action [{}])"), InAction)
	{ return {}; }

	// No-op rather than a re-stamp: re-stamping Params/Current would clobber catalog state.
	if (UCk_Utils_Goap_Planner_UE::Has(InAction))
	{
		ck::goap::Warning(
			TEXT("Handle [{}] is already a Planner; PromoteActionToPlanner is a no-op (returning existing Planner-cast)."), InAction);
		return UCk_Utils_Goap_Planner_UE::Cast(InAction);
	}

	InAction.Add<ck::FFragment_Goap_Planner_Params>(InParams);
	InAction.Add<ck::FFragment_Goap_Planner_Current>();
	InAction.Add<ck::FFragment_Goap_Planner_ActionCatalogIndex>();

	auto& Current = InAction.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	InAction.AddOrGet<ck::FFragment_Goap_Planner_PlanState>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_Activation>();

	// _GoalAuthored comes from InParams, never from the Action role's effects — the Planner's
	// authored goal is independent of them.
	{
		auto& GoalFrag = InAction.AddOrGet<ck::FFragment_Goap_Planner_Goal>();
		GoalFrag._GoalAuthored = InParams.Get_Goal();
		GoalFrag._Goal = {};
		GoalFrag._InvalidGoal = {};
	}

	// Optional: left unset, the promoted Planner inherits the parent's resolved WS at activation.
	if (ck::IsValid(InParams.Get_WorldStateSource()))
	{
		auto& WSFragment = InAction.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
		WSFragment._WorldStateSource = InParams.Get_WorldStateSource();
	}

	// AddOrGet, never Add, for the whole A*-pipeline cluster below: the Planner-side aliases
	// resolve to the same types the host already carries as an Action, so Add would
	// duplicate-assert. PlannerParams' budget/threshold overwrite the ActionParams-seeded ones.
	{
		auto& AStarParams = InAction.AddOrGet<ck::FFragment_AStar_Params>();
		AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
		AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
	}
	InAction.AddOrGet<ck::FFragment_AStar_Debug>();

	InAction.AddOrGet<ck::FFragment_Goap_Planner_SearchState>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_Result>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_PlanContext>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_ReplanThrottle>();

	// Re-run setup so cycle detection and goal resolution see the new Planner-role config.
	InAction.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	return UCk_Utils_Goap_Planner_UE::Cast(InAction);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetEnableToggle(
		FCk_Handle_Goap_Planner& InPlanner,
		ECk_EnableDisable InToggle,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid ActionSet handle in Request_SetEnableToggle"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InToggle;

	// Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
	InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Succeeded);

	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_ResetActiveChain(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid ActionSet handle in Request_ResetActiveChain"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Chain = Get_ActiveChain(InPlanner);
	if (Chain.Num() > 0)
	{
		// Leaf → outermost. An atomic leaf Action is included in the chain as the
		// execution step but carries no Planner role — nothing to deactivate on it.
		for (auto i = Chain.Num() - 1; i >= 0; --i)
		{
			auto Action = Chain[i];
			if (NOT ck::IsValid(Action)) { continue; }
			if (NOT Action.Has<ck::FFragment_Goap_Planner_Activation>()) { continue; }
			ck::FProcessor_Goap_Planner_UpdateActivation::DoDeactivatePlanner(Action);
		}

		// The cache still points at the old Chain[0]; leaving it would make the next UpdateActivation
		// tick no-op on OldStep0 == NewStep0 instead of seeing a fresh transition.
		auto& PlannerActivation = InPlanner.Get<ck::FFragment_Goap_Planner_Activation>();
		PlannerActivation._LastActivatedPlan0 = {};
	}

	// Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
	InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Succeeded);

	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetGoal(
		FCk_Handle_Goap_Planner& InPlanner,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_SetGoal"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_SetGoal{InGoal};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Requests = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Requests._Requests.Add(Request);
	return InPlanner;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Planner_UE::
	Request_Plan(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_Plan"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_Plan{};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_CancelPlan(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_CancelPlan"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_CancelPlan{};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetReplanInterval(
		FCk_Handle_Goap_Planner& InPlanner,
		float InSeconds,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_SetReplanInterval"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_SetReplanInterval{InSeconds};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetReplanPolicy(
		FCk_Handle_Goap_Planner& InPlanner,
		ECk_Goap_ReplanPolicy InPolicy,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_SetReplanPolicy"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_SetReplanPolicy{InPolicy};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetSearchBudget(
		FCk_Handle_Goap_Planner& InPlanner,
		int64 InMicroseconds,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_SetSearchBudget"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_SetSearchBudget{InMicroseconds};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetCostThreshold(
		FCk_Handle_Goap_Planner& InPlanner,
		float InThreshold,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_SetCostThreshold"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_SetCostThreshold{InThreshold};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetChildActionCost(
		FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass,
		float InCost,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_SetChildActionCost"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_SetActionCost{InChildClass, InCost};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_RegisterActionCostProvider(
		FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_RegisterActionCostProvider"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto Request = FCk_Request_Goap_Planner_RegisterActionCostProvider{InChildClass};

	if (InDelegate.IsBound())
	{ Request.Set_CompletionDelegate(InDelegate); }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(Request);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_RemoveAction(
		FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildActionClass,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_Planner
{
	const auto PlannerIsValid = ck::IsValid(InPlanner);
	CK_ENSURE_IF_NOT(PlannerIsValid,
		TEXT("Invalid Planner handle in Request_RemoveAction"))
	{}
	if (NOT PlannerIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto ChildActionClassIsValid = ck::IsValid(InChildActionClass);
	CK_ENSURE_IF_NOT(ChildActionClassIsValid,
		TEXT("Invalid ChildActionClass in Request_RemoveAction (Planner [{}])"), InPlanner)
	{}
	if (NOT ChildActionClassIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InChildActionClass);
	const auto ActionTagIsValid = ActionTag.IsValid();
	CK_ENSURE_IF_NOT(ActionTagIsValid,
		TEXT("Could not derive a valid action tag for class [{}] in Request_RemoveAction (Planner [{}])"),
		InChildActionClass, InPlanner)
	{}
	if (NOT ActionTagIsValid)
	{
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	auto ChildAction = Find_Action(InPlanner, ActionTag);
	if (NOT ck::IsValid(ChildAction))
	{
		ck::goap::Warning(
			TEXT("Request_RemoveAction: no Action with tag [{}] (class [{}]) registered on Planner [{}]; nothing to remove."),
			ActionTag, InChildActionClass, InPlanner);
		InDelegate.ExecuteIfBound(InPlanner, ECk_Request_OperationResult::Failed_NotEnqueued);
		return InPlanner;
	}

	// Invalid for a top-level Planner's children (they have no parent Action); the promoted host
	// otherwise. The child's own Tree fragment is the single source of truth AddAction populates.
	auto ParentAction = FCk_Handle_Goap_Action{};
	{
		const auto& ChildTree = ChildAction.Get<ck::FFragment_Goap_Action_Tree>();
		ParentAction = ChildTree.Get_ParentAction();
	}

	{
		auto& Index = InPlanner.Get<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
		Index.RemoveEntry(ActionTag);
	}

	if (ck::IsValid(ParentAction))
	{
		auto& ParentTree = ParentAction.Get<ck::FFragment_Goap_Action_Tree>();
		ParentTree._ChildActions.RemoveSingle(ChildAction);

		// Only promoted sub-Planner hosts carry Activation, hence the Has<> gate. A cache still
		// pointing at the removed child would rob the next UpdateActivation tick of a transition.
		if (ParentAction.Has<ck::FFragment_Goap_Planner_Activation>())
		{
			auto& ParentActivation = ParentAction.Get<ck::FFragment_Goap_Planner_Activation>();
			if (ParentActivation.Get_LastActivatedPlan0() == ChildAction)
			{
				ParentActivation._LastActivatedPlan0 = {};
			}
		}
	}

	// Catalog mutated → re-run setup (cycle detection / dependency rebuild).
	InPlanner.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	// The owner-cascade destroy removes the FRecordOfGoapActions entry. It walks owning-entity
	// relationships, NOT tree edges, so a removed composite's own catalog entries are not cascaded.
	{
		auto ChildAsGeneric = static_cast<FCk_Handle>(ChildAction);
		UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ChildAsGeneric);
	}

	// The re-plan this removal triggers is where completion is meaningful — forward the delegate
	// onto that enqueued Plan request rather than firing here, before any request exists.
	Request_Plan(InPlanner, InDelegate);

	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnActiveChainChanged(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_ActiveChainChanged,
		InPlanner, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnActiveChainChanged(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_ActiveChainChanged,
		InPlanner, InDelegate);
	return InPlanner;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::goap::internal_planner
{
	// Activation/deactivation broadcasts land on the ACTION entity of a promoted mid-tier Planner.
	// A top-level Planner is always active and never fires them — binding on it is harmless.
	static auto DoResolveBroadcastEntity_ForActivation(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle
	{
		if (NOT ck::IsValid(InPlanner)) { return {}; }

		if (InPlanner.Has<ck::FFragment_Goap_Action_Tree>())
		{
			return static_cast<FCk_Handle>(UCk_Utils_Goap_Action_UE::CastChecked(InPlanner));
		}

		return static_cast<FCk_Handle>(InPlanner);
	}
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlanComplete(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_PlanComplete,
		InPlanner, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlanComplete(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_PlanComplete, InPlanner, InDelegate);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlanFailed(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_PlanFailed,
		InPlanner, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlanFailed(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_PlanFailed, InPlanner, InDelegate);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlannerActivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in BindTo_OnPlannerActivated"))
	{ return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	CK_ENSURE_IF_NOT(ck::IsValid(BroadcastEntity),
		TEXT("Planner [{}] has no activation-broadcast entity. Bind ignored."),
		InPlanner)
	{ return InPlanner; }

	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_Activated,
		BroadcastEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlannerActivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate) -> FCk_Handle_Goap_Planner
{
	if (NOT ck::IsValid(InPlanner)) { return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	if (NOT ck::IsValid(BroadcastEntity)) { return InPlanner; }

	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_Activated, BroadcastEntity, InDelegate);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlannerDeactivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in BindTo_OnPlannerDeactivated"))
	{ return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	CK_ENSURE_IF_NOT(ck::IsValid(BroadcastEntity),
		TEXT("Planner [{}] has no activation-broadcast entity. Bind ignored."),
		InPlanner)
	{ return InPlanner; }

	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_Deactivated,
		BroadcastEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlannerDeactivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate) -> FCk_Handle_Goap_Planner
{
	if (NOT ck::IsValid(InPlanner)) { return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	if (NOT ck::IsValid(BroadcastEntity)) { return InPlanner; }

	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_Deactivated, BroadcastEntity, InDelegate);
	return InPlanner;
}

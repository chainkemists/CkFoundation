#include "CkGoap/ActionSet/CkGoap_ActionSet_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Record_Internal.h"  // FFragment_RecordOfGoapActionSets + utils struct
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/CkGoap_Utils.h"  // UCk_Utils_Goap_UE::Find_ActionSet (uniqueness check)

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================

auto
	UCk_Utils_Goap_ActionSet_UE::
	AddActionSet(
		FCk_Handle_Goap& InGoap,
		const FCk_Fragment_Goap_ActionSetParamsData& InParams)
	-> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid Goap root handle when adding ActionSet"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_ActionSetTag().IsValid(),
		TEXT("ActionSet params has invalid _ActionSetTag (Goap root [{}])"), InGoap)
	{ return {}; }

	// Diagnostic: ActionSet-tag uniqueness within root.
	if (auto Existing = UCk_Utils_Goap_UE::Find_ActionSet(InGoap, InParams.Get_ActionSetTag());
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("ActionSet with tag [{}] already exists on Goap root [{}]; AddActionSet rejected."),
			InParams.Get_ActionSetTag(), InGoap);
		return {};
	}

	auto ActionSetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_ActionSet>(InGoap);

	// Records of ActionSets require GameplayLabels — label the ActionSet with its
	// declared tag. Same for actions in AddAction_ToActionSet.
	UCk_Utils_GameplayLabel_UE::Add(ActionSetEntity, InParams.Get_ActionSetTag());

	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_Params>(InParams);
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_Current>();

	auto& Current = ActionSetEntity.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_ActiveChain>();
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_ActionCatalogIndex>();
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_WorldStateSource>();
	ActionSetEntity.AddOrGet<ck::FTag_Goap_ActionSet_RequiresSetup>();

	// Register the ActionSet in the root's record.
	ck::goap::internal_root::FRecordOfGoapActionSets_Utils::Request_Connect(InGoap, ActionSetEntity);

	return ActionSetEntity;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_ActionSet_Params>();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Find_Action(
		const FCk_Handle_Goap_ActionSet& InActionSet,
		FGameplayTag InActionTag) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	if (NOT InActionTag.IsValid()) { return {}; }

	const auto& Index = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActionCatalogIndex>();
	const auto* Found = Index.Get_TagToAction().Find(InActionTag);
	return Found ? *Found : FCk_Handle_Goap_Action{};
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_ActiveChain(const FCk_Handle_Goap_ActionSet& InActionSet) -> TArray<FCk_Handle_Goap_Action>
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>().Get_Chain();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_EnableToggle(const FCk_Handle_Goap_ActionSet& InActionSet) -> ECk_EnableDisable
{
	if (NOT ck::IsValid(InActionSet)) { return ECk_EnableDisable::Disable; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_EnableToggle();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_DependencyCycles(const FCk_Handle_Goap_ActionSet& InActionSet) -> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_SetEnableToggle(
		FCk_Handle_Goap_ActionSet& InActionSet,
		ECk_EnableDisable InToggle) -> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in Request_SetEnableToggle"))
	{ return InActionSet; }

	auto& Current = InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._EnableToggle = InToggle;
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_ResetActiveChain(FCk_Handle_Goap_ActionSet& InActionSet) -> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in Request_ResetActiveChain"))
	{ return InActionSet; }

	// Truncate everything past the root (index 0). The ChainUpdate processor's
	// truncate path normally handles per-action teardown (signal firing,
	// unsubscribe, etc.); for a direct reset we set a "needs setup" tag and
	// let the chain-update processor pick it up next tick.
	auto& ActiveChain = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>();
	if (ActiveChain._Chain.Num() > 1)
	{
		ActiveChain._Chain.SetNum(1);
		InActionSet.AddOrGet<ck::FTag_Goap_ActionSet_RequiresChainUpdate>();
	}
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	BindTo_OnActiveChainChanged(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_ActionSet
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_ActionSet_ActiveChainChanged,
		InActionSet, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	UnbindFrom_OnActiveChainChanged(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate) -> FCk_Handle_Goap_ActionSet
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_ActionSet_ActiveChainChanged,
		InActionSet, InDelegate);
	return InActionSet;
}

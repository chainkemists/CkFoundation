#include "CkStateTree_Utils.h"

#include "CkStateTree/CkStateTree_Fragment.h"
#include "CkStateTree/CkStateTree_Log.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <StateTree.h>

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Add(
		FCk_Handle& InHandle,
		const FCk_Fragment_StateTree_ParamsData& InParams)
	-> FCk_Handle_StateTree
{
	InHandle.Add<ck::FFragment_StateTree_Params>(InParams);
	InHandle.Add<ck::FFragment_StateTree_Current>();

	InHandle.Add<ck::FTag_StateTree_RequiresSetup>();

	return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_StateTree_UE, FCk_Handle_StateTree,
	ck::FFragment_StateTree_Params, ck::FFragment_StateTree_Current)

// --------------------------------------------------------------------------------------------------------------------
// Lifecycle Request Functions
// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Request_StartLogic(
		FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_StartLogic& InRequest)
	-> FCk_Handle_StateTree
{
	CK_CALLSTACK_RECORD(ck::FFragment_StateTree_Requests, InHandle);

	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Request_StartLogic"))
	{ return InHandle; }

	InHandle.AddOrGet<ck::FFragment_StateTree_Requests>()._Requests.Emplace(InRequest);
	return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Request_RestartLogic(
		FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_RestartLogic& InRequest)
	-> FCk_Handle_StateTree
{
	CK_CALLSTACK_RECORD(ck::FFragment_StateTree_Requests, InHandle);

	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Request_RestartLogic"))
	{ return InHandle; }

	InHandle.AddOrGet<ck::FFragment_StateTree_Requests>()._Requests.Emplace(InRequest);
	return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Request_StopLogic(
		FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_StopLogic& InRequest)
	-> FCk_Handle_StateTree
{
	CK_CALLSTACK_RECORD(ck::FFragment_StateTree_Requests, InHandle);

	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Request_StopLogic"))
	{ return InHandle; }

	InHandle.AddOrGet<ck::FFragment_StateTree_Requests>()._Requests.Emplace(InRequest);
	return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Request_PauseLogic(
		FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_PauseLogic& InRequest)
	-> FCk_Handle_StateTree
{
	CK_CALLSTACK_RECORD(ck::FFragment_StateTree_Requests, InHandle);

	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Request_PauseLogic"))
	{ return InHandle; }

	InHandle.AddOrGet<ck::FFragment_StateTree_Requests>()._Requests.Emplace(InRequest);
	return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Request_ResumeLogic(
		FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_ResumeLogic& InRequest)
	-> FCk_Handle_StateTree
{
	CK_CALLSTACK_RECORD(ck::FFragment_StateTree_Requests, InHandle);

	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Request_ResumeLogic"))
	{ return InHandle; }

	InHandle.AddOrGet<ck::FFragment_StateTree_Requests>()._Requests.Emplace(InRequest);
	return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
// State Query Functions
// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Get_IsRunning(
		const FCk_Handle_StateTree& InHandle)
	-> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Get_IsRunning"))
	{ return false; }

	const auto& Current = InHandle.Get<ck::FFragment_StateTree_Current>();
	return Current.Get_RunStatus() == ECk_StateTree_RunStatus::Running;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Get_IsPaused(
		const FCk_Handle_StateTree& InHandle)
	-> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Get_IsPaused"))
	{ return false; }

	const auto& Current = InHandle.Get<ck::FFragment_StateTree_Current>();
	return Current.Get_RunStatus() == ECk_StateTree_RunStatus::Paused;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Get_RunStatus(
		const FCk_Handle_StateTree& InHandle)
	-> ECk_StateTree_RunStatus
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Get_RunStatus"))
	{ return ECk_StateTree_RunStatus::Stopped; }

	const auto& Current = InHandle.Get<ck::FFragment_StateTree_Current>();
	return Current.Get_RunStatus();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_StateTree_UE::
	Get_StateTreeAsset(
		const FCk_Handle_StateTree& InHandle)
	-> UStateTree*
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
		TEXT("Invalid handle passed to Get_StateTreeAsset"))
	{ return nullptr; }

	const auto& Params = InHandle.Get<ck::FFragment_StateTree_Params>();
	return Params.Get_Params().Get_StateTree();
}

// --------------------------------------------------------------------------------------------------------------------

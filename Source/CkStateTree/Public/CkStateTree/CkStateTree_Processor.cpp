#include "CkStateTree_Processor.h"

#include "CkStateTree/CkStateTree_Owner.h"
#include "CkStateTree/CkStateTree_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <StateTree.h>
#include <StateTreeExecutionContext.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	auto
		FProcessor_StateTree_Setup::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

		_TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_StateTree_Setup::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent) const
		-> void
	{
		const auto StateTree = InParams.Get_Params().Get_StateTree();

		CK_ENSURE_IF_NOT(ck::IsValid(StateTree),
			TEXT("StateTree Entity [{}] has no StateTree asset assigned"), InHandle)
		{ return; }

		// Create owner object
		auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
		InCurrent._Owner = TStrongObjectPtr<UCk_StateTree_Owner>(
			UCk_StateTree_Owner::Create(World, InHandle));

		CK_ENSURE_IF_NOT(ck::IsValid(InCurrent._Owner.Get()),
			TEXT("StateTree Entity [{}] failed to create Owner object"), InHandle)
		{ return; }

		// Instance data will be initialized by Context.Start() - no explicit Init needed

		// Remove setup tag
		InHandle.Remove<FTag_StateTree_RequiresSetup>();

		// Auto-start if configured
		if (InParams.Get_Params().Get_AutoStart() == ECk_StateTree_AutoStart::OnSetup)
		{
			InHandle.AddOrGet<FFragment_StateTree_Requests>()._Requests.Emplace(FCk_Request_StateTree_StartLogic{});
		}
	}

	// --------------------------------------------------------------------------------------------------------------------

	auto
		FProcessor_StateTree_Tick::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent) const
		-> void
	{
		// Only tick if running
		if (InCurrent.Get_RunStatus() != ECk_StateTree_RunStatus::Running)
		{ return; }

		const auto StateTree = InParams.Get_Params().Get_StateTree();
		auto Owner = InCurrent._Owner.Get();

		CK_ENSURE_IF_NOT(ck::IsValid(Owner) && ck::IsValid(StateTree),
			TEXT("StateTree Entity [{}] has invalid owner or StateTree during tick"), InHandle)
		{ return; }

		// Update cached context in case entity state changed
		Owner->UpdateCachedContext();

		// Create temporary execution context
		FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);

		if (NOT Owner->SetContextRequirements(Context))
		{
			ck::statetree::Warning(TEXT("StateTree Entity [{}] failed to set context requirements"), InHandle);
			return;
		}

		// Tick the state tree
		Context.Tick(InDeltaT.Get_Seconds());
	}

	// --------------------------------------------------------------------------------------------------------------------

	auto
		FProcessor_StateTree_HandleRequests::
		DoTick(
			TimeType InDeltaT)
		-> void
	{
		TProcessor::DoTick(InDeltaT);

		_TransientEntity.Clear<MarkedDirtyBy>();
	}

	auto
		FProcessor_StateTree_HandleRequests::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			FFragment_StateTree_Requests& InRequestsComp) const
		-> void
	{
		InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_StateTree_Requests& InRequests)
		{
			algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
			{
				DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

				if (InRequest.Get_IsRequestHandleValid())
				{
					InRequest.GetAndDestroyRequestHandle();
				}
			}));
		});
	}

	auto
		FProcessor_StateTree_HandleRequests::
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_StartLogic& InRequest)
		-> void
	{
		if (InCurrent.Get_RunStatus() == ECk_StateTree_RunStatus::Running)
		{ return; }

		const auto StateTree = InParams.Get_Params().Get_StateTree();
		auto Owner = InCurrent._Owner.Get();

		CK_ENSURE_IF_NOT(ck::IsValid(Owner) && ck::IsValid(StateTree),
			TEXT("StateTree Entity [{}] cannot start - invalid owner or StateTree"), InHandle)
		{ return; }

		FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);

		if (NOT Owner->SetContextRequirements(Context))
		{
			ck::statetree::Warning(TEXT("StateTree Entity [{}] failed to set context requirements for start"), InHandle);
			return;
		}

		Context.Start();
		InCurrent._RunStatus = ECk_StateTree_RunStatus::Running;
	}

	auto
		FProcessor_StateTree_HandleRequests::
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_RestartLogic& InRequest)
		-> void
	{
		// Stop first
		DoHandleRequest(InHandle, InParams, InCurrent, FCk_Request_StateTree_StopLogic{});

		// Then start
		DoHandleRequest(InHandle, InParams, InCurrent, FCk_Request_StateTree_StartLogic{});
	}

	auto
		FProcessor_StateTree_HandleRequests::
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_StopLogic& InRequest)
		-> void
	{
		if (InCurrent.Get_RunStatus() == ECk_StateTree_RunStatus::Stopped)
		{ return; }

		const auto StateTree = InParams.Get_Params().Get_StateTree();
		auto Owner = InCurrent._Owner.Get();

		if (ck::IsValid(Owner) && ck::IsValid(StateTree))
		{
			FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);

			if (Owner->SetContextRequirements(Context))
			{
				Context.Stop();
			}
		}

		InCurrent._RunStatus = ECk_StateTree_RunStatus::Stopped;
	}

	auto
		FProcessor_StateTree_HandleRequests::
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_PauseLogic& InRequest)
		-> void
	{
		if (InCurrent.Get_RunStatus() != ECk_StateTree_RunStatus::Running)
		{ return; }

		InCurrent._RunStatus = ECk_StateTree_RunStatus::Paused;
	}

	auto
		FProcessor_StateTree_HandleRequests::
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_ResumeLogic& InRequest)
		-> void
	{
		if (InCurrent.Get_RunStatus() != ECk_StateTree_RunStatus::Paused)
		{ return; }

		InCurrent._RunStatus = ECk_StateTree_RunStatus::Running;
	}

	// --------------------------------------------------------------------------------------------------------------------

	auto
		FProcessor_StateTree_EndPlay::
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent) const
		-> void
	{
		// Stop if running or paused
		if (InCurrent.Get_RunStatus() != ECk_StateTree_RunStatus::Stopped)
		{
			const auto StateTree = InParams.Get_Params().Get_StateTree();

			if (auto Owner = InCurrent._Owner.Get(); ck::IsValid(Owner) && ck::IsValid(StateTree))
			{
				FStateTreeExecutionContext Context(*Owner, *StateTree, InCurrent._InstanceData);

				if (Owner->SetContextRequirements(Context))
				{
					Context.Stop();
				}
			}

			InCurrent._RunStatus = ECk_StateTree_RunStatus::Stopped;
		}

		// Clear owner reference (TStrongObjectPtr will release)
		InCurrent._Owner.Reset();
	}

}

// --------------------------------------------------------------------------------------------------------------------

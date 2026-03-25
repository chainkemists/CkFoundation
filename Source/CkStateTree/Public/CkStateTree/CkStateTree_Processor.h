#pragma once

#include "CkStateTree_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class CKSTATETREE_API FProcessor_StateTree_Setup : public ck_exp::TProcessor<
			FProcessor_StateTree_Setup,
			FCk_Handle_StateTree,
			FFragment_StateTree_Params,
			FFragment_StateTree_Current,
			FTag_StateTree_RequiresSetup,
			CK_IGNORE_PENDING_KILL>
	{
	public:
		using MarkedDirtyBy = FTag_StateTree_RequiresSetup;

	public:
		using TProcessor::TProcessor;

	public:
		auto
		DoTick(
			TimeType InDeltaT) -> void;

	public:
		auto
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent) const -> void;
	};

	// --------------------------------------------------------------------------------------------------------------------

	class CKSTATETREE_API FProcessor_StateTree_Tick : public ck_exp::TProcessor<
			FProcessor_StateTree_Tick,
			FCk_Handle_StateTree,
			FFragment_StateTree_Params,
			FFragment_StateTree_Current,
			TExclude<FTag_StateTree_RequiresSetup>,
			CK_IGNORE_PENDING_KILL>
	{
	public:
		using TProcessor::TProcessor;

	public:
		auto
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent) const -> void;
	};

	// --------------------------------------------------------------------------------------------------------------------

	class CKSTATETREE_API FProcessor_StateTree_HandleRequests : public ck_exp::TProcessor<
			FProcessor_StateTree_HandleRequests,
			FCk_Handle_StateTree,
			FFragment_StateTree_Params,
			FFragment_StateTree_Current,
			FFragment_StateTree_Requests,
			CK_IGNORE_PENDING_KILL>
	{
	public:
		using MarkedDirtyBy = FFragment_StateTree_Requests;

	public:
		using TProcessor::TProcessor;

	public:
		auto
		DoTick(
			TimeType InDeltaT) -> void;

	public:
		auto
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			FFragment_StateTree_Requests& InRequestsComp) const -> void;

	private:
		static auto
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_StartLogic& InRequest) -> void;

		static auto
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_RestartLogic& InRequest) -> void;

		static auto
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_StopLogic& InRequest) -> void;

		static auto
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_PauseLogic& InRequest) -> void;

		static auto
		DoHandleRequest(
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent,
			const FCk_Request_StateTree_ResumeLogic& InRequest) -> void;
	};

	// --------------------------------------------------------------------------------------------------------------------

	class CKSTATETREE_API FProcessor_StateTree_EndPlay : public ck_exp::TProcessor<
			FProcessor_StateTree_EndPlay,
			FCk_Handle_StateTree,
			FFragment_StateTree_Params,
			FFragment_StateTree_Current,
			CK_IF_END_PLAY>
	{
	public:
		using TProcessor::TProcessor;

	public:
		auto
		ForEachEntity(
			TimeType InDeltaT,
			HandleType InHandle,
			const FFragment_StateTree_Params& InParams,
			FFragment_StateTree_Current& InCurrent) const -> void;
	};

}

// --------------------------------------------------------------------------------------------------------------------

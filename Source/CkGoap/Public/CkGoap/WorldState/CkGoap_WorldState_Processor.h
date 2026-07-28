#pragma once

#include "CkGoap_WorldState_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------
// Drains Set / RegisterKey queue, mutates values, broadcasts OnValueChanged

class CKGOAP_API FProcessor_Goap_WorldState_HandleRequests : public ck_exp::TProcessor<
	FProcessor_Goap_WorldState_HandleRequests,
	FCk_Handle_Goap_WorldState,
	ck::TReadWrite<FFragment_Goap_WorldState_KeyRegistry>,
	ck::TReadWrite<FFragment_Goap_WorldState_Values>,
	ck::TReadWrite<FFragment_Goap_WorldState_Subscribers>,
	ck::TReadOnly<FFragment_Goap_WorldState_Requests>,
	TExclude<FTag_DestroyEntity_Initiate>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using MarkedDirtyBy = FFragment_Goap_WorldState_Requests;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		FFragment_Goap_WorldState_Values& InValues,
		FFragment_Goap_WorldState_Subscribers& InSubscribers,
		const FFragment_Goap_WorldState_Requests& InRequests) const -> void;

private:
	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		FFragment_Goap_WorldState_Values& InValues,
		FFragment_Goap_WorldState_Subscribers& InSubscribers,
		const FCk_Request_Goap_WorldState_SetValue& InRequest) -> ECk_Request_OperationResult;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		const FCk_Request_Goap_WorldState_RegisterKey& InRequest) -> ECk_Request_OperationResult;
};

// --------------------------------------------------------------------------------------------------------------------
// HandleRequests excludes owners already tagged for destruction, so a destroyed WorldState's still-
// queued requests are never drained. This fires each pending request's completion delegate with
// Failed_Cancelled so a caller awaiting completion terminates instead of hanging.

class CKGOAP_API FProcessor_Goap_WorldState_CancelPendingRequests : public ck_exp::TProcessor<
	FProcessor_Goap_WorldState_CancelPendingRequests,
	FCk_Handle_Goap_WorldState,
	ck::TReadOnly<FFragment_Goap_WorldState_Requests>,
	CK_IF_END_PLAY>
{
public:
	using Group = FGroup_EndPlay;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_WorldState_Requests& InRequestsComp)
		-> void;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck

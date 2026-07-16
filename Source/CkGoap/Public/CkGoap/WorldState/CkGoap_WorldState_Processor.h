#pragma once

#include "CkGoap_WorldState_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
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
		const FCk_Request_Goap_WorldState_SetValue& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		const FCk_Request_Goap_WorldState_RegisterKey& InRequest) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck

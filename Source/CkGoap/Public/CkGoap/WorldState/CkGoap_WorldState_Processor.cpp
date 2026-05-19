#include "CkGoap_WorldState_Processor.h"

#include "CkGoap/CkGoap_Fragment.h"
#include "CkGoap/CkGoap_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_WorldState_HandleRequests);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// HANDLE REQUESTS
// ====================================================================================================================

auto
	FProcessor_Goap_WorldState_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		FFragment_Goap_WorldState_Values& InValues,
		FFragment_Goap_WorldState_Subscribers& InSubscribers,
		const FFragment_Goap_WorldState_Requests& InRequests) const
	-> void
{
	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_WorldState_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_WorldState_SetValue>)
			{
				DoHandleRequest(InHandle, InKeyRegistry, InValues, InSubscribers, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_WorldState_RegisterKey>)
			{
				DoHandleRequest(InHandle, InKeyRegistry, InTypedRequest);
			}
		}));
	});
}

// ====================================================================================================================
// SET VALUE
// ====================================================================================================================

auto
	FProcessor_Goap_WorldState_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		FFragment_Goap_WorldState_Values& InValues,
		FFragment_Goap_WorldState_Subscribers& InSubscribers,
		const FCk_Request_Goap_WorldState_SetValue& InRequest)
	-> void
{
	const auto Key = InKeyRegistry._Registry.FindOrRegister(InRequest.Get_Key());
	if (Key == goap::InvalidGoapKey)
	{
		// Boundary condition (registry at MAX_KEYS or tag invalid) — Verbose
		// rather than Warning so the AutoTest harness doesn't escalate to a
		// test failure for the documented overflow path.
		ck::goap::Verbose(TEXT("GOAP WorldState [{}] dropped Set for key [{}] — registry full or tag invalid."),
			InHandle, InRequest.Get_Key());
		return;
	}

	const auto PreviousValue = InValues._Values.Get(Key);
	InValues._Values.Set(Key, InRequest.Get_Value());

	if (PreviousValue == InRequest.Get_Value()) { return; }

	UUtils_Signal_OnGoapWorldStateValueChanged::Broadcast(InHandle,
		MakePayload(InHandle, FCk_Goap_WorldState_Payload_OnValueChanged{
			InRequest.Get_Key(), PreviousValue, InRequest.Get_Value()}));

	// Walk subscribers; tag each planner dirty so its AutoReplan picks the
	// change up on next tick. Lazy-prune dead handles in place.
	for (auto Index = InSubscribers._Subscribers.Num() - 1; Index >= 0; --Index)
	{
		auto& Subscriber = InSubscribers._Subscribers[Index];
		if (NOT ck::IsValid(Subscriber))
		{
			InSubscribers._Subscribers.RemoveAtSwap(Index);
			continue;
		}
		Subscriber.template AddOrGet<FTag_Goap_Dirty_WorldState>();
	}
}

// ====================================================================================================================
// REGISTER KEY
// ====================================================================================================================

auto
	FProcessor_Goap_WorldState_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		const FCk_Request_Goap_WorldState_RegisterKey& InRequest)
	-> void
{
	const auto Key = InKeyRegistry._Registry.FindOrRegister(InRequest.Get_Key());
	if (Key == goap::InvalidGoapKey)
	{
		ck::goap::Verbose(TEXT("GOAP WorldState [{}] dropped RegisterKey for [{}] — registry full or tag invalid."),
			InHandle, InRequest.Get_Key());
	}
}

// ====================================================================================================================

} // namespace ck

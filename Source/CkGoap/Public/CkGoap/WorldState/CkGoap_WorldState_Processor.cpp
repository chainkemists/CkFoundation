#include "CkGoap_WorldState_Processor.h"

#include "CkGoap_WorldState_Utils.h"  // owning-WS resolution for parent-fallback write forwarding

#include "CkGoap/CkGoap_Fragment.h"
#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Stats.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_WorldState_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_WorldState_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("GoapWorldState::HandleRequests"), STAT_Goap_WorldState_HandleRequests, STATGROUP_CkGoap);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

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
	SCOPE_CYCLE_COUNTER(STAT_Goap_WorldState_HandleRequests);

	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_WorldState_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			auto Result = ECk_Request_OperationResult::Failed;
			const auto Guard = MakeCompletionGuard(InTypedRequest, InHandle, Result);

			if constexpr (std::is_same_v<T, FCk_Request_Goap_WorldState_SetValue>)
			{
				Result = DoHandleRequest(InHandle, InKeyRegistry, InValues, InSubscribers, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_WorldState_RegisterKey>)
			{
				Result = DoHandleRequest(InHandle, InKeyRegistry, InTypedRequest);
			}
		}));
	});
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_WorldState_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		FFragment_Goap_WorldState_Values& InValues,
		FFragment_Goap_WorldState_Subscribers& InSubscribers,
		const FCk_Request_Goap_WorldState_SetValue& InRequest)
	-> ECk_Request_OperationResult
{
	const auto& Self = InHandle;
	auto Owner = UCk_Utils_Goap_WorldState_UE::Get_OwningWorldStateForKey(Self, InRequest.Get_Key());

	if (NOT ck::IsValid(Owner))
	{
		// Imported key whose parent chain no longer resolves: drop rather than write the stale
		// local alias slot — reads for this key are false, so a local write could never be read
		// back. Verbose, not Warning (documented boundary condition; harness escalates Warnings).
		ck::goap::Verbose(TEXT("GOAP WorldState [{}] dropped Set for imported key [{}] — parent chain unresolvable."),
			InHandle, InRequest.Get_Key());
		return ECk_Request_OperationResult::Failed;
	}

	if (Owner != Self)
	{
		// Parent-fallback forwarding: direct-apply on the owning ancestor, same drain pass.
		// INVARIANT: never touch the ancestor's Requests fragment — re-enqueueing would
		// interleave with its own CopyAndRemove drain and reintroduce deferred-write ordering bugs.
		const auto OwnerKey = Owner.Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry().Find(InRequest.Get_Key());
		if (OwnerKey == goap::InvalidGoapKey)
		{
			ck::goap::Verbose(TEXT("GOAP WorldState [{}] dropped forwarded Set for key [{}] — owner [{}] lost the key."),
				InHandle, InRequest.Get_Key(), Owner);
			return ECk_Request_OperationResult::Failed;
		}

		auto& OwnerValues = Owner.Get<FFragment_Goap_WorldState_Values>();
		const auto PreviousValue = OwnerValues._Values.Get(OwnerKey);
		OwnerValues._Values.Set(OwnerKey, InRequest.Get_Value());

		if (PreviousValue == InRequest.Get_Value()) { return ECk_Request_OperationResult::Succeeded; }

		Owner.AddOrGet<FFragment_Goap_WorldState_ChangeLog>().Record(
			FCk_Goap_WorldStateChange{InRequest.Get_Key(), PreviousValue, InRequest.Get_Value(),
				static_cast<int64>(GFrameCounter), ECk_Goap_WorldStateMutator::SetValue});

		UUtils_Signal_OnGoapWorldStateValueChanged::Broadcast(Owner,
			MakePayload(Owner, FCk_Goap_WorldState_Payload_OnValueChanged{
				InRequest.Get_Key(), PreviousValue, InRequest.Get_Value()}));

		auto& OwnerSubscribers = Owner.Get<FFragment_Goap_WorldState_Subscribers>();
		for (auto Index = OwnerSubscribers._Subscribers.Num() - 1; Index >= 0; --Index)
		{
			auto& Subscriber = OwnerSubscribers._Subscribers[Index];
			if (NOT ck::IsValid(Subscriber))
			{
				OwnerSubscribers._Subscribers.RemoveAtSwap(Index);
				continue;
			}
			Subscriber.AddOrGet<FTag_Goap_Dirty_WorldState>();
		}

		return ECk_Request_OperationResult::Succeeded;
	}

	const auto Key = InKeyRegistry._Registry.FindOrRegister(InRequest.Get_Key());
	if (Key == goap::InvalidGoapKey)
	{
		// Verbose, not Warning: the AutoTest harness escalates Warnings to failures and this
		// registry-full / invalid-tag path is a documented, expected boundary condition.
		ck::goap::Verbose(TEXT("GOAP WorldState [{}] dropped Set for key [{}] — registry full or tag invalid."),
			InHandle, InRequest.Get_Key());
		return ECk_Request_OperationResult::Failed;
	}

	const auto PreviousValue = InValues._Values.Get(Key);
	InValues._Values.Set(Key, InRequest.Get_Value());

	if (PreviousValue == InRequest.Get_Value())
	{ return ECk_Request_OperationResult::Succeeded; }

	InHandle.AddOrGet<FFragment_Goap_WorldState_ChangeLog>().Record(
		FCk_Goap_WorldStateChange{InRequest.Get_Key(), PreviousValue, InRequest.Get_Value(),
			static_cast<int64>(GFrameCounter), ECk_Goap_WorldStateMutator::SetValue});

	UUtils_Signal_OnGoapWorldStateValueChanged::Broadcast(InHandle,
		MakePayload(InHandle, FCk_Goap_WorldState_Payload_OnValueChanged{
			InRequest.Get_Key(), PreviousValue, InRequest.Get_Value()}));

	for (auto Index = InSubscribers._Subscribers.Num() - 1; Index >= 0; --Index)
	{
		auto& Subscriber = InSubscribers._Subscribers[Index];
		if (ck::Is_NOT_Valid(Subscriber))
		{
			InSubscribers._Subscribers.RemoveAtSwap(Index);
			continue;
		}
		Subscriber.template AddOrGet<FTag_Goap_Dirty_WorldState>();
	}

	return ECk_Request_OperationResult::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_WorldState_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState_KeyRegistry& InKeyRegistry,
		const FCk_Request_Goap_WorldState_RegisterKey& InRequest)
	-> ECk_Request_OperationResult
{
	const auto Key = InKeyRegistry._Registry.FindOrRegister(InRequest.Get_Key());
	if (Key == goap::InvalidGoapKey)
	{
		ck::goap::Verbose(TEXT("GOAP WorldState [{}] dropped RegisterKey for [{}] — registry full or tag invalid."),
			InHandle, InRequest.Get_Key());
		return ECk_Request_OperationResult::Failed;
	}

	return ECk_Request_OperationResult::Succeeded;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_WorldState_CancelPendingRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_WorldState_Requests& InRequestsComp)
	-> void
{
	request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
}

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck

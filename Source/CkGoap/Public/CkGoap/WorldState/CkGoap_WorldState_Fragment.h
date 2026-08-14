#pragma once

#include "CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

// Global-scope forward decl, required above `namespace ck`: without it the `friend class
// UCk_Utils_Goap_WorldState_UE;` lines below bind to ck::UCk_Utils_Goap_WorldState_UE instead.
class UCk_Utils_Goap_WorldState_UE;

namespace ck { class FProcessor_Goap_Planner_HandleRequests; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

using FFragment_Goap_WorldState_Params = FCk_Fragment_Goap_WorldState_ParamsData;

// --------------------------------------------------------------------------------------------------------------------

struct CKGOAP_API FFragment_Goap_WorldState_KeyRegistry
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_KeyRegistry);

	friend class UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_WorldState_HandleRequests;
	friend class FProcessor_Goap_Setup;
	friend class FProcessor_Goap_HandleRequests;

private:
	goap::FKeyRegistry _Registry;

public:
	CK_PROPERTY_GET(_Registry);

	auto Get_MutableRegistry() -> goap::FKeyRegistry& { return _Registry; }
};

// --------------------------------------------------------------------------------------------------------------------

struct CKGOAP_API FFragment_Goap_WorldState_Values
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_Values);

	friend class UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_WorldState_HandleRequests;
	friend class FProcessor_Goap_HandleRequests;

private:
	goap::FWorldState _Values;

public:
	CK_PROPERTY_GET(_Values);

	auto Get_MutableValues() -> goap::FWorldState& { return _Values; }
};

// --------------------------------------------------------------------------------------------------------------------

struct CKGOAP_API FFragment_Goap_WorldState_Requests
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_Requests);

	friend class UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_WorldState_HandleRequests;

	using RequestType = std::variant<
		FCk_Request_Goap_WorldState_SetValue,
		FCk_Request_Goap_WorldState_RegisterKey>;

private:
	TArray<RequestType> _Requests;

public:
	CK_PROPERTY_GET(_Requests);
};

// --------------------------------------------------------------------------------------------------------------------
// Present only on a sub-WS composed with a _FallbackParent. _ImportedTags marks keys whose truth
// lives in an ancestor: the local registry index is a per-plan snapshot alias; Get_Value and
// SetValue route through to the owning ancestor. Dead parent => imported keys degrade to miss
// semantics (reads false, writes dropped) — the stale alias slot is never served as truth.

struct CKGOAP_API FFragment_Goap_WorldState_ParentLink
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_ParentLink);

	friend class ::UCk_Utils_Goap_WorldState_UE;

private:
	FCk_Handle_Goap_WorldState _Parent;
	TSet<FGameplayTag> _ImportedTags;

public:
	CK_PROPERTY_GET(_Parent);

	auto Get_IsImported(FGameplayTag InTag) const -> bool { return _ImportedTags.Contains(InTag); }
	auto Get_ImportedTags() const -> const TSet<FGameplayTag>& { return _ImportedTags; }
};

// --------------------------------------------------------------------------------------------------------------------

struct CKGOAP_API FFragment_Goap_WorldState_OverrideStack
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_OverrideStack);

	friend class ::UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_Planner_HandleRequests;

	struct FLayer
	{
		FName Name;
		TMap<FGameplayTag, bool> Values;
	};

private:
	TArray<FLayer> _Layers;

public:
	auto Get_Layers() const -> const TArray<FLayer>& { return _Layers; }
};

// --------------------------------------------------------------------------------------------------------------------

struct CKGOAP_API FFragment_Goap_WorldState_Subscribers
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_Subscribers);

	friend class UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_WorldState_HandleRequests;
	friend class FProcessor_Goap_Planner_UpdateActivation;

private:
	TArray<FCk_Handle> _Subscribers;

public:
	CK_PROPERTY_GET(_Subscribers);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKGOAP_API FFragment_Goap_WorldState_ChangeLog
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_ChangeLog);

	friend class ::UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_WorldState_HandleRequests;

	static constexpr int32 Capacity = 32;

private:
	TArray<FCk_Goap_WorldStateChange> _Entries;

public:
	CK_PROPERTY_GET(_Entries);

	auto Record(FCk_Goap_WorldStateChange InChange) -> void
	{
		_Entries.Add(MoveTemp(InChange));
		if (_Entries.Num() > Capacity)
		{ _Entries.RemoveAt(0, _Entries.Num() - Capacity); }
	}
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
	CKGOAP_API,
	OnGoapWorldStateValueChanged,
	FCk_Delegate_Goap_WorldState_OnValueChanged,
	FCk_Handle_Goap_WorldState,
	FCk_Goap_WorldState_Payload_OnValueChanged);

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck

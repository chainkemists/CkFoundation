#pragma once

#include "CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// ====================================================================================================================

// Forward decl in global scope so the `friend class UCk_Utils_Goap_WorldState_UE;` lines below resolve
// to the real global class — without this, the friend lookup binds to ck::UCk_Utils_Goap_WorldState_UE
// (the enclosing namespace) and silently grants access to the wrong (non-existent) class. Mirrors the
// pattern in CkGoap_Fragment.h. Keep this above the `namespace ck` block.
class UCk_Utils_Goap_WorldState_UE;

namespace ck { class FProcessor_Goap_Action_HandleRequests; class FProcessor_Goap_Planner_HandleRequests; }

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// PARAMS
// ====================================================================================================================

using FFragment_Goap_WorldState_Params = FCk_Fragment_Goap_WorldState_ParamsData;

// ====================================================================================================================
// KEY REGISTRY FRAGMENT — Maps FGameplayTag ↔ FCk_GoapKey
// ====================================================================================================================
//
// One registry per WorldState entity. Every planner pointing at this WS shares
// the same registry, so key indices are globally consistent across the layer
// of planners that observe this state. The GOAP Setup processor populates the
// registry via FindOrRegister when scanning action/goal CDOs.

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

	// Mutable accessor — public so the GOAP Setup processor (which lives in
	// CkGoap, not in this WorldState sub-feature's friend list once dependency
	// direction settles) can register keys on demand. Read-only callers
	// continue to use the const Get_Registry.
	auto Get_MutableRegistry() -> goap::FKeyRegistry& { return _Registry; }
};

// ====================================================================================================================
// VALUES FRAGMENT — TStaticArray-backed boolean state, fixed-size for O(1) compare/hash
// ====================================================================================================================

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

	// Mutable accessor — public so callers in adjacent translation units that
	// need to mutate values via FWorldState's own typed setters (e.g. the
	// GOAP planner's A* effect-application path) can do so without being a
	// friend. Mirrors the pattern on FFragment_Goap_WorldState in the older
	// planner-owned layout that this replaces.
	auto Get_MutableValues() -> goap::FWorldState& { return _Values; }
};

// ====================================================================================================================
// REQUESTS FRAGMENT — Request queue drained by FProcessor_Goap_WorldState_HandleRequests
// ====================================================================================================================

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

// ====================================================================================================================
// OVERRIDE STACK FRAGMENT — Named layers shadowing base WorldState values
// ====================================================================================================================
//
// Reads (Get_Value) walk this stack TOP-DOWN before falling through to the
// base FFragment_Goap_WorldState_Values. Writes (Set_Value) always mutate the
// base — override layers are read-overlay only. The non-negotiable invariant.
//
// Layers are NAMED so a debug-UI layer ("DebugUI") and AI-deliberation scopes
// can coexist independently. Re-pushing a layer with the same name REPLACES
// its contents idempotently.
//
// Push / pop / clear fire FTag_Goap_Dirty_WorldState on every subscriber for
// keys whose EFFECTIVE value changes. Re-push with the same values does NOT
// fire dirty (regression-test invariant).
//
// At A* seed time the planner flattens this stack onto a snapshot FWorldState
// (base + each layer bottom-to-top) so the inner search loop reads only a
// flat array — stack walks stay out of the hot path.

struct CKGOAP_API FFragment_Goap_WorldState_OverrideStack
{
public:
	CK_GENERATED_BODY(FFragment_Goap_WorldState_OverrideStack);

	friend class ::UCk_Utils_Goap_WorldState_UE;
	friend class FProcessor_Goap_Action_HandleRequests;
	friend class FProcessor_Goap_Planner_HandleRequests;  // PR-B.1b Stage 3

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

// ====================================================================================================================
// SUBSCRIBERS FRAGMENT — Entity handles that should be tagged dirty on WS value change
// ====================================================================================================================
//
// In the unified ActionSet/Action model, subscribers are ACTION entities
// (FCk_Handle_Goap_Action). The list is typed as generic FCk_Handle for two
// reasons:
//   1. The WS processor needs to stamp FTag_Goap_Dirty_WorldState on the
//      subscriber, which is a generic-handle operation.
//   2. Future flexibility (e.g. external systems that want to react to WS
//      changes without being an Action).
//
// Subscription happens at Action activation (ActionSet ChainUpdate appends
// the Action to the active chain); unsubscription at deactivation.
// Lazy-pruned on each walk — invalid entries (destroyed Actions) drop out as
// they're encountered.

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

// ====================================================================================================================
// SIGNAL
// ====================================================================================================================

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
	CKGOAP_API,
	OnGoapWorldStateValueChanged,
	FCk_Delegate_Goap_WorldState_OnValueChanged,
	FCk_Handle_Goap_WorldState,
	FCk_Goap_WorldState_Payload_OnValueChanged);

// ====================================================================================================================

} // namespace ck

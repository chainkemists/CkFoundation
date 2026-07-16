#pragma once

#include "CkEqs/Query/CkEqs_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------
// CkEqs_Fragment.h — non-reflected ECS-side types: fragments, tags, signals.
// All in namespace ck where applicable.
// --------------------------------------------------------------------------------------------------------------------

// Aliases exposed to processors. The stored data is the reflected USTRUCT from
// CkEqs_Fragment_Data.h; the alias keeps processor signatures referring to ECS-side names.
using FFragment_EqsQuery_Params    = FCk_Eqs_QueryParams;
using FFragment_EqsQuery_Results   = FCk_Eqs_QueryResults;
using FFragment_EqsQuery_DebugInfo = FCk_Fragment_EqsQuery_DebugInfoData;

// --------------------------------------------------------------------------------------------------------------------
// FFragment_EqsQuery_State — transient evaluation state. NOT reflected. Lives only on
// the query entity between Generate and Finalize.
//
// Tests are atomic with respect to budget yields. The cursor is _NextTestIndex
// (test boundary), NOT a per-candidate cursor. Yielding mid-test would corrupt Min/Max
// normalization across frames. Do not add `_NextCandidateIndexInTest`.
// --------------------------------------------------------------------------------------------------------------------

struct CKEQS_API FFragment_EqsQuery_State
{
    CK_GENERATED_BODY(FFragment_EqsQuery_State);

    friend struct FCk_Eqs_Algorithm;
    friend class  ck::FProcessor_Eqs_Generate;
    friend class  ck::FProcessor_Eqs_Test;
    friend class  ck::FProcessor_Eqs_Finalize;
    friend class  UCk_Utils_Eqs_UE;

private:
    TArray<FCk_Eqs_Candidate> _Candidates;
    int32 _NextTestIndex = 0;

public:
    CK_PROPERTY_GET(_Candidates);
    CK_PROPERTY_GET(_NextTestIndex);
};

// --------------------------------------------------------------------------------------------------------------------
// FFragment_EqsQuery_Requests — request queue on the querier entity (any entity that
// can be a querier; not the EqsQuery entity). Drained each frame by HandleRequests.
// --------------------------------------------------------------------------------------------------------------------

struct CKEQS_API FFragment_EqsQuery_Requests
{
    CK_GENERATED_BODY(FFragment_EqsQuery_Requests);

    friend class ck::FProcessor_Eqs_HandleRequests;

    using RequestType = FCk_Request_Eqs_RunQuery;

private:
    TArray<RequestType> _Requests;

public:
    CK_PROPERTY(_Requests);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EqsQuery_Pending);     // Generated, awaiting Generate
    CK_DEFINE_ECS_TAG(FTag_EqsQuery_InProgress);  // Generated; tests running (possibly multi-frame)
    CK_DEFINE_ECS_TAG(FTag_EqsQuery_Complete);    // Finalize ran; results available
    CK_DEFINE_ECS_TAG(FTag_EqsQuery_Failed);      // Failed (terminal — pairs with Complete)
    CK_DEFINE_ECS_TAG(FTag_EqsQuery_AutoDestroy); // Cleanup processor destroys when Complete
    CK_DEFINE_ECS_TAG(FTag_EqsQuery_Cancelled);   // caller-issued cancel
}

// --------------------------------------------------------------------------------------------------------------------
// Signal — completion broadcast. Bound at request time via CK_SIGNAL_BIND_REQUEST_FULFILLED
// (auto-unbind after first fire) so callers don't need to hold the query handle to subscribe.
//
// Name follows the OnProbeBeginOverlap convention (verb-first; no underscore between
// On and subject). Lives in namespace ck to match codebase convention (referenced as
// ck::UUtils_Signal_OnEqsQueryComplete by binders).
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKEQS_API,
        OnEqsQueryComplete,
        FCk_Delegate_EqsQuery_OnComplete,
        FCk_Handle_EqsQuery,
        FCk_Eqs_QueryResults);
}

// --------------------------------------------------------------------------------------------------------------------

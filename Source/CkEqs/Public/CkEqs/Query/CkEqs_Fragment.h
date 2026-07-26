#pragma once

#include "CkEqs/Query/CkEqs_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

using FFragment_EqsQuery_Params    = FCk_Eqs_QueryParams;
using FFragment_EqsQuery_Results   = FCk_Eqs_QueryResults;
using FFragment_EqsQuery_DebugInfo = FCk_Fragment_EqsQuery_DebugInfoData;

// --------------------------------------------------------------------------------------------------------------------
// _NextTestIndex is a TEST boundary, never a per-candidate one — yielding mid-test would corrupt
// Min/Max normalization across frames. Do not add `_NextCandidateIndexInTest`.
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
// Lives on the QUERIER entity (any entity can be one), not on the EqsQuery entity.
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
// Bound at request time via CK_SIGNAL_BIND_REQUEST_FULFILLED, so a caller can subscribe without
// ever holding the query handle.
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

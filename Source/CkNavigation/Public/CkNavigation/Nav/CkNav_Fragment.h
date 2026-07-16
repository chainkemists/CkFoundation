#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Nav_UE;

namespace ck
{
    // Per-entity request fragment. Holds the queue of pending FindPath requests for an
    // entity; FProcessor_Nav_HandleRequests drains it each tick. Kept as a variant so
    // additional request types (e.g. cancel-find, partial-extension) can land here
    // without changing the processor view shape.
    struct CKNAVIGATION_API FFragment_Nav_Requests
    {
        CK_GENERATED_BODY(FFragment_Nav_Requests);

        friend class FProcessor_Nav_HandleRequests;
        friend class ::UCk_Utils_Nav_UE;

        using FindPathRequestType = FCk_Request_Nav_FindPath;
        using RequestType = std::variant<FindPathRequestType>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Stored result fragment — the path the caller asked for. Written by the processor
    // on every FindPathSync attempt (success or failure); diagnostics are populated even
    // on failure branches so the debugger has the full picture.
    using FFragment_Nav_PathResult = FCk_Nav_PathResult;

    // --------------------------------------------------------------------------------------------------------------------

    // Signals — fired by the request handler on completion. CK_SIGNAL_BIND macros expect
    // the signal-utils types in namespace ck, matching the existing CkAggro / CkAttribute
    // pattern (e.g. CK_SIGNAL_BIND(ck::UUtils_Signal_OnAggroChanged, ...)).

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        Nav_OnPathReady,
        FCk_Delegate_Nav_OnPathReady,
        FCk_Handle,
        FCk_Nav_PathResult);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNAVIGATION_API,
        Nav_OnPathFailed,
        FCk_Delegate_Nav_OnPathFailed,
        FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------

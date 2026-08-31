#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Nav_UE;

namespace ck::nav
{
    CKNAVIGATION_API auto PurgeDeferredRequestsFor(FCk_Handle& InHandle) -> void;
}

namespace ck
{
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

    using FFragment_Nav_PathResult = FCk_Nav_PathResult;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNAVIGATION_API FNav_DeferredRequest
    {
        FCk_Handle Handle;
        FCk_Request_Nav_FindPath Request;
        // FPlatformTime::Seconds() at first deferral — drives the timeout.
        double DeferredAt = 0.0;
    };

    // The queue of FindPath requests parked because their start point is not bakeable yet. Lives on
    // the world's transient entity: two PIE worlds each hold their own, and neither can drain,
    // supersede, or time out the other's entries.
    struct CKNAVIGATION_API FFragment_Nav_DeferredRequests
    {
        CK_GENERATED_BODY(FFragment_Nav_DeferredRequests);

        friend class FProcessor_Nav_HandleRequests;
        friend auto nav::PurgeDeferredRequestsFor(FCk_Handle& InHandle) -> void;

    private:
        TArray<FNav_DeferredRequest> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav
{
    // Revisions cycle through [1, MAX_int32]. The shorter forward distance around that ring is the
    // newer one — there can never be remotely half the revision space worth of live requests for one
    // entity, so the opposite half is unambiguously an older generation.
    CKNAVIGATION_API auto IsNewerRevision(
        int32 InCandidate,
        int32 InExisting) -> bool;

    // Nonzero request revisions opt a caller into latest-request-wins semantics. This is load-bearing
    // for policy changes while nav is still baking: the deferred queue drains from the back, so merely
    // rejecting a stale result at the consumer can otherwise let an older request overwrite the newer
    // shared result slot in the same pump.
    CKNAVIGATION_API auto AddDeferredLatest(
        TArray<FNav_DeferredRequest>&   InOutQueue,
        const FCk_Handle&               InHandle,
        const FCk_Request_Nav_FindPath& InRequest,
        double                          InDeferredAt) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

#pragma once

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include "CkEcs/Handle/CkHandle.h"

#include <type_traits>
#include <variant>

#include "CkRequest_Completion.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/** Outcome reported to a caller that supplied a completion delegate to a Request_* util.
 *  Failed_NotEnqueued: rejected synchronously at the Utils boundary, same call stack, never queued.
 *  Failed_Cancelled: queued, but the owner was torn down before a processor drained it. */
UENUM(BlueprintType)
enum class ECk_Request_OperationResult : uint8
{
    Succeeded,
    Failed,
    Failed_NotEnqueued UMETA(DisplayName = "Failed (Not Enqueued)"),
    Failed_Cancelled   UMETA(DisplayName = "Failed (Cancelled)")
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Request_OperationResult);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_Request_OnCompleted,
    FCk_Handle, InRequestOwner,
    ECk_Request_OperationResult, InResult);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /** Fires the request's own completion delegate at scope exit with whatever the referenced result
     *  holds by then, so a handler that returns early still completes its caller.
     *
     *  Captures-by-reference invariant: the result MUST outlive this guard. Declare the guard *after*
     *  the result local and do not reorder them.
     *
     *  T_Request must satisfy the FCk_Request_Base shape (`TryFireCompletion`). */
    template <typename T_Request>
    struct TCompletionGuard
    {
        TCompletionGuard(
            const T_Request& InRequest,
            const FCk_Handle& InOwner,
            const ECk_Request_OperationResult& InResult)
            : _Request{InRequest}, _Owner{InOwner}, _Result{InResult} {}

        ~TCompletionGuard()
        {
            _Request.TryFireCompletion(_Owner, _Result);
        }

        TCompletionGuard(const TCompletionGuard&) = delete;
        TCompletionGuard& operator=(const TCompletionGuard&) = delete;
        TCompletionGuard(TCompletionGuard&&) = delete;
        TCompletionGuard& operator=(TCompletionGuard&&) = delete;

    private:
        const T_Request& _Request;
        FCk_Handle _Owner;
        const ECk_Request_OperationResult& _Result;
    };

    template <typename T_Request>
    auto
        MakeCompletionGuard(
            const T_Request& InRequest,
            const FCk_Handle& InOwner,
            const ECk_Request_OperationResult& InResult)
    {
        return TCompletionGuard<T_Request>{InRequest, InOwner, InResult};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::request
{
    template <typename T>
    struct TRequestEntry_IsVariant : std::false_type {};

    template <typename... T_Alternatives>
    struct TRequestEntry_IsVariant<std::variant<T_Alternatives...>> : std::true_type {};

    /** Fires Failed_Cancelled for every pending request. Call from a feature's EndPlay processor while
     *  its requests fragment is still alive: a HandleRequests processor that skips destroyed owners
     *  never drains their queue, so without this a caller awaiting completion waits forever.
     *  Unconditional by design — TryFireCompletion is itself the exactly-once gate.
     *
     *  Handles both request-list shapes: most features hold a std::variant of their request types,
     *  but some hold a plain TArray of one concrete request. ck::Visitor cannot serve the latter —
     *  its operator() calls std::visit unconditionally, which does not compile on a non-variant. */
    template <typename T_RequestList>
    auto
        FireCancelledForPending(
            const FCk_Handle& InOwner,
            const T_RequestList& InRequests)
        -> void
    {
        algo::ForEachRequest(InRequests,
        [&](const auto& InEntry) -> void
        {
            if constexpr (TRequestEntry_IsVariant<std::decay_t<decltype(InEntry)>>::value)
            {
                std::visit([&](const auto& InRequest) -> void
                {
                    InRequest.TryFireCompletion(InOwner, ECk_Request_OperationResult::Failed_Cancelled);
                }, InEntry);
            }
            else
            {
                InEntry.TryFireCompletion(InOwner, ECk_Request_OperationResult::Failed_Cancelled);
            }
        }, policy::DontResetContainer{});
    }
}

// --------------------------------------------------------------------------------------------------------------------

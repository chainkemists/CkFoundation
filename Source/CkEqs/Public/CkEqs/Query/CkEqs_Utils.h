#pragma once

#include "CkEqs/Query/CkEqs_Fragment.h"
#include "CkEqs/Query/CkEqs_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEqs_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_EqsQuery"))
class CKEQS_API UCk_Utils_Eqs_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Eqs_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EqsQuery);

public:
    // True when the handle identifies a query entity (carries Params + State).
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EqsQuery
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Handle -> EqsQuery Handle",
        meta = (CompactNodeTitle = "<AsEqsQuery>", BlueprintAutocast))
    static FCk_Handle_EqsQuery
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid EqsQuery Handle",
        Category = "Ck|Utils|Eqs",
        meta = (CompactNodeTitle = "INVALID_EqsQueryHandle", Keywords = "make"))
    static FCk_Handle_EqsQuery
    Get_InvalidHandle() { return {}; }

public:
    // ----------------------------------------------------------------------------------------------------------------
    // Deferred path: the queue is drained next frame by FProcessor_Eqs_HandleRequests, which spawns
    // the query entity and binds the request's optional OnComplete. Server-authoritative.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Run Query")
    static FCk_Handle
    Request_RunQuery(
        UPARAM(ref) FCk_Handle& InQuerierEntity,
        const FCk_Request_Eqs_RunQuery& InRequest);

    // ----------------------------------------------------------------------------------------------------------------
    // Synchronous path: runs the whole pipeline in-line and returns a handle with results already
    // written. It does NOT broadcast OnEqsQueryComplete — the signal would fire before the caller
    // holds the handle — so read the accessors, or use the deferred path for delegates. That is
    // also why it takes QueryParams rather than the request type: _OnComplete has no use here.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Run Query (Immediate)")
    static FCk_Handle_EqsQuery
    Request_RunQuery_Immediate(
        UPARAM(ref) FCk_Handle& InQuerierEntity,
        const FCk_Eqs_QueryParams& InQueryParams);

    // ----------------------------------------------------------------------------------------------------------------
    // Cancellation is a tag: FProcessor_Eqs_Test picks it up, fails the query with empty results,
    // and broadcasts OnComplete. Nothing happens synchronously here.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Cancel Query")
    static FCk_Handle_EqsQuery
    Request_CancelQuery(
        UPARAM(ref) FCk_Handle_EqsQuery& InQueryEntity);

    // Cancels every in-flight query context-owned by this querier; returns how many were tagged.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Cancel All Queries (for Querier)")
    static int32
    Request_CancelAllQueries(
        UPARAM(ref) FCk_Handle& InQuerierEntity);

    // ----------------------------------------------------------------------------------------------------------------
    // Signal bindings (deferred path only — Immediate does not broadcast).
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Bind To OnComplete")
    static FCk_Handle_EqsQuery
    BindTo_OnComplete(
        UPARAM(ref) FCk_Handle_EqsQuery& InQueryEntity,
        const FCk_Delegate_EqsQuery_OnComplete& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Unbind From OnComplete")
    static FCk_Handle_EqsQuery
    UnbindFrom_OnComplete(
        UPARAM(ref) FCk_Handle_EqsQuery& InQueryEntity,
        const FCk_Delegate_EqsQuery_OnComplete& InDelegate);

    // ----------------------------------------------------------------------------------------------------------------
    // Safe on incomplete or invalid queries, which is the trap: ALWAYS check Get_IsComplete +
    // Get_HasResults first, because Get_BestLocation returns ZeroVector — a valid location, easy
    // to mistake for a real result.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs", DisplayName = "[Ck][Eqs] Get Has Results")
    static bool
    Get_HasResults(
        const FCk_Handle_EqsQuery& InQuery);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs", DisplayName = "[Ck][Eqs] Get Best Location")
    static FVector
    Get_BestLocation(
        const FCk_Handle_EqsQuery& InQuery);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs", DisplayName = "[Ck][Eqs] Get Best Entity")
    static FCk_Handle
    Get_BestEntity(
        const FCk_Handle_EqsQuery& InQuery);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs", DisplayName = "[Ck][Eqs] Get All Candidates")
    static TArray<FCk_Eqs_Candidate>
    Get_AllCandidates(
        const FCk_Handle_EqsQuery& InQuery);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs", DisplayName = "[Ck][Eqs] Get Is Complete")
    static bool
    Get_IsComplete(
        const FCk_Handle_EqsQuery& InQuery);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Eqs", DisplayName = "[Ck][Eqs] Get Is Failed")
    static bool
    Get_IsFailed(
        const FCk_Handle_EqsQuery& InQuery);
};

// --------------------------------------------------------------------------------------------------------------------

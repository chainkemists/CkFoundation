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
    // Has Feature — true if the handle identifies a query entity (has Params + State).
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
    Get_InvalidHandle() { return {}; };

public:
    // ----------------------------------------------------------------------------------------------------------------
    // Deferred path — Request_RunQuery
    //
    // Queues a query request on the querier. FProcessor_Eqs_HandleRequests drains the queue
    // next frame, spawns the query entity, binds the request's optional OnComplete delegate,
    // and runs the pipeline (Generate / Test / Finalize / Cleanup).
    //
    // Authority-gated. CkEqs is server-only in v1; non-authoritative callers early-out
    // with a warning. Returns the querier handle for chaining.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Run Query")
    static FCk_Handle
    Request_RunQuery(
        UPARAM(ref) FCk_Handle& InQuerierEntity,
        const FCk_Request_Eqs_RunQuery& InRequest);

    // ----------------------------------------------------------------------------------------------------------------
    // Synchronous path — Request_RunQuery_Immediate
    //
    // Spawns the query entity, runs Generate / RunTests / Finalize in-line, returns the
    // typed query handle with results already written. Bypasses the deferred processor pipeline.
    //
    // P3-E5: does NOT broadcast OnEqsQueryComplete (the signal would fire before the caller
    // can bind, so a delegate would never see it). Callers read accessors on the returned
    // handle. For delegate-driven completion, use Request_RunQuery (deferred).
    //
    // Hard-capped at UCk_Utils_Eqs_Settings_UE::Get_MaxCandidates_ImmediatePathHardCap()
    // (default 1024) — runaway generators are truncated with a Warning.
    // Intentionally takes QueryParams (not the request type) — the request's _OnComplete
    // delegate has no use here and would silently be dropped.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Run Query (Immediate)")
    static FCk_Handle_EqsQuery
    Request_RunQuery_Immediate(
        UPARAM(ref) FCk_Handle& InQuerierEntity,
        const FCk_Eqs_QueryParams& InQueryParams);

    // ----------------------------------------------------------------------------------------------------------------
    // Cancellation (Pass-3.1 E3)
    //
    // Adds FTag_EqsQuery_Cancelled. FProcessor_Eqs_Test sees the tag at the top of its
    // ForEachEntity, fails the query with empty results + Failed tag, broadcasts OnComplete,
    // and the Cleanup processor destroys the query entity if AutoDestroy was set.
    // ----------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Eqs",
        DisplayName = "[Ck][Eqs] Request Cancel Query")
    static FCk_Handle_EqsQuery
    Request_CancelQuery(
        UPARAM(ref) FCk_Handle_EqsQuery& InQueryEntity);

    // Cancel every in-flight query owned by this querier (queries created via
    // Request_CreateEntity(InQuerier) — context-owner cascade). Each cancelled query
    // gets the Cancelled tag; FProcessor_Eqs_Test fails them and broadcasts OnComplete
    // (empty results + Failed tag). Returns the count of queries cancelled.
    // Authority-gated.
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
    // Result accessors — all read from FFragment_EqsQuery_Results. Safe to call on
    // incomplete or invalid queries (return defaults / false / empty).
    //
    // Pass-4 non-blocking #3: ALWAYS check Get_IsComplete + Get_HasResults first; reading
    // Get_BestLocation on an incomplete query returns ZeroVector, which is a valid location
    // and easy to mistake for a real result.
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

#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkIntent/Debug/CkIntentDebugHistory_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkIntentDebugHistory_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * A DEBUG-DEPTH recording of one source's frame record — rows copied out of the sampler's ring as they are
 * written, retained far beyond the ring's working window (the consumer shape the module doc's anti-pattern 5
 * prescribes). The intent debugger's timeline reads it; a replay scrubber could.
 *
 * Composed ON the same input-source entity as the sampler, which it requires — there is nothing to record
 * without one. COMPILED OUT IN SHIPPING (the CkStateMachine/Debug precedent): the reflected surface below
 * stays, but in a Shipping build Add composes nothing, every read answers empty, and the record processor does
 * not exist — a debug fragment must cost a shipped game nothing.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IntentDebugHistory"))
class CKINTENT_API UCk_Utils_IntentDebugHistory_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntentDebugHistory_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IntentDebugHistory);

public:
    /**
     * Composes the debug history onto an input source that already carries an IntentSampler. Rejects a handle
     * without a sampler, an entity that already carries a history, and a non-positive capacity — each rejection
     * leaving nothing composed. In Shipping builds this is a documented no-op returning an invalid handle.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Add Feature")
    static FCk_Handle_IntentDebugHistory
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IntentDebugHistory_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IntentDebugHistory
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Handle -> IntentDebugHistory Handle",
              meta = (CompactNodeTitle = "<AsIntentDebugHistory>", BlueprintAutocast))
    static FCk_Handle_IntentDebugHistory
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid IntentDebugHistory Handle",
              Category = "Ck|Utils|IntentDebugHistory",
              meta = (CompactNodeTitle = "INVALID_IntentDebugHistoryHandle", Keywords = "make"))
    static FCk_Handle_IntentDebugHistory
    Get_InvalidHandle() { return {}; }

public:
    /** The current retention cap, in frames. Runtime-mutable via Request_SetCapacity. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Get Capacity")
    static int32
    Get_Capacity(
        const FCk_Handle_IntentDebugHistory& InHistory);

    /** How many frames the history is holding — up to the capacity, oldest evicted first. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Get Frame Count")
    static int32
    Get_FrameCount(
        const FCk_Handle_IntentDebugHistory& InHistory);

    /**
     * A retained row addressed backwards from the newest: offset 0 is the newest, FrameCount-1 the oldest.
     * Same addressing and same no-such-frame answer (a negative FrameIndex) as the sampler's ring, so a reader
     * written against one works against the other.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Try Get Frame At Offset")
    static FCk_Intent_FrameRecord
    TryGet_FrameAtOffset(
        const FCk_Handle_IntentDebugHistory& InHistory,
        int32 InOffset);

    /**
     * Retunes the retention cap. An IMMEDIATE mutator, declared with its reason (the house escape hatch): this
     * is a debug-tooling knob whose only consumer is a human at a debugger, and a request queue plus drain and
     * cancel processors for it would be machinery without a caller. Shrinking trims the oldest rows on the
     * calling stack; growing simply allows more. Rejects a non-positive capacity with Failed_NotEnqueued.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentDebugHistory",
              DisplayName = "[Ck][IntentDebugHistory] Request Set Capacity",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_IntentDebugHistory
    Request_SetCapacity(
        UPARAM(ref) FCk_Handle_IntentDebugHistory& InHistory,
        const FCk_Request_IntentDebugHistory_SetCapacity& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

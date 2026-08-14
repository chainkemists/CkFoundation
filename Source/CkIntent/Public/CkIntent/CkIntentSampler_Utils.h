#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkIntent/CkIntentSampler_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkIntentSampler_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The frame record of one input source: a fixed-capacity ring of what the player did, one row per logic frame.
 *
 * The sampler exists so that everything above it reasons about a SEQUENCE rather than about live state. A press
 * that must be answered "within three frames of a direction" cannot be expressed against fragments that only
 * remember now; it can be expressed trivially against rows. That is also why the row is a plain value with no
 * live handles in it — a row read out of the ring stays true after everything it describes has moved on.
 *
 * The feature lives ON the input-source entity. There is no Create: a sampler on a child entity would have no
 * routed events to read, and the delivery outcomes it records are per-source state the router writes.
 *
 * It is opt-in and reads whatever the source happens to carry: a source with no ButtonMap records empty button
 * sets, a source with no InputBias records the raw axis values, and neither is an error — the record describes
 * the stack that exists, not the one it would prefer.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IntentSampler"))
class CKINTENT_API UCk_Utils_IntentSampler_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntentSampler_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IntentSampler);

public:
    /**
     * Composes the frame record onto an input source. Rejects a handle that is not one, an entity that already
     * carries a sampler, a capacity at or below zero, and an axis pair that is invalid or names one key twice —
     * each rejection leaving nothing composed.
     *
     * The first row lands on the next sampling pass, not on this stack: a record row describes a frame, and there
     * is no frame yet at composition time.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Add Feature")
    static FCk_Handle_IntentSampler
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IntentSampler_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IntentSampler
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Handle -> IntentSampler Handle",
              meta = (CompactNodeTitle = "<AsIntentSampler>", BlueprintAutocast))
    static FCk_Handle_IntentSampler
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid IntentSampler Handle",
              Category = "Ck|Utils|IntentSampler",
              meta = (CompactNodeTitle = "INVALID_IntentSamplerHandle", Keywords = "make"))
    static FCk_Handle_IntentSampler
    Get_InvalidHandle() { return {}; }

public:
    /**
     * How many rows the ring is holding: it grows to the declared capacity and then stays there for the rest of
     * the sampler's life, because eviction replaces rather than removes.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Get Frame Count")
    static int32
    Get_FrameCount(
        const FCk_Handle_IntentSampler& InSampler);

    /**
     * The physical keys currently down, as of the last sampling pass — the key-level fact the row's `_Held` set
     * cannot carry, since that one is resolved to button identities.
     *
     * LIVE rather than per-row, the same reading the delivery check makes: a held key routes nothing after its
     * press edge, so no row records that it is still down. Under catch-up several rows are matched against this
     * one answer, which is the *Hitch attribution* cost showing through one more surface.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Get Held Keys")
    static TArray<FKey>
    Get_HeldKeys(
        const FCk_Handle_IntentSampler& InSampler);

    /** The most recently written row, or the no-such-frame row before the first sampling pass has run. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Get Latest Frame")
    static FCk_Intent_FrameRecord
    Get_LatestFrame(
        const FCk_Handle_IntentSampler& InSampler);

    /**
     * A row addressed backwards from the latest: offset 0 is the newest, offset FrameCount-1 the oldest retained.
     * Offsets are relative on purpose — a row's storage slot moves as the ring wraps, and an absolute index would
     * name a different frame depending on when it was asked.
     *
     * An offset outside the retained range — negative, past the count, or anything at all on an empty ring —
     * answers with a row whose FrameIndex is negative. A real row always carries a real index, so the caller
     * cannot mistake the two, and there is no separate success flag to forget to check.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IntentSampler",
              DisplayName = "[Ck][IntentSampler] Try Get Frame At Offset")
    static FCk_Intent_FrameRecord
    TryGet_FrameAtOffset(
        const FCk_Handle_IntentSampler& InSampler,
        int32 InOffset);

private:
    // Composition is atomic: one rejected value invalidates the whole declaration, so this runs every rejection
    // before anything is composed.
    static auto
    DoGet_ParamsAreValid(
        const FCk_Handle& InContext,
        const FCk_Fragment_IntentSampler_ParamsData& InParams) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

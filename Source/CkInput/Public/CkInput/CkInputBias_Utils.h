#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInputBias_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkInputBias_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The per-game conditioning stage between a raw device event and the value gameplay acts on: deadzone, response
 * curve, sensitivity and inversion, declared per axis key.
 *
 * The feature lives ON the input-source entity rather than beside it, because conditioning is a property of that
 * one player's device stream and every query here answers "what is this source's axis reading now".
 *
 * Conditioning NEVER rewrites the raw event. The inbox row a producer wrote and the record made from it stay
 * verbatim — a recorded stick position is a physical fact, and a row that was already conditioned could not be
 * re-conditioned by a later retune or read back by a consumer that needs the fact. The conditioned values live
 * in their own state alongside, and that is what a consumer samples.
 *
 * An axis with no declared bias passes through as an identity: same value, still recorded, still sampled. Buttons
 * are not conditioned at all.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_InputBias"))
class CKINPUT_API UCk_Utils_InputBias_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InputBias_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InputBias);

public:
    /**
     * Composes the conditioning stage onto an input source. Rejects a handle that is not one, and rejects the
     * whole composition — leaving nothing behind — if any declared bias row is out of range or names an axis
     * another row already claimed.
     *
     * There is no Create: a conditioning stage on a child entity would have no inbox to read.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Add Feature")
    static FCk_Handle_InputBias
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_InputBias_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InputBias
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Handle -> InputBias Handle",
              meta = (CompactNodeTitle = "<AsInputBias>", BlueprintAutocast))
    static FCk_Handle_InputBias
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid InputBias Handle",
              Category = "Ck|Utils|InputBias",
              meta = (CompactNodeTitle = "INVALID_InputBiasHandle", Keywords = "make"))
    static FCk_Handle_InputBias
    Get_InvalidHandle() { return {}; }

public:
    /** The live conditioning table. A retune enqueued this frame is absent here until its request drains. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Get Axis Biases")
    static TArray<FCk_InputBias_AxisBias>
    Get_AxisBiases(
        const FCk_Handle_InputBias& InInputBias);

    /**
     * The bias declared for one axis. An axis with none comes back as a row whose AxisKey is INVALID rather than
     * as an identity row — a stored row always names a real key, so the two are never confused, and a caller that
     * wants "identity when absent" gets it for free by conditioning through Get_ConditionedAxisValue instead.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Try Get Axis Bias")
    static FCk_InputBias_AxisBias
    TryGet_AxisBias(
        const FCk_Handle_InputBias& InInputBias,
        const FKey& InAxisKey);

    /**
     * What consumers sample: the last raw value of this axis put through its bias, or through the identity when
     * the axis has none. Zero for an axis no event has ever arrived for — a conditioned value is derived from
     * samples, and with no samples the neutral reading is the only honest one.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Get Conditioned Axis Value")
    static float
    Get_ConditionedAxisValue(
        const FCk_Handle_InputBias& InInputBias,
        const FKey& InAxisKey);

    /**
     * The value the producer actually reported for this axis, kept beside the conditioned one. Zero for an axis
     * no event has ever arrived for.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Get Last Raw Axis Value")
    static float
    Get_LastRawAxisValue(
        const FCk_Handle_InputBias& InInputBias,
        const FKey& InAxisKey);

public:
    /**
     * Declares or replaces the bias for one axis. Deferred: the retune lands when the request drains, which is
     * before the next conditioning pass and after the current one — so it first affects the NEXT event to arrive
     * and never re-conditions a sample already taken.
     *
     * An out-of-range row (deadzone outside [0, 1), exponent or sensitivity at or below zero) or one naming an
     * invalid key is rejected before enqueue and completes Failed_NotEnqueued, leaving the table untouched.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputBias",
              DisplayName = "[Ck][InputBias] Request Set Axis Bias",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputBias
    Request_SetAxisBias(
        UPARAM(ref) FCk_Handle_InputBias& InInputBias,
        const FCk_Request_InputBias_SetAxisBias& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

private:
    static auto
    DoGet_AxisBiasIsValid(
        const FCk_Handle& InContext,
        const FCk_InputBias_AxisBias& InAxisBias) -> bool;

    // Composition is atomic: one rejected row invalidates the whole declaration, so this runs every row's
    // rejection before anything is composed.
    static auto
    DoGet_AxisBiasesAreValid(
        const FCk_Handle& InContext,
        const TArray<FCk_InputBias_AxisBias>& InAxisBiases) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

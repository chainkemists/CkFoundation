#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkInput/CkInputLayer_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkInputLayer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The layer stack of one input source. Any entity becomes a layer by carrying this feature; a layer declares which
 * inputs it is interested in as capture ROWS and what happens to the layers below it when one matches.
 *
 * Routing walks a source's layers from the highest priority down. The first matching Consume capture ends the walk,
 * so nothing below it sees the event; a PassThrough capture is delivered and the walk continues. A capture carries
 * no callback and returns nothing — a layer whose own state decides whether it consumes adds or removes captures
 * instead, which makes the live arbitration set inspectable and removes the "matched but declined" ambiguity.
 *
 * Capture edits are deferred, so an edit made during frame N's routing is first honoured by frame N+1's routing.
 * A modal pushed by this frame's Escape therefore does not swallow the rest of this frame's input.
 *
 * Press and release are paired by the ROUTER, not by the layers: whichever layer consumed a key's press receives
 * that key's release even if its captures changed or the layer was destroyed meanwhile, and the layers below never
 * receive a release for a press they never saw.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_InputLayer"))
class CKINPUT_API UCk_Utils_InputLayer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InputLayer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InputLayer);

public:
    /** Composes a layer onto an existing entity. Rejects when the requested priority is already taken on that source. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Add Feature")
    static FCk_Handle_InputLayer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_InputLayer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Create")
    static FCk_Handle_InputLayer
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_InputLayer_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InputLayer
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Handle -> InputLayer Handle",
              meta = (CompactNodeTitle = "<AsInputLayer>", BlueprintAutocast))
    static FCk_Handle_InputLayer
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid InputLayer Handle",
              Category = "Ck|Utils|InputLayer",
              meta = (CompactNodeTitle = "INVALID_InputLayerHandle", Keywords = "make"))
    static FCk_Handle_InputLayer
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Make Key Capture")
    static FCk_InputLayer_Capture
    Make_KeyCapture(
        const FKey& InKey,
        ECk_InputLayer_CaptureBehavior InBehavior);

    /** Matches every event reaching the layer. A catch-all Consume on a top layer IS suspension, structurally. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Make Catch All Capture")
    static FCk_InputLayer_Capture
    Make_CatchAllCapture(
        ECk_InputLayer_CaptureBehavior InBehavior);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Priority")
    static int32
    Get_Priority(
        const FCk_Handle_InputLayer& InLayer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Input Source")
    static FCk_Handle_InputSource
    Get_InputSource(
        const FCk_Handle_InputLayer& InLayer);

    /** The layer's live capture rows. A capture enqueued this frame is absent here until its request drains. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Captures")
    static TArray<FCk_InputLayer_Capture>
    Get_Captures(
        const FCk_Handle_InputLayer& InLayer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Num Captures")
    static int32
    Get_NumCaptures(
        const FCk_Handle_InputLayer& InLayer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Has Capture For Key")
    static bool
    Get_HasCaptureForKey(
        const FCk_Handle_InputLayer& InLayer,
        ECk_InputLayer_CaptureMatch InMatchMode,
        const FKey& InKey);

    /** The priority the global-action layer occupies. No ordinary layer may claim it. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Global Action Priority")
    static int32
    Get_GlobalActionPriority();

    /** The layer holding a given priority on a source, or an invalid handle when that priority is free. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Try Get Layer With Priority")
    static FCk_Handle_InputLayer
    TryGet_LayerWithPriority(
        FCk_Handle_InputSource InInputSource,
        int32 InPriority);

    /** Invalid until the source's first global action is registered. Bind OnCaptureTriggered on it to receive them. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Try Get Global Action Layer")
    static FCk_Handle_InputLayer
    TryGet_GlobalActionLayer(
        FCk_Handle_InputSource InInputSource);

    /**
     * Which layer owns the in-flight press of a key, or an invalid handle when no layer consumed one. The entry
     * survives the owner's destruction on purpose: it is what stops the release reaching the layers below.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Try Get Press Owner")
    static FCk_Handle_InputLayer
    TryGet_PressOwner(
        FCk_Handle_InputSource InInputSource,
        const FKey& InKey);

    /**
     * Every event this frame's routing pass delivered, each paired with the outcome the walk reached. Empty on a
     * frame that routed nothing, and empty on a source whose router state has not been stamped yet.
     *
     * READ ORDER MATTERS: this is per-frame state the router clears at the top of every pass, so only a caller
     * ordered AFTER FGroup_Input_Route sees this frame's rows. Gameplay qualifies (routing runs ahead of it by
     * group edge); anything inside FGroup_Input_Collect or FGroup_Input_Bias reads the cleared array.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Get Routed Events This Frame")
    static TArray<FCk_InputLayer_RoutedEvent>
    Get_RoutedEventsThisFrame(
        FCk_Handle_InputSource InInputSource);

public:
    /**
     * Declares interest in an input. Deferred: the capture joins the layer when the request drains, which is before
     * the next routing pass and after the current one — the one-frame transition contract.
     * A capture already declared for the same match mode and key has its behavior replaced.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Request Add Capture",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputLayer
    Request_AddCapture(
        UPARAM(ref) FCk_Handle_InputLayer& InLayer,
        const FCk_Request_InputLayer_AddCapture& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Request Remove Capture",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputLayer
    Request_RemoveCapture(
        UPARAM(ref) FCk_Handle_InputLayer& InLayer,
        const FCk_Request_InputLayer_RemoveCapture& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    /**
     * Binds a raw key at the bottom of the source's stack — no binding profile, no intent definition, no bake. It
     * fires only when no layer above consumed the key, which is what makes a debug key stop working while a modal
     * is up without the modal knowing the debug key exists. The reserved bottom layer is created on first use.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Request Add Global Action",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_InputSource
    Request_AddGlobalAction(
        UPARAM(ref) FCk_Handle_InputSource& InInputSource,
        const FCk_Request_InputLayer_AddGlobalAction& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Bind To OnCaptureTriggered")
    static FCk_Handle_InputLayer
    BindTo_OnCaptureTriggered(
        UPARAM(ref) FCk_Handle_InputLayer& InLayer,
        const FCk_Delegate_InputLayer_CaptureTriggered& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputLayer",
              DisplayName = "[Ck][InputLayer] Unbind From OnCaptureTriggered")
    static FCk_Handle_InputLayer
    UnbindFrom_OnCaptureTriggered(
        UPARAM(ref) FCk_Handle_InputLayer& InLayer,
        const FCk_Delegate_InputLayer_CaptureTriggered& InDelegate);

private:
    // Runs every rejection the registration can suffer BEFORE anything is composed or created, so a rejected
    // registration leaves neither a half-composed entity nor an empty child behind.
    static auto
    DoGet_RegistrationIsValid(
        const FCk_Handle& InContext,
        const FCk_Fragment_InputLayer_ParamsData& InParams) -> bool;

    static auto
    DoAddLayer(
        FCk_Handle& InHandle,
        const FCk_Fragment_InputLayer_ParamsData& InParams) -> FCk_Handle_InputLayer;

    static auto
    DoCreateLayer(
        FCk_Handle& InOwner,
        const FCk_Fragment_InputLayer_ParamsData& InParams) -> FCk_Handle_InputLayer;
};

// --------------------------------------------------------------------------------------------------------------------

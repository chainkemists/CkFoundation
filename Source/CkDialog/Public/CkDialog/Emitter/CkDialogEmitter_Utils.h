#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkDialog/Emitter/CkDialogEmitter_Fragment.h"
#include "CkDialog/Emitter/CkDialogEmitter_Fragment_Data.h"
#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

#include "CkDialogEmitter_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_DialogEmitter"))
class CKDIALOG_API UCk_Utils_DialogEmitter_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DialogEmitter_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_DialogEmitter);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Add")
    static FCk_Handle_DialogEmitter
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_DialogEmitter_Spec& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Dialog|Emitter",
        DisplayName="[Ck][Dialog][Emitter] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_DialogEmitter
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Dialog|Emitter",
        DisplayName="[Ck][Dialog][Emitter] Handle -> DialogEmitter Handle",
        meta = (CompactNodeTitle = "<AsDialogEmitter>", BlueprintAutocast))
    static FCk_Handle_DialogEmitter
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid DialogEmitter Handle",
        Category = "Ck|Utils|Dialog|Emitter",
        meta = (CompactNodeTitle = "INVALID_DialogEmitterHandle", Keywords = "make"))
    static FCk_Handle_DialogEmitter
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Get Emitter Tags")
    static FGameplayTagContainer
    Get_EmitterTags(
        const FCk_Handle_DialogEmitter& InEmitter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Get Is Line On Cooldown")
    static bool
    Get_IsLineOnCooldown(
        const FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Handle_DialogLine& InLine);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Get Active Cooldowns")
    static TArray<FCk_Handle_DialogLine>
    Get_ActiveCooldowns(
        const FCk_Handle_DialogEmitter& InEmitter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Get Cooldown Remaining")
    static FCk_Time
    Get_CooldownRemaining(
        const FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Handle_DialogLine& InLine);

    // The full cooldown record: the Chrono (goal = the duration it was started with, elapsed = progress) plus the
    // mode. Get_CooldownRemaining answers "how long left"; this is what answers "how far through" and "is this a
    // Forever". Returns a default entry (zero goal, Timed) when the line is not cooling.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Get Cooldown Entry")
    static FCk_DialogEmitter_CooldownEntry
    Get_CooldownEntry(
        const FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Handle_DialogLine& InLine);

public:
    // A query is drained in two stages — queued by HandleRequests, answered by EvaluateQueries — so InDelegate
    // completes when the query is EVALUATED, not when it is accepted. Evaluation is deferred while the Dialog
    // registry is not ready, and an emitter destroyed with queries still pending completes them Failed_Cancelled.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Query",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_DialogEmitter
    Request_Query(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Request_DialogEmitter_Query InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Reads the played line's LinkedEventTag and, when valid, enqueues a Query{LinkedEventTag}. Invalid line / no exit => no-op
    // (Display log) and InDelegate completes Failed_NotEnqueued. The chaining convenience wrapper over Request_Query.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Query Follow-Up",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_DialogEmitter
    Request_QueryFollowUp(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Handle_DialogLine InPlayedLine,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Start Cooldown",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_DialogEmitter
    Request_StartCooldown(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Request_DialogEmitter_StartCooldown InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Clearing a line that was not cooling removes nothing and completes InDelegate with Failed.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Clear Cooldown",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_DialogEmitter
    Request_ClearCooldown(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Handle_DialogLine InLine,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Clear All Cooldowns",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_DialogEmitter
    Request_ClearAllCooldowns(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName = "[Ck][Dialog][Emitter] Bind To OnQueryCompleted")
    static FCk_Handle_DialogEmitter
    BindTo_OnQueryCompleted(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnQueryCompleted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName = "[Ck][Dialog][Emitter] Unbind From OnQueryCompleted")
    static FCk_Handle_DialogEmitter
    UnbindFrom_OnQueryCompleted(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnQueryCompleted& InDelegate);

    // Fires when a line STARTS cooling on this emitter — including a re-start, which replaces the existing entry.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName = "[Ck][Dialog][Emitter] Bind To OnCooldownStarted")
    static FCk_Handle_DialogEmitter
    BindTo_OnCooldownStarted(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownStarted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName = "[Ck][Dialog][Emitter] Unbind From OnCooldownStarted")
    static FCk_Handle_DialogEmitter
    UnbindFrom_OnCooldownStarted(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownStarted& InDelegate);

    // Fires when a line STOPS cooling — whether it lapsed on its own or was cleared explicitly. A clear that
    // removes nothing is not a transition and does not fire.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName = "[Ck][Dialog][Emitter] Bind To OnCooldownEnded")
    static FCk_Handle_DialogEmitter
    BindTo_OnCooldownEnded(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownEnded& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName = "[Ck][Dialog][Emitter] Unbind From OnCooldownEnded")
    static FCk_Handle_DialogEmitter
    UnbindFrom_OnCooldownEnded(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        const FCk_Delegate_DialogEmitter_OnCooldownEnded& InDelegate);

public:
    // Opt-in selection helper (pure): from a completed query result pick one line — Passed entries only, the
    // most-conditions tier (a specificity proxy), seeded-random tie-break. Selection stays OUT of the query itself.
    // Returns a default entry (invalid _Line) when no line Passed.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Get Best Line")
    static FCk_DialogLine_QueryEntry
    Get_BestLine(
        const FCk_DialogEmitter_QueryResult& InResult,
        int32 InSeed);
};

// --------------------------------------------------------------------------------------------------------------------

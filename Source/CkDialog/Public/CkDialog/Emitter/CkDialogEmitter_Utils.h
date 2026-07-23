#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

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
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Dialog][Emitter] Add")
    static FCk_Handle_DialogEmitter
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_DialogEmitter_ParamsData& InParams);

public:
    // Has Feature
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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Dialog][Emitter] Request Query")
    static FCk_Handle_DialogEmitter
    Request_Query(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Request_DialogEmitter_Query InRequest);

    // Reads the played line's LinkedEventTag and, when valid, enqueues a Query{LinkedEventTag}. Invalid line / no exit => no-op
    // (Display log). The chaining convenience wrapper over Request_Query.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Query Follow-Up")
    static FCk_Handle_DialogEmitter
    Request_QueryFollowUp(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Handle_DialogLine InPlayedLine);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Dialog][Emitter] Request Start Cooldown")
    static FCk_Handle_DialogEmitter
    Request_StartCooldown(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Request_DialogEmitter_StartCooldown InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Clear Cooldown")
    static FCk_Handle_DialogEmitter
    Request_ClearCooldown(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter,
        FCk_Handle_DialogLine InLine);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Dialog|Emitter",
              DisplayName="[Ck][Dialog][Emitter] Request Clear All Cooldowns")
    static FCk_Handle_DialogEmitter
    Request_ClearAllCooldowns(
        UPARAM(ref) FCk_Handle_DialogEmitter& InEmitter);

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

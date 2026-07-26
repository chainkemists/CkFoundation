#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkRewindHistory_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_RewindHistory_Record; }

// --------------------------------------------------------------------------------------------------------------------

/**
 * Server-side hitbox history for lag compensation. The record processor snapshots the entity's
 * labelled hit shapes (posed on its transform) into a ring buffer at a fixed cadence; queries
 * interpolate past poses and run analytic sweeps against them — the basis for validating shots
 * against what the shooter actually saw.
 *
 * Requires a Transform on the entity. Recording happens on the authority only.
 */
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RewindHistory"))
class CKLAGCOMPENSATION_API UCk_Utils_RewindHistory_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RewindHistory_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RewindHistory);

public:
    friend class ck::FProcessor_RewindHistory_Record;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|RewindHistory",
              DisplayName="[Ck][RewindHistory] Add Feature")
    static FCk_Handle_RewindHistory
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_RewindHistory_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|RewindHistory",
              DisplayName="[Ck][RewindHistory] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_RewindHistory
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Handle -> RewindHistory Handle",
        meta = (CompactNodeTitle = "<AsRewindHistory>", BlueprintAutocast))
    static FCk_Handle_RewindHistory
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid RewindHistory Handle",
        Category = "Ck|Utils|RewindHistory",
        meta = (CompactNodeTitle = "INVALID_RewindHistoryHandle", Keywords = "make"))
    static FCk_Handle_RewindHistory
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Get Recorded Frame Count")
    static int32
    Get_RecordedFrameCount(
        const FCk_Handle_RewindHistory& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Get Newest Frame Time")
    static FCk_Time
    Get_NewestFrameTime(
        const FCk_Handle_RewindHistory& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Get Oldest Frame Time")
    static FCk_Time
    Get_OldestFrameTime(
        const FCk_Handle_RewindHistory& InHandle);

    // Hit shape poses interpolated at a (typically past) world time. Empty if the history
    // cannot bracket the requested time
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Get Interpolated Snapshots At Time")
    static TArray<FCk_LagComp_HitShapeSnapshot>
    Get_InterpolatedSnapshotsAtTime(
        const FCk_Handle_RewindHistory& InHandle,
        FCk_Time InWorldTime);

    // Sweep a (radius-inflated) segment travelled over [InTimeStart, InTimeEnd] against this
    // entity's recorded history. OutHit is valid only when the return value is true
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Sweep Against History")
    static bool
    Sweep_AgainstHistory(
        const FCk_Handle_RewindHistory& InHandle,
        const FVector& InSegmentStart,
        const FVector& InSegmentEnd,
        FCk_Time InTimeStart,
        FCk_Time InTimeEnd,
        float InSweepRadius,
        FCk_LagComp_RewindHit& OutHit);

public:
    // Record a frame on the next record pass regardless of the record interval
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|RewindHistory",
        DisplayName="[Ck][RewindHistory] Request Force Record Frame")
    static FCk_Handle_RewindHistory
    Request_ForceRecordFrame(
        UPARAM(ref) FCk_Handle_RewindHistory& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

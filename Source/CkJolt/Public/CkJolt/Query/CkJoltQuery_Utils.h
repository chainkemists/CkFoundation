#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/Query/CkJoltQuery_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkJoltQuery_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/// UE-channel-equivalent scene queries against the Jolt world (static-world bodies, probes, and —
/// from Phase 3 — JoltBodies). Pure reads (`Get_` — no overlap side effects, unlike CkSpatialQuery's
/// `Request_*LineTrace` family which can fire probe overlaps).
UCLASS(NotBlueprintable)
class CKJOLT_API UCk_Utils_JoltQuery_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltQuery_UE);

public:
    /// Closest blocking hit along a ray, honoring UE channel responses.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Ray Cast (Closest, By Channel)")
    static FCk_Jolt_HitResult
    Get_RayCast(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FCk_Jolt_QueryFilter InFilter);

    /// Closest hit sweeping a shape from Start to End, honoring UE channel responses.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Shape Cast (Closest, By Channel)")
    static FCk_Jolt_HitResult
    Get_ShapeCast(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter);

    /// All bodies overlapping a shape placed at a location. Pass MinResponse = Overlap in the
    /// filter for UE overlap semantics (the default filter is Block = trace semantics).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Overlap (All, By Channel)")
    static TArray<FCk_Jolt_HitResult>
    Get_Overlap(
        const UObject* InWorldContextObject,
        FVector InLocation,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter);

    /// All passing hits along a ray, sorted near->far, honoring UE channel responses.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Ray Cast (Multi, By Channel)")
    static TArray<FCk_Jolt_HitResult>
    Get_RayCastMulti(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FCk_Jolt_QueryFilter InFilter);

    /// All hits sweeping a shape from Start to End, sorted near->far, honoring UE channel responses.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Shape Cast (Multi, By Channel)")
    static TArray<FCk_Jolt_HitResult>
    Get_ShapeCastMulti(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter);

    /// Live entities whose bodies overlap a shape placed at a location (deduped). Bodies with no live
    /// entity (e.g. baked static-world bodies) are dropped. Pass MinResponse = Overlap in the filter for
    /// UE overlap semantics (the default filter is Block = trace semantics).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Jolt",
              meta = (WorldContext = "InWorldContextObject"),
              DisplayName="[Ck][Jolt] Overlap Entities (By Channel)")
    static TArray<FCk_Handle>
    Get_OverlapEntities(
        const UObject* InWorldContextObject,
        FVector InLocation,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter);
};

// --------------------------------------------------------------------------------------------------------------------

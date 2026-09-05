#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Fragment.h"

#include "CkGroundNav_Shadow_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// The world-facing surface of the shadow diagnostics: open a fixture, close it, read the report.
//
// The diagnostics live on the world's transient entity, so every function here resolves a world from
// its context object and reaches that entity. This is the only place in Shadow/ - along with the
// console command - that knows a world exists at all; the accumulator and the report builder below it
// are fed values and know nothing.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKGROUNDNAV_API UCk_Utils_GroundNav_Shadow_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GroundNav_Shadow_UE);

public:
    /** Name the fixture every comparison from here on is bucketed under. A run that never opens one
     *  buckets under the world's map name instead, which is enough for a gym session but collapses an
     *  entire test suite sharing one world into a single row. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Request Begin Shadow Fixture",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_BeginShadowFixture(
        const UObject* InWorldContextObject,
        FName InFixture);

    /** Close the open fixture. Its row survives - closing stops new comparisons landing in it, it does
     *  not discard what it already recorded. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Request End Shadow Fixture",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_EndShadowFixture(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Get Shadow Report",
              meta = (WorldContext = "InWorldContextObject"))
    static FString
    Get_ShadowReport(
        const UObject* InWorldContextObject);

    /** How many comparisons a named fixture has recorded, or zero when it has never been opened. Zero
     *  is the honest answer for both, because a caller asking this is asking whether the run moved. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Get Shadow Comparison Count",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Get_ShadowComparisonCount(
        const UObject* InWorldContextObject,
        FName InFixture);

    /** Every containment escape the run has banked, summed over every fixture rather than asked of
     *  one. The producer is the crowd's single Transform writer and it counts per AGENT per frame,
     *  so an escape is not attributable to a query and there is no fixture a caller could name for
     *  it; what the sum answers is whether the two providers ever disagreed about the ground a body
     *  was standing on. Zero for a run that never shadowed, which is the honest reading. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Get Shadow Containment Escapes",
              meta = (WorldContext = "InWorldContextObject"))
    static int64
    Get_ShadowContainmentEscapes(
        const UObject* InWorldContextObject);

    /** Drop every fixture, every diverging id and the open fixture name. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Request Reset Shadow Diagnostics",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_ResetShadowDiagnostics(
        const UObject* InWorldContextObject);

    /** The key comparisons bucket under while no fixture is open: the world's map name. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GroundNav|Shadow",
              DisplayName = "[Ck][GroundNav][Shadow] Get Fallback Fixture Key",
              meta = (WorldContext = "InWorldContextObject"))
    static FName
    Get_FallbackFixtureKey(
        const UObject* InWorldContextObject);
};

// --------------------------------------------------------------------------------------------------------------------

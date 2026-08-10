#pragma once

#include "CkEqs/Query/CkEqs_Fragment_Data.h"
#include "CkEqs/Query/CkEqs_Fragment.h"

#include "CkSpatialQuery/Probe/CkProbeTrace_Context.h"

// --------------------------------------------------------------------------------------------------------------------
// Statics so the deferred processors and Request_RunQuery_Immediate share one implementation. The
// struct is the friend-grant target letting them write FCk_Eqs_Candidate / QueryResults / State.
// --------------------------------------------------------------------------------------------------------------------

struct CKEQS_API FCk_Eqs_Algorithm
{
    // False on failed generation; the caller owns the failure-tag flow.
    static auto
    DoGenerate(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        const FCk_ProbeTrace_Context& InContext) -> bool;

    // Resumes from InState._NextTestIndex; true only when every test finished this call, false
    // means it yielded at a test boundary with the cursor preserved. Tests are atomic — never a
    // mid-test yield — and a query with no progress runs one test regardless of budget.
    static auto
    DoRunTests(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        const FCk_ProbeTrace_Context& InContext,
        int32& InOutRemainingBudget) -> bool;

    // InOutDebug._PerCandidate is reordered/truncated in lockstep with the results, so a
    // post-Finalize debugger lookup by results-index is correct.
    static auto
    DoFinalize(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        FFragment_EqsQuery_DebugInfo& InOutDebug) -> FCk_Eqs_QueryResults;

    // ----------------------------------------------------------------------------------------------------------------

    static auto
    DoGenerate_SimpleGrid(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        TArray<FCk_Eqs_Candidate>& OutCandidates) -> void;

    static auto
    DoGenerate_Grid(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        const FCk_Handle& InAnyHandle,
        const FCk_ProbeTrace_Context& InContext,
        TArray<FCk_Eqs_Candidate>& OutCandidates) -> void;

    static auto
    DoGenerate_Donut(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        TArray<FCk_Eqs_Candidate>& OutCandidates) -> void;

    static auto
    DoGenerate_Cone(
        const FCk_Eqs_GeneratorParams& InGen,
        const FTransform& InQuerierTransform,
        TArray<FCk_Eqs_Candidate>& OutCandidates) -> void;

    static auto
    DoGenerate_EntitiesWithTag(
        const FCk_Eqs_GeneratorParams& InGen,
        const FCk_Handle& InQuerier,
        TArray<FCk_Eqs_Candidate>& OutCandidates) -> void;

    static auto
    DoGenerate_OnCircle(
        const FCk_Eqs_GeneratorParams& InGen,
        const FVector& InQuerierLocation,
        TArray<FCk_Eqs_Candidate>& OutCandidates) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // One helper per ECk_Eqs_TestType, all on the three-pass contract: raw value per candidate →
    // Min/Max across the candidates that produced one → FinishTest scores and filters. GameplayTag
    // is the exception — filter-only, contribution 1.0f, writes its own debug rows directly.
    // ----------------------------------------------------------------------------------------------------------------

    static auto
    DoRunTest_Distance(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Eqs_QueryParams& InParams,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    static auto
    DoRunTest_Dot(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Eqs_QueryParams& InParams,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    static auto
    DoRunTest_Trace(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Eqs_QueryParams& InParams,
        const FCk_Handle& InAnyHandle,
        const FCk_ProbeTrace_Context& InContext,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    static auto
    DoRunTest_GameplayTag(
        const FCk_Eqs_TestParams& InTest,
        FCk_Handle_EqsQuery InQueryHandle,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    static auto
    DoRunTest_Overlap(
        const FCk_Eqs_TestParams& InTest,
        const FCk_Handle& InAnyHandle,
        const FCk_ProbeTrace_Context& InContext,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    static auto
    DoRunTest_VolumeCheck(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    // Zero rotation degenerates to a plain AABB check.
    static auto
    Get_IsPointInOrientedBox(
        const FVector& InPoint,
        const FVector& InCenter,
        const FVector& InHalfExtent,
        const FRotator& InRotation) -> bool;

    static auto
    DoRunTest_Random(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Port of UE EnvQueryTest.cpp::NormalizeItemScores, minus the reference-value path. InMin/InMax
    // span every contributing candidate; a degenerate range returns a neutral 1.0f * ScoringFactor.
    // ----------------------------------------------------------------------------------------------------------------

    static auto
    NormalizeAndScore(
        float InValue,
        float InMin,
        float InMax,
        const FCk_Eqs_ScoringConfig& InConfig) -> float;

private:
    // A member rather than a file-local helper so it inherits the FCk_Eqs_Candidate friend grant.
    // Writes the normalized/weighted/passed debug fields; the raw one is the caller's.
    static auto
    FinishTest(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        const TArray<float>& InRawValues,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

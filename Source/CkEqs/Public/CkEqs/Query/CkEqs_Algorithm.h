#pragma once

#include "CkEqs/Query/CkEqs_Fragment_Data.h"
#include "CkEqs/Query/CkEqs_Fragment.h"

#include <Templates/SharedPointer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace JPH { class PhysicsSystem; }

// --------------------------------------------------------------------------------------------------------------------
// FCk_Eqs_Algorithm — the core math/logic of the EQS module.
//
// The processors in CkEqs_Processor.{h,cpp} are thin dispatchers; all generator math,
// test scoring, and finalization logic lives here as static free functions on this
// tag-struct. This is the mechanism that lets Request_RunQuery_Immediate and the
// processor pipeline share identical behaviour without "manually invoking ForEachEntity."
//
// All helpers are static. The struct exists only as a friend-grant target for
// FCk_Eqs_Candidate / FCk_Eqs_QueryResults / FFragment_EqsQuery_State so we can mutate
// their private fields (_Score / _Passed / _Location / _Candidates / _NextTestIndex)
// without having to leak a public mutator surface to the rest of the codebase.
// --------------------------------------------------------------------------------------------------------------------

struct CKEQS_API FCk_Eqs_Algorithm
{
    // Generate candidates based on InParams.GeneratorParams. Writes into InState._Candidates.
    // Returns false if generation failed (invalid querier, no transform, unknown generator, etc.).
    // Caller is responsible for the failure-tag flow on false return.
    static auto
    DoGenerate(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem) -> bool;

    // Run tests in InParams.Tests on the candidates in InState, resuming from InState._NextTestIndex.
    // Decrements InOutRemainingBudget per processed candidate. Returns true if all tests
    // completed in this call; false if it yielded at a test boundary because the budget
    // was exhausted (cursor preserved in InState for resume next frame).
    //
    // Tests are atomic — yields happen between tests, never mid-test.
    // Anti-deadlock: if no progress has been made this tick AND the next test
    // would exceed the remaining budget, run that one test anyway. Otherwise queries
    // larger than the per-frame budget never advance.
    //
    // InOutDebug is sized lazily on first entry to match (NumCandidates × NumTests)
    // and populated per candidate per test (raw / normalized / weighted / passed). Index
    // ordering matches InState._Candidates pre-Finalize; DoFinalize reorders in lockstep.
    static auto
    DoRunTests(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
        int32& InOutRemainingBudget) -> bool;

    // Build the final FCk_Eqs_QueryResults from state: drop failed candidates, sort,
    // apply RunMode truncation, populate BestLocation / BestEntity / HasResults.
    //
    // InOutDebug._PerCandidate is reordered/truncated in lockstep with the
    // results so post-Finalize debugger lookup by results-index is correct.
    static auto
    DoFinalize(
        FCk_Handle_EqsQuery InQueryHandle,
        const FCk_Eqs_QueryParams& InParams,
        FFragment_EqsQuery_State& InState,
        FFragment_EqsQuery_DebugInfo& InOutDebug) -> FCk_Eqs_QueryResults;

    // ----------------------------------------------------------------------------------------------------------------
    // Generator helpers (declared public so processors / Immediate can dispatch directly,
    // and so future unit tests can poke them in isolation).
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
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
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
    // Test helpers (one per ECk_Eqs_TestType value). All follow the three-pass contract:
    //   1. Raw pass: compute raw value per candidate.
    //   2. Min/Max: across candidates that produced a meaningful raw value.
    //   3. Score+Filter: NormalizeAndScore + multiply into _Score; evaluate FilterConfig.
    // GameplayTag is the exception: it's filter-only with score contribution = 1.0f when passed.
    // ----------------------------------------------------------------------------------------------------------------

    // Each DoRunTest_* takes a debug fragment + test index so the per-test row
    // for each candidate gets populated (raw value during the test's raw pass; normalized /
    // weighted / passed inside FinishTest). GameplayTag is filter-only and writes its own
    // debug rows directly (no FinishTest path).

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
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
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
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    static auto
    DoRunTest_VolumeCheck(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;

    // Pure containment test shared by DoRunTest_VolumeCheck and unit tests.
    // Zero rotation degenerates to the legacy AABB check.
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
    // Scoring helper — port of UE EnvQueryTest.cpp::NormalizeItemScores, simplified branch
    // (no reference-value path; that's a v1.1 follow-up). Inputs:
    //   - InValue: this candidate's raw value for this test.
    //   - InMin / InMax: min and max raw values across all candidates that contributed.
    //   - InConfig: the test's ScoringConfig (equation, factor, normalization type, clamps).
    // Returns the normalized-and-equation-transformed score scaled by ScoringFactor.
    // Degenerate case (Min ≈ Max): returns 1.0f * ScoringFactor — scoring is neutral
    // (multiplicative-identity skip on the per-item loop is applied by the caller).
    // ----------------------------------------------------------------------------------------------------------------

    static auto
    NormalizeAndScore(
        float InValue,
        float InMin,
        float InMax,
        const FCk_Eqs_ScoringConfig& InConfig) -> float;

private:
    // Common Score+Filter pass for tests that follow the three-pass contract. Promoted
    // to a member so it inherits the FCk_Eqs_Candidate friend grant (anonymous-namespace
    // helpers don't get friend access through the struct's friend declaration).
    //
    // Writes per-candidate per-test debug entries (normalized score, weighted
    // contribution, passed-this-test flag). The raw-value field is written by each
    // DoRunTest_* during its raw pass before calling FinishTest.
    static auto
    FinishTest(
        const FCk_Eqs_TestParams& InTest,
        TArray<FCk_Eqs_Candidate>& InOutCandidates,
        const TArray<float>& InRawValues,
        FFragment_EqsQuery_DebugInfo& InOutDebug,
        int32 InTestIndex) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

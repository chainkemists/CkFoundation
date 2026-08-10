#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Ensure/CkEnsure_Tracker.h"

#include <Async/Async.h>
#include <Async/ParallelFor.h>
#include <Misc/AutomationTest.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Ensure_OccurrenceTracker,
    "Ck.CkCore.Ensure.OccurrenceTracker.RepeatsAndReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Ensure_OccurrenceTracker::RunTest(const FString&)
{
    auto Tracker = ck::ensure::FCk_EnsureOccurrenceTracker{};
    const auto NativeSignature = ck::ensure::FCk_EnsureSignature
    {
        TEXT("Native.cpp"),
        42,
        TEXT("Value != nullptr"),
        {},
    };

    constexpr auto RepeatCount = 1000;
    auto HeavyReportCount = 0;
    for (auto Index = 0; Index < RepeatCount; ++Index)
    {
        const auto& Record = Tracker.Record(NativeSignature);
        if (Record.IsFirstOccurrence)
        { ++HeavyReportCount; }
    }

    TestEqual(TEXT("Every occurrence is aggregated"), Tracker.GetTotalCount(), static_cast<uint64>(RepeatCount));
    TestEqual(TEXT("One stable signature is unique once"), Tracker.GetUniqueCount(), 1);
    TestEqual(TEXT("The stable signature retains its exact count"), Tracker.GetOccurrenceCount(NativeSignature), static_cast<uint64>(RepeatCount));
    TestEqual(TEXT("Only the first occurrence reaches the heavy reporter"), HeavyReportCount, 1);

    const auto DifferentExpression = ck::ensure::FCk_EnsureSignature
    {
        NativeSignature.File,
        NativeSignature.Line,
        TEXT("OtherValue != nullptr"),
        {},
    };
    const auto ScriptSignature = ck::ensure::FCk_EnsureSignature
    {
        NativeSignature.File,
        NativeSignature.Line,
        NativeSignature.Expression,
        TEXT("AS:UExample::Tick@Example.as:17:3"),
    };

    TestTrue(TEXT("A different expression receives its own first report"), Tracker.Record(DifferentExpression).IsFirstOccurrence);
    TestTrue(TEXT("A script callsite receives its own first report"), Tracker.Record(ScriptSignature).IsFirstOccurrence);
    TestEqual(TEXT("Expression and script identity are part of the signature"), Tracker.GetUniqueCount(), 3);

    const auto Snapshot = Tracker.ConsumeSnapshotAndReset();
    TestEqual(TEXT("Reset returns one row per distinct signature"), Snapshot.Num(), 3);
    TestEqual(TEXT("Reset clears the aggregate total"), Tracker.GetTotalCount(), static_cast<uint64>(0));
    TestEqual(TEXT("Reset clears the unique count"), Tracker.GetUniqueCount(), 0);
    TestTrue(TEXT("A new epoch reports the signature fully again"), Tracker.Record(NativeSignature).IsFirstOccurrence);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Ensure_OccurrenceTracker_Concurrent,
    "Ck.CkCore.Ensure.OccurrenceTracker.Concurrent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Ensure_OccurrenceTracker_Concurrent::RunTest(const FString&)
{
    auto Tracker = ck::ensure::FCk_EnsureOccurrenceTracker{};
    const auto Signature = ck::ensure::FCk_EnsureSignature
    {
        TEXT("Worker.cpp"),
        7,
        TEXT("State.IsValid()"),
        {},
    };

    constexpr auto WorkerHitCount = 2048;
    std::atomic<int32> FirstWinnerCount = 0;
    ParallelFor(WorkerHitCount, [&](int32)
    {
        if (Tracker.Record(Signature).IsFirstOccurrence)
        { ++FirstWinnerCount; }
    });

    TestEqual(TEXT("Concurrent aggregation retains every hit"), Tracker.GetTotalCount(), static_cast<uint64>(WorkerHitCount));
    TestEqual(TEXT("Concurrent aggregation retains one signature"), Tracker.GetUniqueCount(), 1);
    TestEqual(TEXT("Exactly one thread wins the first-report decision"), FirstWinnerCount.load(), 1);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Ensure_ScriptProvenanceDepth,
    "Ck.CkCore.Ensure.ScriptProvenanceDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Ensure_ScriptProvenanceDepth::RunTest(const FString&)
{
    TestFalse(TEXT("The game thread begins outside script ensure handling"), ck::ensure::Get_IsEnsureFromScript());

    ck::ensure::Do_Push_EnsureIsFromScript();
    ck::ensure::Do_Push_EnsureIsFromScript();
    TestTrue(TEXT("Nested pushes retain script provenance"), ck::ensure::Get_IsEnsureFromScript());

    ck::ensure::Do_Pop_EnsureIsFromScript();
    TestTrue(TEXT("The outer script scope remains after one pop"), ck::ensure::Get_IsEnsureFromScript());

    ck::ensure::Do_Pop_EnsureIsFromScript();
    TestFalse(TEXT("Balanced pops restore native provenance"), ck::ensure::Get_IsEnsureFromScript());

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Ensure_WorkerFirstReport,
    "Ck.CkCore.Ensure.WorkerFirstReport.NativeOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Ensure_WorkerFirstReport::RunTest(const FString&)
{
    const auto CountBefore = ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount();
    const auto UniqueFile = FName{*FString::Printf(
        TEXT("CkEnsureWorkerTest-%s.cpp"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits))};
    auto BreakInCode = false;
    auto BreakInScript = false;
    auto WorkerReport = FString{};
    std::atomic<bool> ReportEmitterRanOnWorker = false;
    auto Future = Async(EAsyncExecution::ThreadPool, [&, UniqueFile]()
    {
        ck::ensure::Ensure_Impl_ForTesting(
            TEXT("Worker ensure integration test"),
            TEXT("WorkerTestExpression"),
            UniqueFile,
            731,
            BreakInCode,
            BreakInScript,
            [&](const FString& InReport)
            {
                ReportEmitterRanOnWorker = NOT IsInGameThread();
                WorkerReport = InReport;
            });
    });
    Future.Wait();

    TestEqual(
        TEXT("The worker hit is aggregated"),
        ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount(),
        CountBefore + 1);
    TestFalse(TEXT("Worker reporting never requests a native debug break"), BreakInCode);
    TestFalse(TEXT("Worker reporting never requests a script debug break"), BreakInScript);
    TestTrue(TEXT("The report emitter runs on the originating worker"), ReportEmitterRanOnWorker.load());
    TestTrue(TEXT("The worker report retains the expression"), WorkerReport.Contains(TEXT("[Worker] WorkerTestExpression")));
    TestTrue(TEXT("The worker report includes a native call stack"), WorkerReport.Contains(TEXT("== CallStack ==")));
    TestFalse(TEXT("The worker report does not request a Blueprint stack"), WorkerReport.Contains(TEXT("== BP CallStack ==")));
    TestFalse(TEXT("The worker report does not request an AngelScript stack"), WorkerReport.Contains(TEXT("== AS CallStack ==")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

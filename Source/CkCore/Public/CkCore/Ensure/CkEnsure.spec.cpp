#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Ensure/CkEnsure_Tracker.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Settings/CkCore_Settings.h"

#include <Async/Async.h>
#include <Async/ParallelFor.h>
#include <CoreGlobals.h>
#include <Misc/AutomationTest.h>
#include <Misc/ScopeExit.h>
#include <Templates/UnrealTemplate.h>

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
            },
            [](const FCk_Utils_EditorOnly_PushNewEditorMessage_Params&)
            {
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Ensure_GameThreadRepeatSuppressionInteractiveOnly,
    "Ck.CkCore.Ensure.GameThreadRepeatSuppressionInteractiveOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Ensure_GameThreadRepeatSuppressionInteractiveOnly::RunTest(const FString&)
{
    const auto PreviousDisplayPolicy = UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy();
    const auto PreviousDetailsPolicy = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy();
    ON_SCOPE_EXIT
    {
        UCk_Utils_Core_UserSettings_UE::Set_EnsureDisplayPolicy(PreviousDisplayPolicy);
        UCk_Utils_Core_UserSettings_UE::Set_EnsureDetailsPolicy(PreviousDetailsPolicy);
    };

    // Exercise the editor-notification branch while the automation process remains unattended, so the real
    // implementation returns before any modal dialog. The injected emitter is the exact Message Log callsite.
    UCk_Utils_Core_UserSettings_UE::Set_EnsureDisplayPolicy(ECk_EnsureDisplay_Policy::ModalDialog);
    UCk_Utils_Core_UserSettings_UE::Set_EnsureDetailsPolicy(ECk_EnsureDetails_Policy::MessageOnly);

    const auto CountBefore = ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount();
    const auto UniqueBefore = ck::ensure::Get_EnsureOccurrenceTracker().GetUniqueCount();
    const auto UniqueFile = FName{*FString::Printf(
        TEXT("CkEnsureGameThreadTest-%s.cpp"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits))};
    auto EditorMessageCount = 0;
    auto BreakInCode = false;
    auto BreakInScript = false;

    const auto Invoke = [&](const FString& InExpression)
    {
        ck::ensure::Ensure_Impl_ForTesting(
            TEXT("Game-thread ensure integration test"),
            InExpression,
            UniqueFile,
            911,
            BreakInCode,
            BreakInScript,
            [](const FString&)
            {
            },
            [&](const FCk_Utils_EditorOnly_PushNewEditorMessage_Params&)
            {
                ++EditorMessageCount;
            });
    };

    constexpr auto InteractiveRepeatCount = 1000;
    constexpr auto AutomationRepeatCount = 3;

    AddExpectedError(
        TEXT("Game-thread ensure integration test"),
        EAutomationExpectedErrorFlags::Contains,
        1 + AutomationRepeatCount + 1);

    {
        // The automation output device stays registered while the flag is cleared, so the first
        // occurrence's error line is still captured and matched by the expectation above.
        const auto InteractiveScope = TGuardValue{GIsAutomationTesting, false};
        for (auto Index = 0; Index < InteractiveRepeatCount; ++Index)
        {
            Invoke(TEXT("RepeatedGameThreadExpression"));
        }
    }

    TestEqual(TEXT("One repeated signature pushes one editor message outside automation"), EditorMessageCount, 1);

    // Under automation every occurrence reports: tests assert exact per-occurrence counts, and the
    // process-global tracker would otherwise swallow a later test's ensure at an already-fired site.
    for (auto Index = 0; Index < AutomationRepeatCount; ++Index)
    {
        Invoke(TEXT("RepeatedGameThreadExpression"));
    }

    TestEqual(
        TEXT("Under automation every repeated hit pushes its own editor message"),
        EditorMessageCount,
        1 + AutomationRepeatCount);

    Invoke(TEXT("DistinctGameThreadExpression"));

    TestEqual(
        TEXT("A distinct expression pushes its own editor message"),
        EditorMessageCount,
        1 + AutomationRepeatCount + 1);
    TestEqual(
        TEXT("Every repeated and distinct hit remains aggregated"),
        ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount(),
        CountBefore + InteractiveRepeatCount + AutomationRepeatCount + 1);
    TestEqual(
        TEXT("The two game-thread signatures remain distinct"),
        ck::ensure::Get_EnsureOccurrenceTracker().GetUniqueCount(),
        UniqueBefore + 2);
    TestFalse(TEXT("Unattended game-thread reporting does not request a native break"), BreakInCode);
    TestFalse(TEXT("Unattended game-thread reporting does not request a script break"), BreakInScript);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

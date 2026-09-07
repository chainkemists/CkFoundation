#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_frame_analyzer_progressive_tests
{
    auto AreEqual(const TMap<uint32, double>& Left, const TMap<uint32, double>& Right) -> bool
    {
        if (Left.Num() != Right.Num()) return false;
        for (const auto& [Key, Value] : Left)
        {
            const double* Other = Right.Find(Key);
            if (Other == nullptr || *Other != Value) return false;
        }
        return true;
    }

    auto AreEqual(const TMap<uint32, uint32>& Left, const TMap<uint32, uint32>& Right) -> bool
    {
        if (Left.Num() != Right.Num()) return false;
        for (const auto& [Key, Value] : Left)
        {
            const uint32* Other = Right.Find(Key);
            if (Other == nullptr || *Other != Value) return false;
        }
        return true;
    }

    auto AreEqual(const TMap<uint32, TMap<uint32, double>>& Left,
                  const TMap<uint32, TMap<uint32, double>>& Right) -> bool
    {
        if (Left.Num() != Right.Num()) return false;
        for (const auto& [Key, Value] : Left)
        {
            const TMap<uint32, double>* Other = Right.Find(Key);
            if (Other == nullptr || NOT AreEqual(Value, *Other)) return false;
        }
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FrameAnalyzerProgressive_RejectsUnavailableSession,
    "Ck.CkInsightsAnalyzer.FrameAnalyzerProgressive.RejectsUnavailableSession",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FrameAnalyzerProgressive_RejectsUnavailableSession::RunTest(const FString&)
{
    const auto Session = FCk_TraceSession{};

    const FCk_AvailableFrameBatch Batch = Session.ReadAvailableFrames(0);
    TestEqual(TEXT("unopened session exposes no frame count"), Batch.FrameCount, uint64{0});
    TestEqual(TEXT("unopened session exposes no durations"), Batch.Durations.Num(), 0);
    TestEqual(TEXT("unopened session gives an explicit batch error"), Batch.Error,
        FString{TEXT("Trace session is not available.")});

    auto Snapshot = FCk_FrameSnapshot{};
    Snapshot.Result.Events.Add({1, 0.0, 0.001, 0});
    Snapshot.TimerNames.Add(1, TEXT("Stale"));

    TestFalse(TEXT("an unopened session cannot produce a provisional snapshot"),
        FCk_FrameAnalyzer::TryCaptureFrameSnapshot(Session, 0, Snapshot));
    TestFalse(TEXT("failed capture clears all partial event data"), Snapshot.Result.IsValid());
    TestEqual(TEXT("failed capture clears copied timer names"), Snapshot.TimerNames.Num(), 0);
    TestEqual(TEXT("unopened session gives an explicit defer reason"), Snapshot.UnavailableReason,
        FString{TEXT("Trace session is not available.")});
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FrameAnalyzerProgressive_RealTraceSnapshotParity,
    "Ck.CkInsightsAnalyzer.FrameAnalyzerProgressive.RealTraceSnapshotParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FrameAnalyzerProgressive_RealTraceSnapshotParity::RunTest(const FString&)
{
    // This is intentionally a machine-fixture test. The saved trace is large enough to expose the
    // parser's intermediate state on the development machine, but source control does not own it.
    FString TracePath = FPlatformMisc::GetEnvironmentVariable(TEXT("CK_INSIGHTS_TEST_TRACE"));
    const bool UsesEnvironmentTrace = NOT TracePath.IsEmpty();
    if (NOT UsesEnvironmentTrace)
    { TracePath = FPaths::ProjectSavedDir() / TEXT("Profiling/20260807_192547_5F8520.utrace"); }
    if (NOT IFileManager::Get().FileExists(*TracePath))
    {
        if (UsesEnvironmentTrace)
        {
            AddError(*FString::Printf(TEXT("CK_INSIGHTS_TEST_TRACE does not exist: %s"), *TracePath));
            return false;
        }

        AddWarning(TEXT("SKIPPED: progressive TraceServices fixture is not present in Saved/Profiling."));
        return true;
    }

    auto Session = FCk_TraceSession{};
    if (NOT TestTrue(TEXT("TraceServices analysis service prepares"), Session.PrepareAnalysisService()) ||
        NOT TestTrue(TEXT("Real fixture starts asynchronous analysis"), Session.StartAnalysis(TracePath)))
    {
        return false;
    }

    constexpr double TimeoutSeconds = 30.0;
    const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
    auto ProvisionalSnapshot = FCk_FrameSnapshot{};
    uint64 ProvisionalFrameIndex = 0;
    bool SawIncompleteAnalysis = false;
    bool SawAvailableFrames = false;
    bool CapturedProvisionalSnapshot = false;

    while (FPlatformTime::Seconds() < Deadline && NOT Session.IsAnalysisComplete())
    {
        SawIncompleteAnalysis = true;
        const FCk_AvailableFrameBatch Batch = Session.ReadAvailableFrames(0);
        SawAvailableFrames |= Batch.Durations.Num() > 0;

        if (Batch.Durations.Num() > 0)
        {
            // The batch is contiguous from zero and omits an active tail, so this closed frame can
            // be retried later without an index translation.
            const uint64 CandidateFrameIndex = static_cast<uint64>(Batch.Durations.Num() - 1);
            if (FCk_FrameAnalyzer::TryCaptureFrameSnapshot(
                    Session, CandidateFrameIndex, ProvisionalSnapshot))
            {
                ProvisionalFrameIndex = CandidateFrameIndex;
                CapturedProvisionalSnapshot = ProvisionalSnapshot.IsProvisional;
                break;
            }
        }

        FPlatformProcess::Sleep(0.001f);
    }

    while (FPlatformTime::Seconds() < Deadline && NOT Session.IsAnalysisComplete())
    {
        FPlatformProcess::Sleep(0.001f);
    }

    const bool Finished = Session.IsAnalysisComplete();
    if (NOT Finished)
    {
        AddError(TEXT("TraceServices analysis did not complete before the progressive fixture timeout."));
        Session.Close();
        return false;
    }

    if (NOT SawIncompleteAnalysis || NOT SawAvailableFrames || NOT CapturedProvisionalSnapshot)
    {
        AddWarning(FString::Printf(
            TEXT("SKIPPED: fixture did not expose a usable provisional snapshot (incomplete=%s frames=%s snapshot=%s complete=%s)."),
            SawIncompleteAnalysis ? TEXT("true") : TEXT("false"),
            SawAvailableFrames ? TEXT("true") : TEXT("false"),
            CapturedProvisionalSnapshot ? TEXT("true") : TEXT("false"),
            Finished ? TEXT("true") : TEXT("false")));
        Session.Close();
        return true;
    }

    TestTrue(TEXT("Captured snapshot is marked provisional"), ProvisionalSnapshot.IsProvisional);
    TestTrue(TEXT("Captured provisional snapshot contains timing events"), ProvisionalSnapshot.Result.IsValid());
    TestTrue(TEXT("Provisional frame start is finite"), FMath::IsFinite(ProvisionalSnapshot.Result.FrameStartTime));
    TestTrue(TEXT("Provisional frame end is finite"), FMath::IsFinite(ProvisionalSnapshot.Result.FrameEndTime));
    TestTrue(TEXT("Provisional frame bounds are ordered"),
        ProvisionalSnapshot.Result.FrameEndTime >= ProvisionalSnapshot.Result.FrameStartTime);
    TestTrue(TEXT("Provisional snapshot copied referenced timer names"), ProvisionalSnapshot.TimerNames.Num() > 0);

    auto AllCopiedNamesAreReferenced = true;
    for (const auto& [TimerIndex, Name] : ProvisionalSnapshot.TimerNames)
    {
        const auto HasEvent = ProvisionalSnapshot.Result.Events.ContainsByPredicate(
            [TimerIndex](const FCk_TimingEvent& Event) { return Event.TimerIndex == TimerIndex; });
        AllCopiedNamesAreReferenced &= NOT Name.IsEmpty() && HasEvent;
    }
    TestTrue(TEXT("Every copied timer name belongs to a provisional event"), AllCopiedNamesAreReferenced);
    AddInfo(FString::Printf(
        TEXT("Observed provisional snapshot: frame=%llu events=%d names=%d range=[%.6f, %.6f]."),
        ProvisionalFrameIndex,
        ProvisionalSnapshot.Result.Events.Num(),
        ProvisionalSnapshot.TimerNames.Num(),
        ProvisionalSnapshot.Result.FrameStartTime,
        ProvisionalSnapshot.Result.FrameEndTime));

    auto FinalSnapshot = FCk_FrameSnapshot{};
    const bool CapturedFinalSnapshot = FCk_FrameAnalyzer::TryCaptureFrameSnapshot(
        Session, ProvisionalFrameIndex, FinalSnapshot);
    TestTrue(TEXT("Completed trace recaptures the provisional frame"), CapturedFinalSnapshot);
    TestFalse(TEXT("Completed trace snapshot is not provisional"), FinalSnapshot.IsProvisional);
    if (NOT CapturedFinalSnapshot || FinalSnapshot.IsProvisional)
    {
        Session.Close();
        return false;
    }

    const FCk_FrameAnalysisResult Completed = FCk_FrameAnalyzer::AnalyzeFrame(Session, ProvisionalFrameIndex);
    TestTrue(TEXT("Completed analyzer can read the final snapshot frame"), Completed.IsValid());
    if (NOT Completed.IsValid())
    {
        Session.Close();
        return false;
    }

    TestEqual(TEXT("Final snapshot and completed frame indices match"),
        FinalSnapshot.Result.FrameIndex, Completed.FrameIndex);
    TestEqual(TEXT("Final snapshot and completed GameThread IDs match"),
        FinalSnapshot.Result.ThreadId, Completed.ThreadId);
    TestEqual(TEXT("Final snapshot and completed time-range validity match"),
        FinalSnapshot.Result.HasValidTimeRange, Completed.HasValidTimeRange);
    TestTrue(TEXT("Final snapshot preserves a valid completed time range"),
        FinalSnapshot.Result.HasValidTimeRange);
    TestEqual(TEXT("Final snapshot and completed event counts match"),
        FinalSnapshot.Result.Events.Num(), Completed.Events.Num());
    TestTrue(TEXT("Final snapshot and completed instrumented totals match"),
        FMath::IsNearlyEqual(FinalSnapshot.Result.InstrumentedMs, Completed.InstrumentedMs, 0.001));
    TestTrue(TEXT("Final snapshot and completed inclusive maps match"),
        ck_frame_analyzer_progressive_tests::AreEqual(
            FinalSnapshot.Result.TimerInclusive, Completed.TimerInclusive));
    TestTrue(TEXT("Final snapshot and completed outer-inclusive maps match"),
        ck_frame_analyzer_progressive_tests::AreEqual(
            FinalSnapshot.Result.TimerOuterInclusive, Completed.TimerOuterInclusive));
    TestTrue(TEXT("Final snapshot and completed exclusive maps match"),
        ck_frame_analyzer_progressive_tests::AreEqual(
            FinalSnapshot.Result.TimerExclusive, Completed.TimerExclusive));
    TestTrue(TEXT("Final snapshot and completed timer counts match"),
        ck_frame_analyzer_progressive_tests::AreEqual(
            FinalSnapshot.Result.TimerCount, Completed.TimerCount));
    TestTrue(TEXT("Final snapshot and completed child graphs match"),
        ck_frame_analyzer_progressive_tests::AreEqual(
            FinalSnapshot.Result.ChildrenOf, Completed.ChildrenOf));
    const auto SnapshotAccounting = FCk_FrameReport::ComputeFrameAccounting(
        FinalSnapshot.Result, FinalSnapshot.TimerNames);
    const auto CompletedAccounting = FCk_FrameReport::ComputeFrameAccounting(
        Completed, FinalSnapshot.TimerNames);
    TestTrue(TEXT("Final snapshot and completed uninstrumented accounting match"),
        FMath::IsNearlyEqual(
            SnapshotAccounting.UninstrumentedMs,
            CompletedAccounting.UninstrumentedMs,
            0.001));
    TestTrue(TEXT("Final snapshot and completed exclusive coverage errors match"),
        FMath::IsNearlyEqual(
            SnapshotAccounting.ExclusiveCoverageErrorMs,
            CompletedAccounting.ExclusiveCoverageErrorMs,
            0.001));
    AddInfo(FString::Printf(
        TEXT("Verified final snapshot parity: frame=%llu events=%d names=%d."),
        ProvisionalFrameIndex,
        FinalSnapshot.Result.Events.Num(),
        FinalSnapshot.TimerNames.Num()));

    Session.Close();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

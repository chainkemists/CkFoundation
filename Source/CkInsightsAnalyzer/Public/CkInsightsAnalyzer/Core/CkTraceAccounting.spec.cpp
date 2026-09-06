#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Report/CkJsonReport.h"
#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_trace_accounting_tests
{
    auto Event(uint32 InTimer, double InStart, double InEnd, uint32 InDepth) -> FCk_TimingEvent
    {
        return FCk_TimingEvent{InTimer, InStart, InEnd, InDepth};
    }

    auto IsFiniteAccounting(const FCk_FrameAccounting& InAccounting) -> bool
    {
        return FMath::IsFinite(InAccounting.FrameMs) &&
            FMath::IsFinite(InAccounting.InstrumentedMs) &&
            FMath::IsFinite(InAccounting.NamedWaitMs) &&
            FMath::IsFinite(InAccounting.OtherInstrumentedMs) &&
            FMath::IsFinite(InAccounting.UninstrumentedMs) &&
            FMath::IsFinite(InAccounting.ExclusiveSumMs) &&
            FMath::IsFinite(InAccounting.ExclusiveCoverageErrorMs);
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_TraceAccounting_ConservesNestedFramePartitions,
    "Ck.CkInsightsAnalyzer.TraceAccounting.ConservesNestedFramePartitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_TraceAccounting_ConservesNestedFramePartitions::RunTest(const FString&)
{
    using namespace ck_trace_accounting_tests;

    const auto Result = FCk_FrameAnalyzer::AnalyzeEvents(
        {
            Event(1, 0.0, 10.0, 0),
            Event(2, 2.0, 4.0, 1),
            Event(3, 5.0, 7.0, 1),
        },
        0.0,
        10.0,
        17,
        44);

    const auto TimerNames = FCk_FrameReport::FTimerNameMap{
        {1, TEXT("FrameRoot")},
        {2, TEXT("GameThreadWaitForTask")},
        {3, TEXT("NestedWork")},
    };
    const auto Accounting = FCk_FrameReport::ComputeFrameAccounting(Result, TimerNames);

    TestEqual(TEXT("frame duration is fully instrumented"), Accounting.FrameMs, 10000.0);
    TestEqual(TEXT("nested scopes union into one instrumented window"), Accounting.InstrumentedMs, 10000.0);
    TestEqual(TEXT("named wait uses exclusive nested wait time"), Accounting.NamedWaitMs, 2000.0);
    TestEqual(TEXT("other instrumented remainder excludes named wait"), Accounting.OtherInstrumentedMs, 8000.0);
    TestEqual(TEXT("no uninstrumented time remains"), Accounting.UninstrumentedMs, 0.0);
    TestEqual(TEXT("exclusive values cover the union without overlap error"), Accounting.ExclusiveCoverageErrorMs, 0.0);
    TestEqual(TEXT("named wait plus other instrumented conserves instrumented wall time"),
        Accounting.NamedWaitMs + Accounting.OtherInstrumentedMs, Accounting.InstrumentedMs);
    TestEqual(TEXT("instrumented plus uninstrumented conserves frame wall time"),
        Accounting.InstrumentedMs + Accounting.UninstrumentedMs, Accounting.FrameMs);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_TraceAccounting_RealTraceEnvironment,
    "Ck.CkInsightsAnalyzer.TraceAccounting.RealTraceEnvironment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_TraceAccounting_RealTraceEnvironment::RunTest(const FString&)
{
    using namespace ck_trace_accounting_tests;

    const FString TracePath = FPlatformMisc::GetEnvironmentVariable(TEXT("CK_INSIGHTS_TEST_TRACE"));
    if (TracePath.IsEmpty())
    {
        AddWarning(TEXT("SKIPPED: set CK_INSIGHTS_TEST_TRACE to a .utrace path to run real-trace accounting coverage."));
        return true;
    }
    if (NOT IFileManager::Get().FileExists(*TracePath))
    {
        AddError(*FString::Printf(TEXT("CK_INSIGHTS_TEST_TRACE does not exist: %s"), *TracePath));
        return false;
    }

    auto Session = FCk_TraceSession{};
    if (NOT TestTrue(TEXT("environment trace opens"), Session.Open(TracePath)))
    { return false; }

    constexpr auto FirstFrame = uint64{1};
    constexpr auto LastFrame = uint64{994};
    if (Session.GetFrameCount() <= LastFrame)
    {
        AddError(*FString::Printf(
            TEXT("CK_INSIGHTS_TEST_TRACE has %llu frames; real-trace coverage requires frame 994."),
            Session.GetFrameCount()));
        return false;
    }

    auto Config = FCk_MultiFrameReportConfig{};
    Config.TimerAverageCount = MAX_int32;
    Config.MinTimerAverageMs = 0.0;
    Config.MinCategoryMs = 0.0;
    Config.ComputeWaitAverages = true;
    Config.BuildMergedHotPaths = true;

    auto Report = FCk_MultiFrameReport{Config};
    const auto Markdown = Report.AnalyzeFrameSet(Session, {FCk_FrameRun{FirstFrame, LastFrame}});
    TestFalse(TEXT("real trace produces a report"), Markdown.IsEmpty());

    const FCk_MultiFrameStats& Stats = Report.GetStats();
    constexpr auto ExpectedFrameCount = LastFrame - FirstFrame + 1;
    TestEqual(TEXT("all requested real frames are analysed"), Stats.FrameCount, ExpectedFrameCount);
    TestEqual(TEXT("one inclusive production range is retained"), Stats.SelectedRuns.Num(), 1);
    if (Stats.SelectedRuns.Num() == 1)
    {
        TestEqual(TEXT("selected range starts at frame one"), Stats.SelectedRuns[0].FirstFrame, FirstFrame);
        TestEqual(TEXT("selected range ends at frame 994"), Stats.SelectedRuns[0].LastFrame, LastFrame);
    }
    TestEqual(TEXT("accounting remains aligned with analysed frames"),
        Stats.FrameAccounting.Num(), static_cast<int32>(Stats.FrameCount));
    TestTrue(TEXT("average accounting is available"), Stats.AverageAccounting.IsSet());
    TestTrue(TEXT("production-equivalent wait averages are computed"), Stats.WaitAveragesComputed);

    for (const FCk_FrameAccounting& Accounting : Stats.FrameAccounting)
    {
        TestTrue(TEXT("every real-frame accounting value is finite"), IsFiniteAccounting(Accounting));
        TestTrue(TEXT("every real-frame exclusive coverage error is bounded"),
            FMath::Abs(Accounting.ExclusiveCoverageErrorMs) < 0.001);
        TestTrue(TEXT("every real-frame accounting amount is nonnegative"),
            Accounting.InstrumentedMs >= 0.0 && Accounting.NamedWaitMs >= 0.0 &&
                Accounting.OtherInstrumentedMs >= 0.0 && Accounting.UninstrumentedMs >= 0.0 &&
                Accounting.ExclusiveSumMs >= 0.0);
        TestTrue(TEXT("instrumented plus uninstrumented conserves each real frame"),
            FMath::IsNearlyEqual(
                Accounting.InstrumentedMs + Accounting.UninstrumentedMs,
                Accounting.FrameMs,
                0.001));
        TestTrue(TEXT("named wait plus other instrumented conserves each real frame"),
            FMath::IsNearlyEqual(
                Accounting.NamedWaitMs + Accounting.OtherInstrumentedMs,
                Accounting.InstrumentedMs,
                0.001));
    }
    if (Stats.AverageAccounting.IsSet())
    {
        TestTrue(TEXT("average total exclusive time matches average accounting attribution"),
            FMath::IsNearlyEqual(
                Stats.TotalExclusiveMs,
                Stats.AverageAccounting->ExclusiveSumMs,
                0.001));
    }

    const FString OutputDir = FPaths::ProjectSavedDir() / TEXT("Profiling/LateGameSteadyCpu");
    IFileManager::Get().MakeDirectory(*OutputDir, true);
    const FString OutputPath = OutputDir / TEXT("InsightsAnalyzerV3.json");
    TestTrue(TEXT("real-trace JSON artifact is written"),
        FFileHelper::SaveStringToFile(
            FCk_JsonReport::GenerateMultiFrame(Session, Stats, Report.GetConfig()),
            *OutputPath));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

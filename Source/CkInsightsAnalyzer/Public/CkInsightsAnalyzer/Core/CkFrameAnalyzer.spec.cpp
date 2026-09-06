#include "CkInsightsAnalyzer/Core/CkFrameAnalyzer.h"

#include "Misc/AutomationTest.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_frame_analyzer_tests
{
    auto Event(uint32 InTimer, double InStart, double InEnd, uint32 InDepth) -> FCk_TimingEvent
    {
        return FCk_TimingEvent{InTimer, InStart, InEnd, InDepth};
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FrameAnalyzer_ClipsBoundarySpanningScopes,
    "Ck.CkInsightsAnalyzer.FrameAnalyzer.ClipsBoundarySpanningScopes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FrameAnalyzer_ClipsBoundarySpanningScopes::RunTest(const FString&)
{
    using namespace ck_frame_analyzer_tests;

    const auto Result = FCk_FrameAnalyzer::AnalyzeEvents(
        {Event(11, -1.0, 1.0, 1), Event(10, -2.0, 3.0, 0)}, 0.0, 2.0, 7, 42);

    TestTrue(TEXT("clipped range is valid"), Result.HasValidTimeRange);
    TestEqual(TEXT("thread id is retained"), Result.ThreadId, 7u);
    TestEqual(TEXT("frame index is retained"), Result.FrameIndex, uint64{42});
    TestEqual(TEXT("both intersecting scopes are retained"), Result.Events.Num(), 2);
    TestEqual(TEXT("parent sorts before same-window child"), Result.Events[0].TimerIndex, 10u);
    TestEqual(TEXT("parent is clipped at both window boundaries"), Result.Events[0].StartTime, 0.0);
    TestEqual(TEXT("parent end is clipped at window end"), Result.Events[0].EndTime, 2.0);
    TestEqual(TEXT("instrumented union is the full two-second window"), Result.InstrumentedMs, 2000.0);
    TestEqual(TEXT("parent inclusive preserves clipped duration"), Result.TimerInclusive.FindRef(10), 2.0);
    TestEqual(TEXT("child inclusive preserves clipped duration"), Result.TimerInclusive.FindRef(11), 1.0);
    TestEqual(TEXT("parent exclusive excludes clipped child"), Result.TimerExclusive.FindRef(10), 1.0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FrameAnalyzer_RecursiveTimerOuterInclusiveIsUnioned,
    "Ck.CkInsightsAnalyzer.FrameAnalyzer.RecursiveTimerOuterInclusiveIsUnioned",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FrameAnalyzer_RecursiveTimerOuterInclusiveIsUnioned::RunTest(const FString&)
{
    using namespace ck_frame_analyzer_tests;

    const auto Result = FCk_FrameAnalyzer::AnalyzeEvents(
        {Event(20, 0.0, 10.0, 0), Event(20, 2.0, 8.0, 1), Event(20, 12.0, 14.0, 0)},
        0.0, 20.0);

    TestEqual(TEXT("summed inclusive retains recursive calls"), Result.TimerInclusive.FindRef(20), 18.0);
    TestEqual(TEXT("outer inclusive unions recursive same-name calls"), Result.TimerOuterInclusive.FindRef(20), 12.0);
    TestEqual(TEXT("all same-name intervals instrument twelve seconds"), Result.InstrumentedMs, 12000.0);
    TestEqual(TEXT("recursive timer call count remains three"), Result.GetCount(20), 3u);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FrameAnalyzer_DisjointRootsLeaveUninstrumentedGap,
    "Ck.CkInsightsAnalyzer.FrameAnalyzer.DisjointRootsLeaveUninstrumentedGap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FrameAnalyzer_DisjointRootsLeaveUninstrumentedGap::RunTest(const FString&)
{
    using namespace ck_frame_analyzer_tests;

    const auto Result = FCk_FrameAnalyzer::AnalyzeEvents(
        {Event(30, 0.0, 2.0, 0), Event(31, 5.0, 7.0, 0)}, 0.0, 10.0);

    TestEqual(TEXT("two root intervals are retained"), Result.Events.Num(), 2);
    TestEqual(TEXT("instrumented union excludes the three-second gap"), Result.InstrumentedMs, 4000.0);
    TestEqual(TEXT("frame duration retains complete requested window"), Result.FrameDurationMs, 10000.0);
    TestEqual(TEXT("first root is selected"), Result.FrameRootTimerIndex, 30u);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_FrameAnalyzer_RejectsInvalidWindowsAndMalformedEvents,
    "Ck.CkInsightsAnalyzer.FrameAnalyzer.RejectsInvalidWindowsAndMalformedEvents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_FrameAnalyzer_RejectsInvalidWindowsAndMalformedEvents::RunTest(const FString&)
{
    using namespace ck_frame_analyzer_tests;

    const auto EmptyRange = FCk_FrameAnalyzer::AnalyzeEvents({}, 1.0, 2.0);
    TestTrue(TEXT("empty finite window remains valid and uninstrumented"), EmptyRange.HasValidTimeRange);
    TestTrue(TEXT("empty finite window is a valid result"), EmptyRange.IsValid());
    TestEqual(TEXT("empty finite window has no instrumentation"), EmptyRange.InstrumentedMs, 0.0);

    const auto InvalidRange = FCk_FrameAnalyzer::AnalyzeEvents({Event(40, 0.0, 1.0, 0)}, 2.0, 2.0);
    TestFalse(TEXT("zero-length window is invalid"), InvalidRange.HasValidTimeRange);
    TestFalse(TEXT("zero-length window is not valid"), InvalidRange.IsValid());
    TestEqual(TEXT("invalid window leaves no events"), InvalidRange.Events.Num(), 0);

    const auto ReversedRange = FCk_FrameAnalyzer::AnalyzeEvents({Event(40, 0.0, 1.0, 0)}, 3.0, 2.0);
    TestFalse(TEXT("reversed window is invalid"), ReversedRange.HasValidTimeRange);
    TestEqual(TEXT("reversed window leaves no events before any indexing"), ReversedRange.Events.Num(), 0);

    const auto PositiveInfinity = std::numeric_limits<double>::infinity();
    const auto QuietNaN = std::numeric_limits<double>::quiet_NaN();
    const auto MaxFinite = std::numeric_limits<double>::max();
    const auto NaNWindow = FCk_FrameAnalyzer::AnalyzeEvents({}, QuietNaN, 1.0);
    TestFalse(TEXT("NaN window bound is invalid"), NaNWindow.HasValidTimeRange);

    const auto OverflowWindow = FCk_FrameAnalyzer::AnalyzeEvents({}, -MaxFinite, MaxFinite);
    TestFalse(TEXT("finite endpoints whose duration overflows are invalid"), OverflowWindow.HasValidTimeRange);

    const auto MillisecondOverflowWindow = FCk_FrameAnalyzer::AnalyzeEvents({}, 0.0, MaxFinite / 500.0);
    TestFalse(TEXT("finite seconds whose millisecond conversion overflows are invalid"),
        MillisecondOverflowWindow.HasValidTimeRange);

    const auto Malformed = FCk_FrameAnalyzer::AnalyzeEvents(
        {
            Event(41, -PositiveInfinity, 1.0, 0),
            Event(42, 0.0, PositiveInfinity, 0),
            Event(43, 4.0, 3.0, 0),
            Event(44, 2.0, 2.0, 0),
            Event(45, 1.0, QuietNaN, 0),
            Event(46, 6.0, 7.0, 0),
        },
        0.0, 5.0);

    TestTrue(TEXT("finite window with malformed events remains valid"), Malformed.HasValidTimeRange);
    TestEqual(TEXT("only positive-infinity capture tail is retained"), Malformed.Events.Num(), 1);
    TestEqual(TEXT("positive-infinity tail is clipped to window"), Malformed.Events[0].EndTime, 5.0);
    TestEqual(TEXT("malformed-event result remains fully instrumented by tail"), Malformed.InstrumentedMs, 5000.0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

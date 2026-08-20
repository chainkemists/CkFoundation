// Per-timer samples in TimerAverages are stored only for the frames a timer actually appeared in;
// the absent frames are treated as leading zeros analytically instead of being materialised. If
// that shortcut ever diverges from the straightforward padded computation, every p95 in the
// timerAverages section is quietly wrong, so the equivalence is pinned directly rather than
// sampled through a generated report.

#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_multi_frame_report_tests
{
    // Spans the shapes that matter: a timer present in every frame (no zeros), one present in most,
    // one so rare the percentile falls inside the zero run, and both degenerate ends.
    const TArray<int32> PresentCounts {0, 1, 2, 5, 20, 100};
    const TArray<int32> AbsentCounts  {0, 1, 3, 19, 95, 400};
    const TArray<double> Percentiles  {0.0, 50.0, 95.0, 99.0, 100.0};

    // Strictly increasing and all above zero, so a wrong zero-run offset shows up as a different
    // value rather than coincidentally matching.
    auto Make_PresentValues(int32 InCount) -> TArray<double>
    {
        auto Values = TArray<double>{};
        Values.Reserve(InCount);

        for (auto Index = 0; Index < InCount; ++Index)
        {
            Values.Add(1.0 + static_cast<double>(Index));
        }

        return Values;
    }

    auto Make_PaddedValues(const TArray<double>& InPresentValues, int32 InAbsentCount) -> TArray<double>
    {
        auto Padded = TArray<double>{};
        Padded.Reserve(InPresentValues.Num() + InAbsentCount);
        Padded.AddZeroed(InAbsentCount);
        Padded.Append(InPresentValues);

        ck::algo::Sort(Padded);

        return Padded;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_PercentileWithLeadingZerosMatchesPadded,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.PercentileWithLeadingZerosMatchesPadded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_PercentileWithLeadingZerosMatchesPadded::RunTest(const FString&)
{
    using namespace ck_multi_frame_report_tests;

    ck::algo::ForEach(PresentCounts, [&](int32 InPresentCount)
    {
        ck::algo::ForEach(AbsentCounts, [&](int32 InAbsentCount)
        {
            const auto Present = Make_PresentValues(InPresentCount);
            const auto Padded = Make_PaddedValues(Present, InAbsentCount);

            ck::algo::ForEach(Percentiles, [&](double InPercentile)
            {
                TestEqual(
                    *FString::Printf(TEXT("present=%d absent=%d p%.0f"),
                        InPresentCount, InAbsentCount, InPercentile),
                    FCk_MultiFrameReport::PercentileWithLeadingZeros(Present, InAbsentCount, InPercentile),
                    FCk_MultiFrameReport::Percentile(Padded, InPercentile));
            });
        });
    });

    // A negative zero-count clamps rather than indexing backwards.
    auto Single = TArray<double>{7.0};
    TestEqual(TEXT("negative leading-zero count clamps to none"),
        FCk_MultiFrameReport::PercentileWithLeadingZeros(Single, -5, 95.0), 7.0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS

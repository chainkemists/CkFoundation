// Per-timer samples in TimerAverages are stored only for the frames a timer actually appeared in;
// the absent frames are treated as leading zeros analytically instead of being materialised. If
// that shortcut ever diverges from the straightforward padded computation, every p95 in the
// timerAverages section is quietly wrong, so the equivalence is pinned directly rather than
// sampled through a generated report.

#include "CkInsightsAnalyzer/Report/CkMultiFrameReport.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

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

    auto Make_HotPathNode(
        const FString& InRawName,
        double InInclusiveMs,
        double InExclusiveMs,
        uint32 InCount,
        const TArray<FString>& InBreadcrumbs = {})
        -> TSharedPtr<FCk_HotPathNode>
    {
        auto Node = MakeShared<FCk_HotPathNode>();
        Node->RawName = InRawName;
        Node->DisplayName = InRawName;
        Node->Breadcrumbs = InBreadcrumbs;
        Node->InclusiveMs = InInclusiveMs;
        Node->ExclusiveMs = InExclusiveMs;
        Node->Count = InCount;

        return Node;
    }

    auto Find_MergedChild(const TSharedPtr<FCk_MergedHotPathNode>& InNode, const FString& InRawName)
        -> TSharedPtr<FCk_MergedHotPathNode>
    {
        const auto* Found = InNode->Children.FindByPredicate(
            [&InRawName](const TSharedPtr<FCk_MergedHotPathNode>& InChild)
            {
                return InChild->RawName == InRawName;
            });

        return Found != nullptr ? *Found : nullptr;
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

// The screenshot predicate decides which frames the worst-frame ranking skips as capture cost;
// a drift here silently re-pollutes (over-match) or re-admits (under-match) the ranking, so the
// substring contract is pinned: any "ScreenshotTracing*" scope marks the frame, case-insensitively,
// and nothing else does.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_ScreenshotFrameDetection,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.ScreenshotFrameDetection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_ScreenshotFrameDetection::RunTest(const FString&)
{
    auto TimerNames = TMap<uint32, FString>{};
    TimerNames.Add(1, TEXT("GameThreadWaitForTask"));
    TimerNames.Add(2, TEXT("ScreenshotTracing_Prepare"));
    TimerNames.Add(3, TEXT("screenshottracing_execute"));
    TimerNames.Add(4, TEXT("FScreenshotRequest"));   // not a ScreenshotTracing scope

    const auto MakeResult = [](const TArray<uint32>& InTimerIndices)
    {
        auto Result = FCk_FrameAnalysisResult{};
        for (const auto TimerIndex : InTimerIndices)
        { Result.TimerExclusive.Add(TimerIndex, 0.001); }
        return Result;
    };

    TestFalse(TEXT("plain frame"),
        FCk_MultiFrameReport::DoIs_ScreenshotFrame(MakeResult({1}), TimerNames));
    TestTrue(TEXT("ScreenshotTracing_Prepare marks the frame"),
        FCk_MultiFrameReport::DoIs_ScreenshotFrame(MakeResult({1, 2}), TimerNames));
    TestTrue(TEXT("case-insensitive sibling scope marks the frame"),
        FCk_MultiFrameReport::DoIs_ScreenshotFrame(MakeResult({3}), TimerNames));
    TestFalse(TEXT("a non-tracing screenshot scope does not"),
        FCk_MultiFrameReport::DoIs_ScreenshotFrame(MakeResult({4}), TimerNames));
    TestFalse(TEXT("timer index with no name entry does not"),
        FCk_MultiFrameReport::DoIs_ScreenshotFrame(MakeResult({99}), TimerNames));
    TestFalse(TEXT("empty frame"),
        FCk_MultiFrameReport::DoIs_ScreenshotFrame(MakeResult({}), TimerNames));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The run selection is the only place a caller can express "these frames and no others". If the
// validator lets a malformed set through, the worker averages frames nobody asked for and labels
// the result with the selection that was requested — a silently wrong report rather than a rejected
// one. Admission is all-or-nothing, so the predicate is pinned per malformation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_FrameRunSelectionValidation,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.FrameRunSelectionValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_FrameRunSelectionValidation::RunTest(const FString&)
{
    constexpr auto TotalFrames = uint64{500};

    TestFalse(TEXT("empty selection"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection({}, TotalFrames));
    TestFalse(TEXT("empty trace rejects an otherwise sane run"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection({FCk_FrameRun{0, 0}}, 0));

    TestTrue(TEXT("single whole-trace run"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection({FCk_FrameRun{0, TotalFrames - 1}}, TotalFrames));
    TestTrue(TEXT("single one-frame run"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection({FCk_FrameRun{120, 120}}, TotalFrames));
    TestTrue(TEXT("disjoint ascending runs"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection(
            {FCk_FrameRun{120, 140}, FCk_FrameRun{200, 200}, FCk_FrameRun{250, 260}}, TotalFrames));
    TestTrue(TEXT("adjacent runs are legal - the caller chose not to merge them"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection(
            {FCk_FrameRun{120, 140}, FCk_FrameRun{141, 150}}, TotalFrames));

    TestFalse(TEXT("reversed run"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection({FCk_FrameRun{140, 120}}, TotalFrames));
    TestFalse(TEXT("descending runs"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection(
            {FCk_FrameRun{250, 260}, FCk_FrameRun{120, 140}}, TotalFrames));
    TestFalse(TEXT("overlapping runs"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection(
            {FCk_FrameRun{120, 140}, FCk_FrameRun{140, 150}}, TotalFrames));
    TestFalse(TEXT("last frame out of bounds"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection({FCk_FrameRun{0, TotalFrames}}, TotalFrames));
    TestFalse(TEXT("one valid run does not rescue a malformed sibling"),
        FCk_MultiFrameReport::DoIs_ValidRunSelection(
            {FCk_FrameRun{120, 140}, FCk_FrameRun{260, 250}}, TotalFrames));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// Flattening is what the worker actually iterates, and every average divides by its length. An
// off-by-one at a run boundary shifts the whole report onto frames adjacent to the selection.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_FrameRunFlattening,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.FrameRunFlattening",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_FrameRunFlattening::RunTest(const FString&)
{
    const auto AsInt64 = [](uint64 InValue) -> int64 { return static_cast<int64>(InValue); };

    const auto Empty = FCk_MultiFrameReport::DoGet_FrameIndices({});
    TestEqual(TEXT("empty selection flattens to nothing"), Empty.Num(), 0);
    TestEqual(TEXT("empty selection counts zero"),
        AsInt64(FCk_MultiFrameReport::DoGet_SelectedFrameCount({})), static_cast<int64>(0));

    const auto Single = FCk_MultiFrameReport::DoGet_FrameIndices({FCk_FrameRun{7, 7}});
    TestEqual(TEXT("one-frame run is inclusive on both ends"), Single.Num(), 1);
    if (Single.Num() == 1)
    {
        TestEqual(TEXT("one-frame run yields its frame"), AsInt64(Single[0]), static_cast<int64>(7));
    }

    const auto Runs = TArray<FCk_FrameRun>{
        FCk_FrameRun{3, 5}, FCk_FrameRun{9, 9}, FCk_FrameRun{20, 22}};
    const auto Flattened = FCk_MultiFrameReport::DoGet_FrameIndices(Runs);
    const auto Expected = TArray<uint64>{3, 4, 5, 9, 20, 21, 22};

    TestEqual(TEXT("flattened length matches the counted length"),
        static_cast<int64>(Flattened.Num()),
        AsInt64(FCk_MultiFrameReport::DoGet_SelectedFrameCount(Runs)));
    TestEqual(TEXT("flattened length"), Flattened.Num(), Expected.Num());

    if (Flattened.Num() == Expected.Num())
    {
        for (auto Index = 0; Index < Expected.Num(); ++Index)
        {
            TestEqual(*FString::Printf(TEXT("index %d"), Index),
                AsInt64(Flattened[Index]), AsInt64(Expected[Index]));
        }
    }

    // Not reachable through the validated entry points, but the flattener is public and pure:
    // a reversed run contributes nothing rather than looping to exhaustion.
    TestEqual(TEXT("reversed run contributes no indices"),
        FCk_MultiFrameReport::DoGet_FrameIndices({FCk_FrameRun{5, 3}}).Num(), 0);
    TestEqual(TEXT("reversed run counts zero"),
        AsInt64(FCk_MultiFrameReport::DoGet_SelectedFrameCount({FCk_FrameRun{5, 3}})),
        static_cast<int64>(0));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The selection label is the only place the markdown and JSON reports say which frames a
// multi-run report covers; a wrong label misattributes every number under it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_FrameRunsLabel,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.FrameRunsLabel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_FrameRunsLabel::RunTest(const FString&)
{
    TestEqual(TEXT("empty selection"),
        FCk_MultiFrameReport::DoGet_FrameRunsLabel({}), FString{});
    TestEqual(TEXT("contiguous run"),
        FCk_MultiFrameReport::DoGet_FrameRunsLabel({FCk_FrameRun{120, 140}}), FString(TEXT("120-140")));
    TestEqual(TEXT("one-frame run prints bare"),
        FCk_MultiFrameReport::DoGet_FrameRunsLabel({FCk_FrameRun{200, 200}}), FString(TEXT("200")));
    TestEqual(TEXT("mixed disjoint selection"),
        FCk_MultiFrameReport::DoGet_FrameRunsLabel(
            {FCk_FrameRun{120, 140}, FCk_FrameRun{200, 200}, FCk_FrameRun{250, 260}}),
        FString(TEXT("120-140, 200, 250-260")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The merged tree is what replaced deriving hot paths from the averaged frame, and its whole point is
// that a path costing 30ms on two frames out of forty is distinguishable from one costing 1.5ms on all
// forty. Averaging over the analysed frame count, presence counting, and the absent sentinel are the
// three things that carry that distinction, so they are pinned directly.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_MergedHotPathPresence,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.MergedHotPathPresence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_MergedHotPathPresence::RunTest(const FString&)
{
    using namespace ck_multi_frame_report_tests;

    auto FirstRoot = Make_HotPathNode(TEXT("A"), 10.0, 4.0, 1);
    FirstRoot->Children.Add(Make_HotPathNode(TEXT("B"), 4.0, 4.0, 2));
    FirstRoot->Children.Add(Make_HotPathNode(TEXT("C"), 2.0, 2.0, 1));

    auto SecondRoot = Make_HotPathNode(TEXT("A"), 8.0, 6.0, 1);
    SecondRoot->Children.Add(Make_HotPathNode(TEXT("B"), 2.0, 2.0, 2));

    const auto Merged = FCk_MultiFrameReport::DoMerge_HotPathTrees({{FirstRoot}, {SecondRoot}});

    TestEqual(TEXT("both frames' roots merge into one"), Merged.Num(), 1);

    if (Merged.Num() != 1)
    { return false; }

    const auto& Root = Merged[0];
    TestEqual(TEXT("root present in both frames"), static_cast<int32>(Root->FramesPresent), 2);
    TestEqual(TEXT("root inclusive averages over both frames"), Root->AvgInclusiveMs, 9.0);
    TestEqual(TEXT("root exclusive averages over both frames"), Root->AvgExclusiveMs, 5.0);
    TestEqual(TEXT("root count averages over both frames"), Root->AvgCount, 1.0);
    TestEqual(TEXT("root hit average equals its plain average when present everywhere"),
        Root->HitAvgInclusiveMs, 9.0);
    TestEqual(TEXT("root max is the biggest present sample"), Root->MaxInclusiveMs, 10.0);

    TestEqual(TEXT("children merge under the root"), Root->Children.Num(), 2);

    if (Root->Children.Num() != 2)
    { return false; }

    TestEqual(TEXT("children are sorted by average inclusive descending"),
        Root->Children[0]->RawName, FString(TEXT("B")));

    const auto EverywhereChild = Find_MergedChild(Root, TEXT("B"));
    const auto OneFrameChild = Find_MergedChild(Root, TEXT("C"));

    TestTrue(TEXT("both children survive the merge"),
        EverywhereChild.IsValid() && OneFrameChild.IsValid());

    if (NOT EverywhereChild.IsValid() || NOT OneFrameChild.IsValid())
    { return false; }

    TestEqual(TEXT("child present in both frames"),
        static_cast<int32>(EverywhereChild->FramesPresent), 2);
    TestEqual(TEXT("child inclusive averages over both frames"),
        EverywhereChild->AvgInclusiveMs, 3.0);

    // The absent frame is in the divisor but not in the presence count, which is exactly the gap the
    // strip is drawn to show.
    TestEqual(TEXT("one-frame child present in one frame"),
        static_cast<int32>(OneFrameChild->FramesPresent), 1);
    TestEqual(TEXT("one-frame child averages over ALL analysed frames"),
        OneFrameChild->AvgInclusiveMs, 1.0);
    TestEqual(TEXT("one-frame child hit average is over the present frame alone"),
        OneFrameChild->HitAvgInclusiveMs, 2.0);

    TestEqual(TEXT("per-frame series is ordinal-indexed"),
        OneFrameChild->PerFrameInclusiveMs.Num(), 2);

    if (OneFrameChild->PerFrameInclusiveMs.Num() == 2)
    {
        TestEqual(TEXT("present ordinal carries the magnitude"),
            OneFrameChild->PerFrameInclusiveMs[0], 2.0f);
        TestEqual(TEXT("absent ordinal is negative, not zero"),
            OneFrameChild->PerFrameInclusiveMs[1], -1.0f);
    }

    TestEqual(TEXT("an empty selection merges to nothing"),
        FCk_MultiFrameReport::DoMerge_HotPathTrees({}).Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The same timer legitimately appears under different collapsed wrapper chains. Merging by raw name
// alone would fuse those rows and attribute cost to a call path that never ran, so identity is the
// (RawName, Breadcrumbs) pair.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_MergedHotPathIdentity,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.MergedHotPathIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_MergedHotPathIdentity::RunTest(const FString&)
{
    using namespace ck_multi_frame_report_tests;

    const auto FirstTree = TArray<TSharedPtr<FCk_HotPathNode>>{
        Make_HotPathNode(TEXT("X"), 5.0, 5.0, 1, {TEXT("WrapperA")}),
        Make_HotPathNode(TEXT("Y"), 1.0, 1.0, 1, {TEXT("SameWrapper")})};

    const auto SecondTree = TArray<TSharedPtr<FCk_HotPathNode>>{
        Make_HotPathNode(TEXT("X"), 7.0, 7.0, 1, {TEXT("WrapperB")}),
        Make_HotPathNode(TEXT("Y"), 3.0, 3.0, 1, {TEXT("SameWrapper")})};

    const auto Merged = FCk_MultiFrameReport::DoMerge_HotPathTrees({FirstTree, SecondTree});

    TestEqual(TEXT("differing breadcrumbs stay two rows, matching ones merge"), Merged.Num(), 3);

    if (Merged.Num() != 3)
    { return false; }

    const auto CountWithBreadcrumb = [&Merged](const FString& InBreadcrumb) -> int32
    {
        return Merged.FilterByPredicate([&InBreadcrumb](const TSharedPtr<FCk_MergedHotPathNode>& InNode)
        {
            return InNode->Breadcrumbs.Num() == 1 && InNode->Breadcrumbs[0] == InBreadcrumb;
        }).Num();
    };

    TestEqual(TEXT("first wrapper chain kept its own row"), CountWithBreadcrumb(TEXT("WrapperA")), 1);
    TestEqual(TEXT("second wrapper chain kept its own row"), CountWithBreadcrumb(TEXT("WrapperB")), 1);
    TestEqual(TEXT("the shared wrapper chain produced one row"),
        CountWithBreadcrumb(TEXT("SameWrapper")), 1);

    const auto* Split = Merged.FindByPredicate([](const TSharedPtr<FCk_MergedHotPathNode>& InNode)
    {
        return InNode->RawName == TEXT("X") && InNode->Breadcrumbs[0] == TEXT("WrapperA");
    });

    const auto* Shared = Merged.FindByPredicate([](const TSharedPtr<FCk_MergedHotPathNode>& InNode)
    {
        return InNode->RawName == TEXT("Y");
    });

    TestTrue(TEXT("both shapes are present"), Split != nullptr && Shared != nullptr);

    if (Split == nullptr || Shared == nullptr)
    { return false; }

    TestEqual(TEXT("a split row is present in one frame only"),
        static_cast<int32>((*Split)->FramesPresent), 1);
    TestEqual(TEXT("a merged row is present in both frames"),
        static_cast<int32>((*Shared)->FramesPresent), 2);
    TestEqual(TEXT("a merged row averages both frames"), (*Shared)->AvgInclusiveMs, 2.0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// A frame whose own tree came back empty is still an analysed frame: it belongs in the divisor, or
// every average reads high by exactly the share of frames the path was missing from. Percentiles are
// the other half of that contract — those describe the frames the path DID run in.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_MergedHotPathDenominator,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.MergedHotPathDenominator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_MergedHotPathDenominator::RunTest(const FString&)
{
    using namespace ck_multi_frame_report_tests;

    const auto WithEmptyFrame = FCk_MultiFrameReport::DoMerge_HotPathTrees({
        {Make_HotPathNode(TEXT("A"), 6.0, 6.0, 1)},
        {},
        {Make_HotPathNode(TEXT("A"), 3.0, 3.0, 1)}});

    TestEqual(TEXT("the empty frame contributes no row"), WithEmptyFrame.Num(), 1);

    if (WithEmptyFrame.Num() != 1)
    { return false; }

    const auto& Node = WithEmptyFrame[0];
    TestEqual(TEXT("the empty frame adds no presence"), static_cast<int32>(Node->FramesPresent), 2);
    TestEqual(TEXT("the empty frame still divides"), Node->AvgInclusiveMs, 3.0);
    TestEqual(TEXT("hit average skips the empty frame"), Node->HitAvgInclusiveMs, 4.5);
    TestEqual(TEXT("the empty frame takes its own ordinal"), Node->PerFrameInclusiveMs.Num(), 3);

    if (Node->PerFrameInclusiveMs.Num() == 3)
    {
        TestEqual(TEXT("empty frame's ordinal is absent"), Node->PerFrameInclusiveMs[1], -1.0f);
        TestEqual(TEXT("the frame after it keeps its own ordinal"),
            Node->PerFrameInclusiveMs[2], 3.0f);
    }

    const auto Spiky = FCk_MultiFrameReport::DoMerge_HotPathTrees({
        {},
        {Make_HotPathNode(TEXT("Spike"), 10.0, 10.0, 1)},
        {},
        {Make_HotPathNode(TEXT("Spike"), 30.0, 30.0, 1)}});

    TestEqual(TEXT("the spiky path merges into one row"), Spiky.Num(), 1);

    if (Spiky.Num() != 1)
    { return false; }

    const auto& SpikeNode = Spiky[0];
    TestEqual(TEXT("spike averages over every analysed frame"), SpikeNode->AvgInclusiveMs, 10.0);
    TestEqual(TEXT("spike hit average is over the frames it ran in"),
        SpikeNode->HitAvgInclusiveMs, 20.0);
    TestEqual(TEXT("max is over present samples, not the padded series"),
        SpikeNode->MaxInclusiveMs, 30.0);
    // Linear interpolation over the two PRESENT samples: 10 * 0.05 + 30 * 0.95. Over the zero-padded
    // four-frame series it would interpolate 10 and 30 at 0.85 instead, giving 27.
    TestEqual(TEXT("p95 is over present samples, not the padded series"),
        SpikeNode->P95InclusiveMs, 29.0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_FilterAfterAveraging,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.FilterAfterAveraging",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_FilterAfterAveraging::RunTest(const FString&)
{
    using namespace ck_multi_frame_report_tests;

    // Exercise event accounting and the real tree builder, rather than inventing partially
    // populated timer maps. The variable child crosses the per-frame floor but averages above it.
    auto Names = FCk_FrameReport::FTimerNameMap{{0, TEXT("Frame")}, {1, TEXT("Scheduler")},
        {2, TEXT("Variable")}, {3, TEXT("Tiny")}};
    for (uint32 Index = 10; Index < 22; ++Index)
    { Names.Add(Index, FString::Printf(TEXT("Steady%u"), Index)); }

    auto Config = FCk_FrameReportConfig{};
    auto SamplingConfig = Config;
    SamplingConfig.ShowAllChildren = true;
    const auto Session = FCk_TraceSession{};
    auto CompleteTrees = TArray<TArray<TSharedPtr<FCk_HotPathNode>>>{};

    for (const auto VariableMs : {0.2, 1.8})
    {
        auto Frame = FCk_FrameAnalysisResult{};
        Frame.FrameRootTimerIndex = 0;
        Frame.FrameDurationMs = 30.0;
        Frame.Events = {{0, 0.0, 0.030, 0}, {1, 0.0, 0.020, 1},
            {2, 0.0, VariableMs / 1000.0, 2}};
        auto Cursor = VariableMs / 1000.0;
        Frame.Events.Add({3, Cursor, Cursor + 0.0001, 2});
        Cursor += 0.0001;
        for (uint32 Index = 10; Index < 22; ++Index)
        {
            Frame.Events.Add({Index, Cursor, Cursor + 0.0008, 2});
            Cursor += 0.0008;
        }
        FCk_FrameAnalyzer::ComputeExclusiveTimes(Frame.Events, Frame);
        CompleteTrees.Add(FCk_FrameReport{SamplingConfig}.BuildHotPathTree(Session, Frame, Names));
    }

    const auto Filtered = FCk_MultiFrameReport::DoBuild_MergedHotPaths(CompleteTrees, Config);
    const auto ShowAll = FCk_MultiFrameReport::DoBuild_MergedHotPaths(CompleteTrees, SamplingConfig);
    if (Filtered.Num() != 1 || ShowAll.Num() != 1)
    {
        AddError(TEXT("Expected one scheduler root in each presentation"));
        return false;
    }

    const auto Variable = Find_MergedChild(Filtered[0], TEXT("Variable"));
    const auto AllVariable = Find_MergedChild(ShowAll[0], TEXT("Variable"));
    if (NOT Variable.IsValid() || NOT AllVariable.IsValid())
    {
        AddError(TEXT("Variable child missing"));
        return false;
    }
    TestEqual(TEXT("filtering retains both samples"), Variable->FramesPresent, uint64{2});
    TestTrue(TEXT("displayed mean includes every sample"), FMath::IsNearlyEqual(Variable->AvgInclusiveMs, 1.0));
    TestEqual(TEXT("show all cannot change the mean"), Variable->AvgInclusiveMs, AllVariable->AvgInclusiveMs);
    TestTrue(TEXT("show all cannot change the presence series"),
        Variable->PerFrameInclusiveMs == AllVariable->PerFrameInclusiveMs);
    for (uint32 Index = 10; Index < 22; ++Index)
    {
        TestTrue(TEXT("all twelve significant siblings survive the default filter"),
            Find_MergedChild(Filtered[0], Names[Index]).IsValid());
    }
    TestFalse(TEXT("tiny child is filtered by its displayed mean"),
        Find_MergedChild(Filtered[0], TEXT("Tiny")).IsValid());
    TestTrue(TEXT("show all reveals tiny child"), Find_MergedChild(ShowAll[0], TEXT("Tiny")).IsValid());
    TestEqual(TEXT("twelve steady + variable + one remainder"), Filtered[0]->Children.Num(), 14);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_AggregateIdentityAndSamples,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.AggregateIdentityAndSamples",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_AggregateIdentityAndSamples::RunTest(const FString&)
{
    using namespace ck_multi_frame_report_tests;
    auto First = Make_HotPathNode(TEXT("Parent"), 20.0, 18.0, 1);
    auto Second = Make_HotPathNode(TEXT("Parent"), 20.0, 16.0, 1);
    auto RemainderA = Make_HotPathNode(TEXT("(+501 below threshold)"), 2.0, 0.0, 501);
    auto RemainderB = Make_HotPathNode(TEXT("(+500 below threshold)"), 4.0, 0.0, 500);
    RemainderA->bIsAggregate = true;
    RemainderB->bIsAggregate = true;
    First->Children = {RemainderA};
    Second->Children = {RemainderB};
    const auto Merged = FCk_MultiFrameReport::DoMerge_HotPathTrees({{First}, {Second}, {}});
    if (Merged.Num() != 1 || Merged[0]->Children.Num() != 1)
    {
        AddError(TEXT("Changing hidden counts must merge into exactly one remainder per parent"));
        return false;
    }
    const auto Remainder = Merged[0]->Children[0];
    TestTrue(TEXT("remainder retains aggregate flag"), Remainder->bIsAggregate);
    TestEqual(TEXT("all-frame mean"), Remainder->AvgInclusiveMs, 2.0);
    TestEqual(TEXT("presence across count changes"), Remainder->FramesPresent, uint64{2});
    TestEqual(TEXT("hit average"), Remainder->HitAvgInclusiveMs, 3.0);
    TestEqual(TEXT("absent ordinal stays absent"), Remainder->PerFrameInclusiveMs[2], -1.0f);

    // A real timer using the same text must not collide with the synthetic row.
    First->Children.Add(Make_HotPathNode(Remainder->RawName, 1.0, 1.0, 1));
    const auto Collision = FCk_MultiFrameReport::DoMerge_HotPathTrees({{First}, {Second}});
    TestEqual(TEXT("aggregate flag is part of identity"), Collision[0]->Children.Num(), 2);

    auto Config = FCk_FrameReportConfig{};
    Config.MinChildMs = 5.0;
    Config.MinChildPctOfParent = 1.0;
    const auto Folded = FCk_MultiFrameReport::DoBuild_MergedHotPaths({{First}, {Second}, {}}, Config);
    if (Folded.Num() != 1 || Folded[0]->Children.Num() != 1)
    {
        AddError(TEXT("Filtered children and existing remainder must form one group"));
        return false;
    }
    const auto Group = Folded[0]->Children[0];
    TestTrue(TEXT("group mean sums original means"), FMath::IsNearlyEqual(Group->AvgInclusiveMs, 7.0 / 3.0));
    TestEqual(TEXT("group presence is union, not sum"), Group->FramesPresent, uint64{2});
    TestEqual(TEXT("group series adds simultaneous samples"), Group->PerFrameInclusiveMs[0], 3.0f);
    TestEqual(TEXT("group series keeps other sample"), Group->PerFrameInclusiveMs[1], 4.0f);
    TestEqual(TEXT("group series preserves absent ordinal"), Group->PerFrameInclusiveMs[2], -1.0f);
    TestEqual(TEXT("group hit average"), Group->HitAvgInclusiveMs, 3.5);
    TestEqual(TEXT("group maximum"), Group->MaxInclusiveMs, 4.0);
    TestTrue(TEXT("group p95 is computed from the summed series"), FMath::IsNearlyEqual(Group->P95InclusiveMs, 3.95));
    TestTrue(TEXT("empty input remains empty"), FCk_MultiFrameReport::DoBuild_MergedHotPaths({}, Config).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_ThousandCuts,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.ThousandCuts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_ThousandCuts::RunTest(const FString&)
{
    auto Frame = FCk_FrameAnalysisResult{};
    Frame.FrameRootTimerIndex = 0;
    Frame.FrameDurationMs = 3.0;
    Frame.Events = {{0, 0.0, 0.003, 0}, {1, 0.0, 0.002, 1}};
    auto Names = FCk_FrameReport::FTimerNameMap{{0, TEXT("Frame")}, {1, TEXT("ManySmallChildren")}};
    for (uint32 Index = 0; Index < 100; ++Index)
    {
        const auto Start = static_cast<double>(Index) * 0.00002;
        Frame.Events.Add({Index + 2, Start, FMath::Min(Start + 0.00002, 0.002), 2});
        Names.Add(Index + 2, FString::Printf(TEXT("Tiny%u"), Index));
    }
    FCk_FrameAnalyzer::ComputeExclusiveTimes(Frame.Events, Frame);
    const auto Session = FCk_TraceSession{};
    const auto Config = FCk_FrameReportConfig{};
    const auto Single = FCk_FrameReport{Config}.BuildHotPathTree(Session, Frame, Names);
    auto AllConfig = Config;
    AllConfig.ShowAllChildren = true;
    const auto Complete = FCk_FrameReport{AllConfig}.BuildHotPathTree(Session, Frame, Names);
    const auto Merged = FCk_MultiFrameReport::DoBuild_MergedHotPaths({Complete, Complete}, Config);
    if (Single.Num() != 1 || Merged.Num() != 1)
    {
        AddError(TEXT("Expected one 2ms root"));
        return false;
    }
    double SingleVisibleMs = 0.0;
    for (const auto& Child : Single[0]->Children)
    {
        if (NOT Child->bIsAggregate)
        { SingleVisibleMs += Child->InclusiveMs; }
    }
    double MergedVisibleMs = 0.0;
    for (const auto& Child : Merged[0]->Children)
    {
        if (NOT Child->bIsAggregate)
        { MergedVisibleMs += Child->AvgInclusiveMs; }
    }
    TestTrue(TEXT("single-frame tiny children expose at least 97% of parent cost"), SingleVisibleMs >= 1.94 - 0.000001);
    TestTrue(TEXT("averaged tiny children expose at least 97% of parent cost"), MergedVisibleMs >= 1.94 - 0.000001);
    return true;
}

// Cancellation is an atomic admission rule: callers must never receive a partial selection
// masquerading as a small complete result.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_MultiFrameReport_CancellationClearsOutput,
    "Ck.CkInsightsAnalyzer.MultiFrameReport.CancellationClearsOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_MultiFrameReport_CancellationClearsOutput::RunTest(const FString&)
{
    auto Cancelled = TAtomic<bool>{true};
    auto Config = FCk_MultiFrameReportConfig{};
    Config.Cancelled = &Cancelled;

    auto Report = FCk_MultiFrameReport{Config};
    const auto Session = FCk_TraceSession{};
    const FString RangeMarkdown = Report.AnalyzeAndGenerate(Session);
    const FString FrameSetMarkdown = Report.AnalyzeFrameSet(Session, {{0, 0}});
    const FString WorstMarkdown = Report.AnalyzeWorstFrames(Session);
    const FCk_MultiFrameStats& Stats = Report.GetStats();
    const auto Tree = TArray<TArray<TSharedPtr<FCk_HotPathNode>>>{
        {ck_multi_frame_report_tests::Make_HotPathNode(TEXT("cancelled root"), 1.0, 1.0, 1)}};
    const auto Presentation = FCk_FrameReportConfig{};

    TestTrue(TEXT("cancelled range analysis returns no markdown"), RangeMarkdown.IsEmpty());
    TestTrue(TEXT("cancelled frame-set analysis returns no markdown"), FrameSetMarkdown.IsEmpty());
    TestTrue(TEXT("cancelled worst-frame analysis returns no markdown"), WorstMarkdown.IsEmpty());
    TestTrue(TEXT("cancelled raw hot-path merge publishes no roots"),
        FCk_MultiFrameReport::DoMerge_HotPathTrees(Tree, &Cancelled).IsEmpty());
    TestTrue(TEXT("cancelled presented hot-path merge publishes no roots"),
        FCk_MultiFrameReport::DoBuild_MergedHotPaths(Tree, Presentation, &Cancelled).IsEmpty());
    TestEqual(TEXT("cancelled analysis publishes zero frames"), Stats.FrameCount, uint64{0});
    TestEqual(TEXT("cancelled analysis clears selected runs"), Stats.SelectedRuns.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears analysed frame indices"), Stats.AnalysedFrameIndices.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears frame durations"), Stats.FrameDurationsMs.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears worst-frame summaries"), Stats.WorstFrames.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears hot-frame detail"), Stats.HotFrames.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears merged hot paths"), Stats.MergedHotPaths.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears category averages"), Stats.CategoryAverages.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears timer averages"), Stats.TimerAverages.Num(), 0);
    TestEqual(TEXT("cancelled analysis clears wait averages"), Stats.WaitAverages.Num(), 0);
    TestFalse(TEXT("cancelled analysis clears averaged frame"), Stats.AveragedFrame.IsSet());

    // Source control does not own this development-machine fixture. When present, exercise a real
    // completed provider session before cancellation so the second request proves stale statistics
    // are cleared after the narrowed provider-read scopes have populated them.
    const FString TracePath = FPaths::ProjectSavedDir() / TEXT("Profiling/20260807_192547_5F8520.utrace");
    if (NOT IFileManager::Get().FileExists(*TracePath))
    {
        AddWarning(TEXT("SKIPPED: multi-frame cancellation fixture is not present in Saved/Profiling."));
        return true;
    }

    Cancelled.Store(false);
    auto FixtureSession = FCk_TraceSession{};
    if (NOT TestTrue(TEXT("multi-frame fixture prepares TraceServices"), FixtureSession.PrepareAnalysisService()) ||
        NOT TestTrue(TEXT("multi-frame fixture starts asynchronous analysis"), FixtureSession.StartAnalysis(TracePath)))
    {
        return false;
    }

    const double Deadline = FPlatformTime::Seconds() + 30.0;
    while (FPlatformTime::Seconds() < Deadline && NOT FixtureSession.IsAnalysisComplete())
    {
        FPlatformProcess::Sleep(0.001f);
    }
    if (NOT TestTrue(TEXT("multi-frame fixture completes TraceServices analysis"), FixtureSession.IsAnalysisComplete()))
    {
        FixtureSession.Close();
        return false;
    }

    const FCk_AvailableFrameBatch Batch = FixtureSession.ReadAvailableFrames(0, 8);
    if (NOT TestTrue(TEXT("multi-frame fixture has closed frames"), Batch.Durations.Num() > 0))
    {
        FixtureSession.Close();
        return false;
    }

    Config.ComputeWaitAverages = true;
    Config.BuildMergedHotPaths = true;
    Config.ProgressIntervalSeconds = 0.0;
    auto ProgressSnapshots = TArray<FCk_MultiFrameStats>{};
    auto CancelOnProgress = false;
    Config.OnProgress = [&ProgressSnapshots, &Cancelled, &CancelOnProgress](FCk_MultiFrameStats&& InStats,
                                                                              uint64,
                                                                              uint64)
    {
        ProgressSnapshots.Add(MoveTemp(InStats));
        if (CancelOnProgress) Cancelled.Store(true);
    };
    const FCk_FrameRun Run{0, static_cast<uint64>(Batch.Durations.Num() - 1)};
    FixtureSession.GetGameThreadId(); // Prime the compatibility cache on the test owner before worker reads.

    struct FWorkerResult
    {
        FString Markdown;
        FCk_MultiFrameStats Stats;
    };
    const auto AnalyzeOnWorker = [&FixtureSession, Config, Run]() -> FWorkerResult
    {
        auto WorkerReport = FCk_MultiFrameReport{Config};
        auto Result = FWorkerResult{};
        Result.Markdown = WorkerReport.AnalyzeFrameSet(FixtureSession, {Run});
        Result.Stats = WorkerReport.GetStats();
        return Result;
    };

    auto PopulatedFuture = Async(EAsyncExecution::ThreadPool, AnalyzeOnWorker);
    const FWorkerResult Populated = PopulatedFuture.Get();
    const FCk_MultiFrameStats& PopulatedStats = Populated.Stats;
    const bool HasPopulatedStats = PopulatedStats.FrameCount > 0
        && NOT PopulatedStats.AnalysedFrameIndices.IsEmpty()
        && NOT PopulatedStats.FrameDurationsMs.IsEmpty();
    TestTrue(TEXT("multi-frame fixture produces a populated report before cancellation"),
        NOT Populated.Markdown.IsEmpty() && HasPopulatedStats);
    if (NOT HasPopulatedStats)
    {
        FixtureSession.Close();
        return false;
    }
    TestTrue(TEXT("zero test interval publishes progress for completed frames"), ProgressSnapshots.Num() > 0);
    if (ProgressSnapshots.Num() > 0)
    {
        const FCk_MultiFrameStats& LastProgress = ProgressSnapshots.Last();
        TestEqual(TEXT("progress denominator is the processed valid-frame count"),
            LastProgress.FrameCount, PopulatedStats.FrameCount);
        TestEqual(TEXT("progress indices stop at its processed prefix"),
            LastProgress.AnalysedFrameIndices.Num(), PopulatedStats.AnalysedFrameIndices.Num());
        if (LastProgress.MergedHotPaths.Num() > 0)
        {
            TestEqual(TEXT("progress hot-path strip uses the processed denominator"),
                LastProgress.MergedHotPaths[0]->PerFrameInclusiveMs.Num(),
                LastProgress.AnalysedFrameIndices.Num());
        }
    }
    if (ProgressSnapshots.Num() > 1 && ProgressSnapshots[0].MergedHotPaths.Num() > 0 &&
        ProgressSnapshots.Last().MergedHotPaths.Num() > 0)
    {
        TestTrue(TEXT("progress hot-path snapshots own independent tree nodes"),
            ProgressSnapshots[0].MergedHotPaths[0] != ProgressSnapshots.Last().MergedHotPaths[0]);
    }

    const int32 ProgressCountBeforeCancellation = ProgressSnapshots.Num();
    CancelOnProgress = true;
    Cancelled.Store(false);
    auto ClearedFuture = Async(EAsyncExecution::ThreadPool, AnalyzeOnWorker);
    const FWorkerResult Cleared = ClearedFuture.Get();
    const FCk_MultiFrameStats& ClearedStats = Cleared.Stats;
    TestTrue(TEXT("cancelled populated re-run returns no markdown"), Cleared.Markdown.IsEmpty());
    TestTrue(TEXT("mid-progress cancellation fires the callback before clearing output"),
        ProgressSnapshots.Num() > ProgressCountBeforeCancellation);
    TestEqual(TEXT("cancelled populated re-run clears frame count"), ClearedStats.FrameCount, uint64{0});
    TestTrue(TEXT("cancelled populated re-run clears selected runs"), ClearedStats.SelectedRuns.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears frame indices"), ClearedStats.AnalysedFrameIndices.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears frame durations"), ClearedStats.FrameDurationsMs.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears hot paths"), ClearedStats.MergedHotPaths.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears worst frames"), ClearedStats.WorstFrames.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears hot frame detail"), ClearedStats.HotFrames.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears category averages"), ClearedStats.CategoryAverages.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears timer averages"), ClearedStats.TimerAverages.IsEmpty());
    TestTrue(TEXT("cancelled populated re-run clears wait averages"), ClearedStats.WaitAverages.IsEmpty());
    TestFalse(TEXT("cancelled populated re-run clears averaged frame"), ClearedStats.AveragedFrame.IsSet());
    FixtureSession.Close();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

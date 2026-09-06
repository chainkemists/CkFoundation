#include "CkInsightsAnalyzer/Report/CkJsonReport.h"

#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Macros/CkMacros.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_json_report
{
    auto Make_FrameAccounting(const FCk_FrameAccounting& InAccounting, bool bIncludeIdentityAndTimes) -> TSharedPtr<FJsonObject>
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        if (bIncludeIdentityAndTimes)
        {
            Obj->SetNumberField(TEXT("frameIndex"), static_cast<double>(InAccounting.FrameIndex));
            Obj->SetNumberField(TEXT("threadId"), InAccounting.ThreadId);
            Obj->SetNumberField(TEXT("frameStartSeconds"), InAccounting.StartTime);
            Obj->SetNumberField(TEXT("frameEndSeconds"), InAccounting.EndTime);
        }
        Obj->SetNumberField(TEXT("frameMs"), InAccounting.FrameMs);
        Obj->SetNumberField(TEXT("instrumentedMs"), InAccounting.InstrumentedMs);
        Obj->SetNumberField(TEXT("namedWaitMs"), InAccounting.NamedWaitMs);
        Obj->SetNumberField(TEXT("otherInstrumentedMs"), InAccounting.OtherInstrumentedMs);
        Obj->SetNumberField(TEXT("uninstrumentedMs"), InAccounting.UninstrumentedMs);
        Obj->SetNumberField(TEXT("exclusiveSumMs"), InAccounting.ExclusiveSumMs);
        Obj->SetNumberField(TEXT("exclusiveCoverageErrorMs"), InAccounting.ExclusiveCoverageErrorMs);
        return Obj;
    }

    // Takes the rounder rather than reaching for FCk_JsonReport::Round3: that one is private, and a
    // second copy of the rounding rule would let this section drift from every other number here.
    // PerFrameInclusiveMs is deliberately not exported: it is (nodes x analysed frames) numbers, and
    // the strip it feeds is a live UI affordance rather than something a report reader reads back.
    auto Make_MergedHotPathNode(const FCk_MergedHotPathNode& InNode, double (*InRoundMs)(double))
        -> TSharedPtr<FJsonObject>
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

        Obj->SetStringField(TEXT("name"), InNode.DisplayName);
        Obj->SetStringField(TEXT("rawName"), InNode.RawName);
        Obj->SetNumberField(TEXT("avgMs"), InRoundMs(InNode.AvgInclusiveMs));
        Obj->SetNumberField(TEXT("selfAvgMs"), InRoundMs(InNode.AvgExclusiveMs));
        Obj->SetNumberField(TEXT("avgCount"), InRoundMs(InNode.AvgCount));
        Obj->SetNumberField(TEXT("framesPresent"), static_cast<double>(InNode.FramesPresent));
        Obj->SetNumberField(TEXT("hitAvgMs"), InRoundMs(InNode.HitAvgInclusiveMs));
        Obj->SetNumberField(TEXT("p95Ms"), InRoundMs(InNode.P95InclusiveMs));
        Obj->SetNumberField(TEXT("maxMs"), InRoundMs(InNode.MaxInclusiveMs));
        Obj->SetBoolField(TEXT("isAggregate"), InNode.bIsAggregate);

        const auto ChildValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(InNode.Children,
            [InRoundMs](const TSharedPtr<FCk_MergedHotPathNode>& InChild) -> TSharedPtr<FJsonValue>
            {
                return MakeShared<FJsonValueObject>(Make_MergedHotPathNode(*InChild, InRoundMs));
            });

        if (NOT ChildValues.IsEmpty())
        {
            Obj->SetArrayField(TEXT("children"), ChildValues);
        }

        return Obj;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_JsonReport::
    Round3(double Value)
    -> double
{
    return FMath::RoundToDouble(Value * 1000.0) / 1000.0;
}

auto
    FCk_JsonReport::
    ToJsonString(const TSharedRef<FJsonObject>& Root)
    -> FString
{
    FString Out;
    const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
    FJsonSerializer::Serialize(Root, Writer);
    return Out;
}

// --------------------------------------------------------------------------------------------------------------------
// Trace Overview
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_JsonReport::
    MakeTraceOverview(const FCk_TraceSession& Session)
    -> TSharedPtr<FJsonObject>
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

    Obj->SetStringField(TEXT("file"), Session.GetFilePath());
    Obj->SetNumberField(TEXT("durationSeconds"), Round3(Session.GetDurationSeconds()));
    Obj->SetNumberField(TEXT("gameFrameCount"), static_cast<double>(Session.GetFrameCount()));

    if (const uint64 RenderFrames = Session.GetRenderFrameCount();
        RenderFrames > 0)
    {
        Obj->SetNumberField(TEXT("renderFrameCount"), static_cast<double>(RenderFrames));
    }

    TArray<TSharedPtr<FJsonValue>> ThreadValues;
    for (const TraceServices::FThreadInfo& Info : Session.GetThreadInfos())
    {
        TSharedPtr<FJsonObject> ThreadObj = MakeShared<FJsonObject>();
        ThreadObj->SetNumberField(TEXT("id"), Info.Id);
        ThreadObj->SetStringField(TEXT("name"),
            Info.Name ? FString(Info.Name) : FString::Printf(TEXT("Thread %u"), Info.Id));
        if (Info.GroupName && *Info.GroupName)
        {
            ThreadObj->SetStringField(TEXT("group"), Info.GroupName);
        }
        ThreadValues.Add(MakeShared<FJsonValueObject>(ThreadObj));
    }
    if (ThreadValues.Num() > 0)
    {
        Obj->SetArrayField(TEXT("threads"), ThreadValues);
    }

    return Obj;
}

// --------------------------------------------------------------------------------------------------------------------
// Call Tree
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_JsonReport::
    BuildCallTree(const TArray<FCk_TimingEvent>& Events,
                  TArray<FCallTreeNode>& OutPool)
    -> TArray<int32>
{
    TArray<int32> Roots;
    if (Events.Num() == 0) return Roots;

    uint32 MinDepth = MAX_uint32;
    for (const FCk_TimingEvent& Evt : Events)
    {
        MinDepth = FMath::Min(MinDepth, Evt.Depth);
    }

    OutPool.Reserve(Events.Num());

    TMap<uint32, int32> RootByTimer;

    // Most recent pool index at each depth level. Events are sorted by start time then depth,
    // so an event's parent (the most recent event one level up) is always visited first.
    TArray<int32> StackByDepth;

    for (const FCk_TimingEvent& Evt : Events)
    {
        const int32 Level = static_cast<int32>(Evt.Depth - MinDepth);

        int32 NodeIndex = INDEX_NONE;

        if (Level == 0)
        {
            if (const int32* Existing = RootByTimer.Find(Evt.TimerIndex))
            {
                NodeIndex = *Existing;
            }
            else
            {
                NodeIndex = OutPool.Add(FCallTreeNode{Evt.TimerIndex});
                RootByTimer.Add(Evt.TimerIndex, NodeIndex);
                Roots.Add(NodeIndex);
            }
        }
        else
        {
            if (Level - 1 >= StackByDepth.Num() || StackByDepth[Level - 1] == INDEX_NONE)
            {
                // Orphaned event (parent outside the analyzed window) — cannot attach
                continue;
            }

            const int32 ParentIndex = StackByDepth[Level - 1];
            if (const int32* Existing = OutPool[ParentIndex].ChildByTimer.Find(Evt.TimerIndex))
            {
                NodeIndex = *Existing;
            }
            else
            {
                NodeIndex = OutPool.Add(FCallTreeNode{Evt.TimerIndex});
                OutPool[ParentIndex].ChildByTimer.Add(Evt.TimerIndex, NodeIndex);
                OutPool[ParentIndex].Children.Add(NodeIndex);
            }
        }

        OutPool[NodeIndex].InclusiveSeconds += Evt.EndTime - Evt.StartTime;
        OutPool[NodeIndex].Count += 1;

        while (StackByDepth.Num() <= Level)
        {
            StackByDepth.Add(INDEX_NONE);
        }
        StackByDepth[Level] = NodeIndex;
    }

    return Roots;
}

auto
    FCk_JsonReport::
    SerializeCallTreeNode(int32 NodeIndex,
                          const TArray<FCallTreeNode>& Pool,
                          const FCk_FrameReport::FTimerNameMap& TimerNames,
                          double MinInclusiveMs)
    -> TSharedPtr<FJsonObject>
{
    const FCallTreeNode& Node = Pool[NodeIndex];

    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), FCk_FrameReport::GetTimerName(TimerNames, Node.TimerIndex));
    Obj->SetNumberField(TEXT("inclusiveMs"), Round3(Node.InclusiveSeconds * 1000.0));

    // Exclusive = self time; computed from ALL children, including ones pruned from display
    double ChildrenInclusiveSeconds = 0.0;
    for (const int32 ChildIndex : Node.Children)
    {
        ChildrenInclusiveSeconds += Pool[ChildIndex].InclusiveSeconds;
    }
    const double ExclusiveMs = FMath::Max(0.0, (Node.InclusiveSeconds - ChildrenInclusiveSeconds) * 1000.0);
    Obj->SetNumberField(TEXT("exclusiveMs"), Round3(ExclusiveMs));
    Obj->SetNumberField(TEXT("count"), Node.Count);

    const auto SortedChildren = ck::algo::Sort(Node.Children, [&Pool](int32 A, int32 B)
    {
        return Pool[A].InclusiveSeconds > Pool[B].InclusiveSeconds;
    });

    TArray<TSharedPtr<FJsonValue>> ChildValues;
    for (const int32 ChildIndex : SortedChildren)
    {
        if (Pool[ChildIndex].InclusiveSeconds * 1000.0 < MinInclusiveMs)
        {
            break; // sorted descending — the rest are smaller
        }
        ChildValues.Add(MakeShared<FJsonValueObject>(
            SerializeCallTreeNode(ChildIndex, Pool, TimerNames, MinInclusiveMs)));
    }
    if (ChildValues.Num() > 0)
    {
        Obj->SetArrayField(TEXT("children"), ChildValues);
    }

    return Obj;
}

// --------------------------------------------------------------------------------------------------------------------
// Single Frame
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_JsonReport::
    MakeFrameDetails(const FCk_TraceSession& Session,
                     const FCk_FrameAnalysisResult& Result,
                     const FCk_FrameReportConfig& Config)
    -> TSharedPtr<FJsonObject>
{
    TSharedPtr<FJsonObject> Frame = MakeShared<FJsonObject>();
    Frame->SetNumberField(TEXT("frameIndex"), static_cast<double>(Result.FrameIndex));
    Frame->SetNumberField(TEXT("durationMs"), Round3(Result.FrameDurationMs));
    Frame->SetNumberField(TEXT("frameStartSeconds"), Result.FrameStartTime);
    Frame->SetNumberField(TEXT("frameEndSeconds"), Result.FrameEndTime);

    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();
    const FCk_FrameReport::FTimerNameMap TimerNames = FCk_FrameReport::BuildTimerNameMap(Session);
    if (Result.HasValidTimeRange)
    {
        Frame->SetObjectField(TEXT("accounting"), ck_json_report::Make_FrameAccounting(
            FCk_FrameReport::ComputeFrameAccounting(Result, TimerNames), true));
    }

    // ---- Call tree ----

    TArray<FCallTreeNode> Pool;
    TArray<int32> Roots = BuildCallTree(Result.Events, Pool);
    ck::algo::Sort(Roots, [&Pool](int32 A, int32 B)
    {
        return Pool[A].InclusiveSeconds > Pool[B].InclusiveSeconds;
    });

    TArray<TSharedPtr<FJsonValue>> TreeValues;
    for (const int32 RootIndex : Roots)
    {
        if (Pool[RootIndex].InclusiveSeconds * 1000.0 < Config.MinInclusiveMs)
        {
            break; // sorted descending — the rest are smaller
        }
        TreeValues.Add(MakeShared<FJsonValueObject>(
            SerializeCallTreeNode(RootIndex, Pool, TimerNames, Config.MinInclusiveMs)));
    }
    if (TreeValues.Num() > 0)
    {
        Frame->SetArrayField(TEXT("callTree"), TreeValues);
    }

    // ---- Top timers by exclusive time ----

    TArray<TPair<uint32, double>> ExclSorted;
    for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
    {
        ExclSorted.Emplace(TimerIndex, ExclSec * 1000.0);
    }
    ck::algo::Sort(ExclSorted, [](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
    {
        return A.Value > B.Value;
    });

    TArray<TSharedPtr<FJsonValue>> TimerValues;
    const int32 TopCount = FMath::Min(ExclSorted.Num(), Config.RawTimerCount);
    for (int32 i = 0; i < TopCount; ++i)
    {
        TSharedPtr<FJsonObject> TimerObj = MakeShared<FJsonObject>();
        TimerObj->SetStringField(TEXT("name"), FCk_FrameReport::GetTimerName(TimerNames, ExclSorted[i].Key));
        TimerObj->SetNumberField(TEXT("exclusiveMs"), Round3(ExclSorted[i].Value));
        TimerObj->SetNumberField(TEXT("inclusiveMs"), Round3(Result.GetInclusiveMs(ExclSorted[i].Key)));
        TimerObj->SetNumberField(TEXT("count"), Result.GetCount(ExclSorted[i].Key));
        TimerValues.Add(MakeShared<FJsonValueObject>(TimerObj));
    }
    if (TimerValues.Num() > 0)
    {
        Frame->SetArrayField(TEXT("topTimers"), TimerValues);
    }

    // ---- Categories (exclusive time) ----

    const FCk_TimerCategorizer Categorizer;

    TMap<FString, double> CategoryExclMs;
    for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
    {
        const double ExclMs = ExclSec * 1000.0;

        const FString Category = Categorizer.Categorize(
            FCk_FrameReport::GetTimerName(TimerNames, TimerIndex));
        CategoryExclMs.FindOrAdd(Category, 0.0) += ExclMs;
    }

    TArray<TPair<FString, double>> SortedCategories;
    for (const auto& [CatName, ExclMs] : CategoryExclMs)
    {
        if (ExclMs >= Config.MinCategoryMs)
        {
            SortedCategories.Emplace(CatName, ExclMs);
        }
    }
    ck::algo::Sort(SortedCategories, [](const TPair<FString, double>& A, const TPair<FString, double>& B)
    {
        return A.Value > B.Value;
    });

    TArray<TSharedPtr<FJsonValue>> CategoryValues;
    for (const auto& [CatName, ExclMs] : SortedCategories)
    {
        TSharedPtr<FJsonObject> CatObj = MakeShared<FJsonObject>();
        CatObj->SetStringField(TEXT("name"), CatName);
        CatObj->SetNumberField(TEXT("exclusiveMs"), Round3(ExclMs));
        const double Pct = (Result.FrameDurationMs > 0.0) ? (ExclMs / Result.FrameDurationMs) * 100.0 : 0.0;
        CatObj->SetNumberField(TEXT("pctOfFrame"), Round3(Pct));
        CategoryValues.Add(MakeShared<FJsonValueObject>(CatObj));
    }
    if (CategoryValues.Num() > 0)
    {
        Frame->SetArrayField(TEXT("categories"), CategoryValues);
    }

    // ---- Worker threads ----

    const TArray<FCk_WorkerThreadSummary> Workers = FCk_FrameReport::ComputeWorkerThreadSummaries(
        Session, Result, Config.MinWorkerThreadMs);

    TArray<TSharedPtr<FJsonValue>> WorkerValues;
    const int32 MaxWorkers = FMath::Min(Workers.Num(), Config.MaxWorkerThreads);
    for (int32 i = 0; i < MaxWorkers; ++i)
    {
        const FCk_WorkerThreadSummary& Worker = Workers[i];

        TSharedPtr<FJsonObject> WorkerObj = MakeShared<FJsonObject>();
        WorkerObj->SetNumberField(TEXT("id"), Worker.ThreadId);
        WorkerObj->SetStringField(TEXT("name"), Worker.ThreadName);
        if (NOT Worker.ThreadGroup.IsEmpty())
        {
            WorkerObj->SetStringField(TEXT("group"), Worker.ThreadGroup);
        }
        WorkerObj->SetNumberField(TEXT("wallTimeMs"), Round3(Worker.WallTimeMs));
        WorkerObj->SetNumberField(TEXT("eventCount"), Worker.EventCount);

        TArray<TSharedPtr<FJsonValue>> TopValues;
        for (const FCk_WorkerThreadSummary::FTopTimer& Top : Worker.TopTimers)
        {
            TSharedPtr<FJsonObject> TopObj = MakeShared<FJsonObject>();
            TopObj->SetStringField(TEXT("name"), Top.Name);
            TopObj->SetNumberField(TEXT("exclusiveMs"), Round3(Top.ExclusiveMs));
            TopObj->SetNumberField(TEXT("count"), Top.Count);
            TopValues.Add(MakeShared<FJsonValueObject>(TopObj));
        }
        if (TopValues.Num() > 0)
        {
            WorkerObj->SetArrayField(TEXT("topTimers"), TopValues);
        }

        WorkerValues.Add(MakeShared<FJsonValueObject>(WorkerObj));
    }
    if (WorkerValues.Num() > 0)
    {
        Frame->SetArrayField(TEXT("workerThreads"), WorkerValues);
    }

    // ---- Wait/stall breakdown ----

    const TArray<FCk_WaitThreadSummary> Waits = FCk_FrameReport::ComputeWaitSummaries(
        Session, Result, Config.MinWaitMs);

    TArray<TSharedPtr<FJsonValue>> WaitValues;
    for (const FCk_WaitThreadSummary& Wait : Waits)
    {
        TSharedPtr<FJsonObject> WaitObj = MakeShared<FJsonObject>();
        WaitObj->SetNumberField(TEXT("id"), Wait.ThreadId);
        WaitObj->SetStringField(TEXT("name"), Wait.ThreadName);
        WaitObj->SetBoolField(TEXT("isGameThread"), Wait.bIsGameThread);
        WaitObj->SetNumberField(TEXT("waitMs"), Round3(Wait.WaitMs));
        WaitObj->SetNumberField(TEXT("wallMs"), Round3(Wait.WallMs));

        TArray<TSharedPtr<FJsonValue>> TopValues;
        for (const FCk_WaitThreadSummary::FWaitScope& Top : Wait.TopWaits)
        {
            TSharedPtr<FJsonObject> TopObj = MakeShared<FJsonObject>();
            TopObj->SetStringField(TEXT("name"), Top.Name);
            TopObj->SetNumberField(TEXT("exclusiveMs"), Round3(Top.ExclusiveMs));
            TopObj->SetNumberField(TEXT("count"), Top.Count);
            TopValues.Add(MakeShared<FJsonValueObject>(TopObj));
        }
        if (TopValues.Num() > 0)
        {
            WaitObj->SetArrayField(TEXT("topWaits"), TopValues);
        }

        WaitValues.Add(MakeShared<FJsonValueObject>(WaitObj));
    }
    if (WaitValues.Num() > 0)
    {
        Frame->SetArrayField(TEXT("waitBreakdown"), WaitValues);
    }

    return Frame;
}

auto
    FCk_JsonReport::
    GenerateSingleFrame(const FCk_TraceSession& Session,
                        const FCk_FrameAnalysisResult& Result,
                        const FCk_FrameReportConfig& Config)
    -> FString
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schemaVersion"), 3);

    TSharedPtr<FJsonObject> Generator = MakeShared<FJsonObject>();
    Generator->SetStringField(TEXT("name"), TEXT("CkInsightsAnalyzer"));
    Generator->SetStringField(TEXT("reportKind"), TEXT("singleFrame"));
    Generator->SetStringField(TEXT("accountingScope"), TEXT("GameThread only; elapsed frame time is not asserted CPU busy"));
    Generator->SetStringField(TEXT("accountingValidity"), TEXT("exclusiveCoverageErrorMs beyond floating-point tolerance means exclusive attribution does not reconcile with interval coverage"));
    Generator->SetStringField(TEXT("waitClassification"), TEXT("Named wait scopes only; heuristic, not a critical-path inference"));
    Generator->SetStringField(TEXT("inclusiveSemantics"), TEXT("inclusive timing is nested and non-additive"));
    Root->SetObjectField(TEXT("generator"), Generator);
    Root->SetObjectField(TEXT("trace"), MakeTraceOverview(Session));
    Root->SetNumberField(TEXT("budgetMs"), Round3(Config.TargetFrameMs));
    Root->SetObjectField(TEXT("singleFrame"), MakeFrameDetails(Session, Result, Config));
    return ToJsonString(Root);
}

// --------------------------------------------------------------------------------------------------------------------
// Multi Frame
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_JsonReport::
    GenerateMultiFrame(const FCk_TraceSession& Session,
                       const FCk_MultiFrameStats& Stats,
                       const FCk_MultiFrameReportConfig& Config)
    -> FString
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schemaVersion"), 3);

    TSharedPtr<FJsonObject> Generator = MakeShared<FJsonObject>();
    Generator->SetStringField(TEXT("name"), TEXT("CkInsightsAnalyzer"));
    Generator->SetStringField(TEXT("reportKind"), TEXT("multiFrame"));
    Generator->SetNumberField(TEXT("hotFrameDetailLimit"), Config.WorstFrameCount);
    Generator->SetStringField(TEXT("accountingScope"), TEXT("GameThread only; elapsed frame time is not asserted CPU busy"));
    Generator->SetStringField(TEXT("accountingValidity"), TEXT("exclusiveCoverageErrorMs beyond floating-point tolerance means exclusive attribution does not reconcile with interval coverage"));
    Generator->SetStringField(TEXT("waitClassification"), TEXT("Named wait scopes only; heuristic, not a critical-path inference"));
    Generator->SetStringField(TEXT("inclusiveSemantics"), TEXT("avgInclusiveMs is nested and non-additive; avgOuterInclusiveMs unions same-timer outer scopes"));
    Root->SetObjectField(TEXT("generator"), Generator);
    Root->SetObjectField(TEXT("trace"), MakeTraceOverview(Session));
    Root->SetNumberField(TEXT("budgetMs"), Round3(Config.TargetFrameMs));

    TSharedPtr<FJsonObject> Multi = MakeShared<FJsonObject>();
    Multi->SetNumberField(TEXT("frameCount"), static_cast<double>(Stats.FrameCount));
    Multi->SetNumberField(TEXT("avgMs"), Round3(Stats.AvgFrameMs));
    Multi->SetNumberField(TEXT("minMs"), Round3(Stats.MinFrameMs));
    Multi->SetNumberField(TEXT("maxMs"), Round3(Stats.MaxFrameMs));
    Multi->SetNumberField(TEXT("p95Ms"), Round3(Stats.P95FrameMs));
    Multi->SetNumberField(TEXT("p99Ms"), Round3(Stats.P99FrameMs));
    Multi->SetBoolField(TEXT("waitAveragesComputed"), Stats.WaitAveragesComputed);

    if (Stats.AverageAccounting.IsSet())
    {
        Multi->SetObjectField(TEXT("averageAccounting"),
            ck_json_report::Make_FrameAccounting(*Stats.AverageAccounting, false));
    }
    if (NOT Stats.FrameAccounting.IsEmpty())
    {
        const auto AccountingValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(
            Stats.FrameAccounting, [](const FCk_FrameAccounting& Accounting) -> TSharedPtr<FJsonValue>
            {
                return MakeShared<FJsonValueObject>(ck_json_report::Make_FrameAccounting(Accounting, true));
            });
        Multi->SetArrayField(TEXT("frameAccounting"), AccountingValues);
    }

    TSharedPtr<FJsonObject> Thresholds = MakeShared<FJsonObject>();
    Thresholds->SetNumberField(TEXT("totalExclusiveMs"), Stats.TotalExclusiveMs);
    Thresholds->SetNumberField(TEXT("reportedTimerExclusiveMs"), Stats.ReportedTimerExclusiveMs);
    Thresholds->SetNumberField(TEXT("omittedTimerExclusiveMs"), Stats.OmittedTimerExclusiveMs);
    Thresholds->SetNumberField(TEXT("reportedCategoryExclusiveMs"), Stats.ReportedCategoryExclusiveMs);
    Thresholds->SetNumberField(TEXT("omittedCategoryExclusiveMs"), Stats.OmittedCategoryExclusiveMs);
    Thresholds->SetStringField(TEXT("units"), TEXT("milliseconds per analysed GameThread frame"));
    Multi->SetObjectField(TEXT("thresholdOmissionAccounting"), Thresholds);

    // The SELECTION, not the outcome: frameCount above is what survived analysis, these runs are
    // what was asked for. Inclusive on both ends, matching FCk_FrameRun.
    if (NOT Stats.SelectedRuns.IsEmpty())
    {
        const auto RunValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(Stats.SelectedRuns,
            [](const FCk_FrameRun& InRun) -> TSharedPtr<FJsonValue>
            {
                TSharedPtr<FJsonObject> RunObj = MakeShared<FJsonObject>();
                RunObj->SetNumberField(TEXT("firstFrame"), static_cast<double>(InRun.FirstFrame));
                RunObj->SetNumberField(TEXT("lastFrame"), static_cast<double>(InRun.LastFrame));

                return MakeShared<FJsonValueObject>(RunObj);
            });

        Multi->SetArrayField(TEXT("selectedRuns"), RunValues);
        Multi->SetStringField(TEXT("selectedFrames"),
            FCk_MultiFrameReport::DoGet_FrameRunsLabel(Stats.SelectedRuns));
        Multi->SetNumberField(TEXT("selectedFrameCount"),
            static_cast<double>(FCk_MultiFrameReport::DoGet_SelectedFrameCount(Stats.SelectedRuns)));
    }

    // The ordinal space of the per-frame series: what was actually analysed, which the runs above are
    // not (they still contain excluded and failed frames).
    if (NOT Stats.AnalysedFrameIndices.IsEmpty())
    {
        const auto AnalysedValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(
            Stats.AnalysedFrameIndices,
            [](uint64 InFrameIndex) -> TSharedPtr<FJsonValue>
            {
                return MakeShared<FJsonValueNumber>(static_cast<double>(InFrameIndex));
            });

        Multi->SetArrayField(TEXT("analysedFrameIndices"), AnalysedValues);
    }

    if (NOT Stats.MergedHotPaths.IsEmpty())
    {
        const auto MergedValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(
            Stats.MergedHotPaths,
            [](const TSharedPtr<FCk_MergedHotPathNode>& InRoot) -> TSharedPtr<FJsonValue>
            {
                return MakeShared<FJsonValueObject>(
                    ck_json_report::Make_MergedHotPathNode(*InRoot, &FCk_JsonReport::Round3));
            });

        Multi->SetArrayField(TEXT("mergedHotPaths"), MergedValues);
    }

    TArray<TSharedPtr<FJsonValue>> WorstValues;
    for (const FCk_FrameSummary& Frame : Stats.WorstFrames)
    {
        TSharedPtr<FJsonObject> FrameObj = MakeShared<FJsonObject>();
        FrameObj->SetNumberField(TEXT("frameIndex"), static_cast<double>(Frame.FrameIndex));
        FrameObj->SetNumberField(TEXT("durationMs"), Round3(Frame.DurationMs));
        FrameObj->SetStringField(TEXT("dominantCost"), Frame.DominantCost);
        FrameObj->SetNumberField(TEXT("dominantCostMs"), Round3(Frame.DominantCostMs));
        WorstValues.Add(MakeShared<FJsonValueObject>(FrameObj));
    }
    if (WorstValues.Num() > 0)
    {
        Multi->SetArrayField(TEXT("worstFrames"), WorstValues);
    }

    FCk_FrameReportConfig HotFrameConfig;
    HotFrameConfig.Depth = Config.Depth;
    HotFrameConfig.TargetFrameMs = Config.TargetFrameMs;
    HotFrameConfig.ApplyDepth();

    TArray<TSharedPtr<FJsonValue>> HotFrameValues;
    for (const FCk_HotFrameDetails& HotFrame : Stats.HotFrames)
    {
        TSharedPtr<FJsonObject> HotFrameObj = MakeFrameDetails(Session, HotFrame.Analysis, HotFrameConfig);
        HotFrameObj->SetStringField(TEXT("dominantCost"), HotFrame.Summary.DominantCost);
        HotFrameObj->SetNumberField(TEXT("dominantCostMs"), Round3(HotFrame.Summary.DominantCostMs));
        HotFrameValues.Add(MakeShared<FJsonValueObject>(HotFrameObj));
    }
    if (HotFrameValues.Num() > 0)
    {
        Multi->SetArrayField(TEXT("hotFrames"), HotFrameValues);
    }

    TArray<TSharedPtr<FJsonValue>> CategoryValues;
    for (const FCk_MultiFrameStats::FCategoryStats& Cat : Stats.CategoryAverages)
    {
        TSharedPtr<FJsonObject> CatObj = MakeShared<FJsonObject>();
        CatObj->SetStringField(TEXT("name"), Cat.Name);
        CatObj->SetNumberField(TEXT("avgExclusiveMs"), Round3(Cat.AvgExclMs));
        CatObj->SetNumberField(TEXT("p95ExclusiveMs"), Round3(Cat.P95ExclMs));
        CatObj->SetNumberField(TEXT("pctOfTotal"), Round3(Cat.TotalPct));
        CategoryValues.Add(MakeShared<FJsonValueObject>(CatObj));
    }
    if (Config.ShowCategoryAverages && CategoryValues.Num() > 0)
    {
        Multi->SetArrayField(TEXT("categoryAverages"), CategoryValues);
    }

    const auto TimerValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(Stats.TimerAverages,
        [](const FCk_MultiFrameStats::FTimerStats& InTimer) -> TSharedPtr<FJsonValue>
        {
            TSharedPtr<FJsonObject> TimerObj = MakeShared<FJsonObject>();
            TimerObj->SetStringField(TEXT("name"), InTimer.Name);
            TimerObj->SetStringField(TEXT("category"), InTimer.Category);
            TimerObj->SetNumberField(TEXT("avgExclusiveMs"), Round3(InTimer.AvgExclMs));
            TimerObj->SetNumberField(TEXT("p95ExclusiveMs"), Round3(InTimer.P95ExclMs));
            TimerObj->SetNumberField(TEXT("maxExclusiveMs"), Round3(InTimer.MaxExclMs));
            TimerObj->SetNumberField(TEXT("avgOuterInclusiveMs"), Round3(InTimer.AvgOuterInclMs));
            TimerObj->SetNumberField(TEXT("avgInclusiveMs"), Round3(InTimer.AvgInclMs));
            TimerObj->SetNumberField(TEXT("avgCount"), Round3(InTimer.AvgCount));
            TimerObj->SetNumberField(TEXT("framesPresent"), static_cast<double>(InTimer.FramesPresent));

            return MakeShared<FJsonValueObject>(TimerObj);
        });

    if (NOT TimerValues.IsEmpty())
    {
        Multi->SetArrayField(TEXT("timerAverages"), TimerValues);
    }

    if (Stats.WaitAveragesComputed)
    {
        TArray<TSharedPtr<FJsonValue>> WaitValues;
        for (const FCk_WaitThreadSummary& Wait : Stats.WaitAverages)
        {
            TSharedPtr<FJsonObject> WaitObj = MakeShared<FJsonObject>();
            WaitObj->SetNumberField(TEXT("id"), Wait.ThreadId);
            WaitObj->SetStringField(TEXT("name"), Wait.ThreadName);
            WaitObj->SetBoolField(TEXT("isGameThread"), Wait.bIsGameThread);
            WaitObj->SetNumberField(TEXT("waitMs"), Round3(Wait.WaitMs));
            WaitObj->SetNumberField(TEXT("wallMs"), Round3(Wait.WallMs));
            TArray<TSharedPtr<FJsonValue>> TopValues;
            for (const FCk_WaitThreadSummary::FWaitScope& Top : Wait.TopWaits)
            {
                TSharedPtr<FJsonObject> TopObj = MakeShared<FJsonObject>();
                TopObj->SetStringField(TEXT("name"), Top.Name);
                TopObj->SetNumberField(TEXT("exclusiveMs"), Round3(Top.ExclusiveMs));
                TopObj->SetNumberField(TEXT("count"), Top.Count);
                TopValues.Add(MakeShared<FJsonValueObject>(TopObj));
            }
            if (NOT TopValues.IsEmpty())
            { WaitObj->SetArrayField(TEXT("topWaits"), TopValues); }
            WaitValues.Add(MakeShared<FJsonValueObject>(WaitObj));
        }
        Multi->SetArrayField(TEXT("waitAverages"), WaitValues);
    }

    if (Stats.ExcludedScreenshotFrameCount > 0)
    {
        Multi->SetNumberField(
            TEXT("excludedScreenshotFrames"),
            static_cast<double>(Stats.ExcludedScreenshotFrameCount));
    }

    // Screenshot frames that stayed in the averages but were skipped by the worst-frame ranking
    // (capture cost, not game cost). Absent when screenshots were off or excluded.
    if (Stats.ScreenshotFrameIndices.Num() > 0)
    {
        const auto ScreenshotValues = ck::algo::Transform<TArray<TSharedPtr<FJsonValue>>>(
            Stats.ScreenshotFrameIndices,
            [](uint64 InFrameIndex) -> TSharedPtr<FJsonValue>
            {
                return MakeShared<FJsonValueNumber>(static_cast<double>(InFrameIndex));
            });
        Multi->SetArrayField(TEXT("screenshotFrames"), ScreenshotValues);
    }

    Root->SetObjectField(TEXT("multiFrame"), Multi);
    return ToJsonString(Root);
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_frame_report
{
    const FString Pipe  = TEXT("\u2502  ");    // │  (continuing branch)
    const FString Tee   = TEXT("\u251C\u2500 "); // ├─ (sibling)
    const FString Ell   = TEXT("\u2514\u2500 "); // └─ (last child)
    const FString Space = TEXT("   ");           //    (after last child)
    const FString HRule = TEXT("\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500");

    const TSet<FString> FrameWrapperNames = {
        TEXT("FEngineLoop::Tick"),
        TEXT("FrameTime"),
        TEXT("Frame"),
        TEXT("BeginFrame"),
        TEXT("STAT_EventLoop_TEventLoop_RunOnce"),
        TEXT("FStats::AdvanceFrame"),
        TEXT("FRHIBreadcrumbEvent_GameThread_Begin"),
    };

    auto Get_WallMsAtMinDepth(const TArray<FCk_TimingEvent>& InEvents) -> double
    {
        uint32 MinDepth = MAX_uint32;
        for (const FCk_TimingEvent& Evt : InEvents)
        {
            MinDepth = FMath::Min(MinDepth, Evt.Depth);
        }

        double WallMs = 0.0;
        for (const FCk_TimingEvent& Evt : InEvents)
        {
            if (Evt.Depth == MinDepth)
            {
                WallMs += (Evt.EndTime - Evt.StartTime) * 1000.0;
            }
        }
        return WallMs;
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_FrameReport::FCk_FrameReport() = default;

FCk_FrameReport::FCk_FrameReport(const FCk_FrameReportConfig& Config)
    : _Config(Config)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    BuildTimerNameMap(const FCk_TraceSession& Session)
    -> FTimerNameMap
{
    FTimerNameMap Map;

    Session.ReadTimers(
        [&Map](const TraceServices::ITimingProfilerTimerReader& Reader)
        {
            const uint32 Count = Reader.GetTimerCount();
            for (uint32 i = 0; i < Count; ++i)
            {
                if (const TraceServices::FTimingProfilerTimer* Timer = Reader.GetTimer(i))
                {
                    if (Timer->Name)
                    {
                        Map.Add(Timer->Id, FString(Timer->Name));
                    }
                }
            }
        });

    return Map;
}

auto
    FCk_FrameReport::
    GetTimerName(const FTimerNameMap& Names, uint32 TimerIndex)
    -> FString
{
    if (const FString* Name = Names.Find(TimerIndex))
    {
        return *Name;
    }
    return FString::Printf(TEXT("UNKNOWN_%u"), TimerIndex);
}

auto
    FCk_FrameReport::
    ComputeFrameAccounting(const FCk_FrameAnalysisResult& Result,
                           const FTimerNameMap& TimerNames) -> FCk_FrameAccounting
{
    FCk_FrameAccounting Accounting;
    if (NOT Result.HasValidTimeRange)
    { return Accounting; }

    Accounting.FrameIndex = Result.FrameIndex;
    Accounting.ThreadId = Result.ThreadId;
    Accounting.StartTime = Result.FrameStartTime;
    Accounting.EndTime = Result.FrameEndTime;
    Accounting.FrameMs = Result.FrameDurationMs;
    Accounting.InstrumentedMs = Result.InstrumentedMs;
    for (const auto& [TimerIndex, ExclusiveSeconds] : Result.TimerExclusive)
    {
        const double ExclusiveMs = ExclusiveSeconds * 1000.0;
        Accounting.ExclusiveSumMs += ExclusiveMs;
        if (FCk_TimerCategorizer::IsWaitTimer(GetTimerName(TimerNames, TimerIndex)))
        { Accounting.NamedWaitMs += ExclusiveMs; }
    }
    Accounting.OtherInstrumentedMs = FMath::Max(0.0, Accounting.InstrumentedMs - Accounting.NamedWaitMs);
    Accounting.UninstrumentedMs = FMath::Max(0.0, Accounting.FrameMs - Accounting.InstrumentedMs);
    Accounting.ExclusiveCoverageErrorMs = Accounting.ExclusiveSumMs - Accounting.InstrumentedMs;
    return Accounting;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    Generate(const FCk_TraceSession& Session,
             const FCk_FrameAnalysisResult& Result) const
    -> FString
{
    if (NOT Result.IsValid())
    {
        return TEXT("(No analysis data)");
    }

    TArray<FString> Lines;
    Lines.Reserve(128);

    // The overview accessors each take their own read scope — emit before the
    // scope below so the session read lock is never acquired recursively.
    GenerateTraceOverview(Session, Lines);

    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();
    const FTimerNameMap TimerNames = BuildTimerNameMap(Session);

    GenerateHeader(Result, Lines);
    GenerateHotPaths(Result, TimerNames, Lines);

    if (_Config.ShowCategorySummary)
    {
        GenerateCategorySummary(Result, TimerNames, Lines);
    }

    if (_Config.ShowWaitBreakdown)
    {
        GenerateWaitBreakdown(Session, Result, Lines);
    }

    if (_Config.ShowWorkerThreads)
    {
        GenerateWorkerThreads(Session, Result, Lines);
    }

    if (_Config.ShowRawTimerList)
    {
        GenerateRawTimerList(Result, TimerNames, Lines);
    }

    return FString::Join(Lines, TEXT("\n"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    GenerateTraceOverview(const FCk_TraceSession& Session,
                          TArray<FString>& Lines)
    -> void
{
    const uint64 RenderFrames = Session.GetRenderFrameCount();
    const FString RenderFramesStr = RenderFrames > 0
        ? FString::Printf(TEXT(", %llu render frames"), RenderFrames)
        : FString{};

    Lines.Add(FString::Printf(TEXT("_Trace: %s \u2014 %.1fs, %llu game frames%s, %d threads_"),
        *FPaths::GetCleanFilename(Session.GetFilePath()),
        Session.GetDurationSeconds(),
        Session.GetFrameCount(),
        *RenderFramesStr,
        Session.GetThreadInfos().Num()));
}

auto
    FCk_FrameReport::
    GenerateHeader(const FCk_FrameAnalysisResult& Result,
                   TArray<FString>& Lines) const
    -> void
{
    const double FrameMs = Result.FrameDurationMs;
    const double OverBudget = FrameMs / _Config.TargetFrameMs;

    const FString Icon = FCk_TimerCategorizer::SeverityIcon(FrameMs);
    const FString FrameStr = FString::Printf(TEXT("%.1fms"), FrameMs);

    Lines.Add(FString::Printf(TEXT("%s *Frame Analysis: %s (%.1fx over %.1fms budget)*\n"),
        *Icon, *FrameStr, OverBudget, _Config.TargetFrameMs));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    IsFrameWrapper(const FString& TimerName)
    -> bool
{
    return ck_frame_report::FrameWrapperNames.Contains(TimerName);
}

auto
    FCk_FrameReport::
    UnwrapRoots(uint32 ParentTimerIndex,
                const FCk_FrameAnalysisResult& Result,
                const FTimerNameMap& TimerNames,
                int32 Depth) const
    -> TMap<uint32, double>
{
    TMap<uint32, double> Roots;

    const auto Children = Result.ChildrenOf.Find(ParentTimerIndex);
    if (NOT Children) return Roots;

    for (const auto& [ChildIndex, ChildInclSec] : *Children)
    {
        const double ChildInclMs = ChildInclSec * 1000.0;
        if (ChildInclMs < 0.5)
        {
            continue;
        }

        const FString ChildName = GetTimerName(TimerNames, ChildIndex);

        // Unnamed timers (no entry in the trace's timer table) behave like frame
        // wrappers: drill through so real work (UWorld_Tick, Slate) surfaces as
        // roots instead of an opaque UNKNOWN_<id> node covering the whole frame.
        const bool IsUnnamed = ChildName.StartsWith(TEXT("UNKNOWN_"));

        if ((IsFrameWrapper(ChildName) || IsUnnamed) && Depth < 5)
        {
            TMap<uint32, double> Deeper = UnwrapRoots(ChildIndex, Result, TimerNames, Depth + 1);

            const bool IsUnnamedLeaf = Deeper.Num() == 0 && IsUnnamed;
            if (IsUnnamedLeaf)
            {
                double& Existing = Roots.FindOrAdd(ChildIndex, 0.0);
                Existing += ChildInclSec;
                continue;
            }

            for (const auto& [K, V] : Deeper)
            {
                double& Existing = Roots.FindOrAdd(K, 0.0);
                Existing += V;
            }
        }
        else
        {
            double& Existing = Roots.FindOrAdd(ChildIndex, 0.0);
            Existing += ChildInclSec;
        }
    }

    return Roots;
}

auto
    FCk_FrameReport::
    GetSignificantChildren(uint32 ParentTimerIndex,
                           const FCk_FrameAnalysisResult& Result,
                           double MinInclusiveMs)
    -> TArray<FChildInfo>
{
    TArray<FChildInfo> Children;

    const auto ChildMap = Result.ChildrenOf.Find(ParentTimerIndex);
    if (NOT ChildMap) return Children;

    for (const auto& [ChildIndex, ChildInclSec] : *ChildMap)
    {
        const double InclMs = ChildInclSec * 1000.0;
        if (InclMs >= MinInclusiveMs)
        {
            Children.Add(FChildInfo{
                ChildIndex,
                InclMs,
                Result.GetExclusiveMs(ChildIndex),
                Result.GetCount(ChildIndex)
            });
        }
    }

    ck::algo::Sort(Children, [](const FChildInfo& A, const FChildInfo& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    return Children;
}

auto
    FCk_FrameReport::
    CollapseWrappers(uint32 TimerIndex, double InclMs, double ExclMs, uint32 Count,
                     const FCk_FrameAnalysisResult& Result,
                     const FTimerNameMap& TimerNames) const
    -> FCollapsedTimer
{
    FCollapsedTimer Collapsed;
    Collapsed.TimerIndex = TimerIndex;
    Collapsed.InclusiveMs = InclMs;
    Collapsed.ExclusiveMs = ExclMs;
    Collapsed.Count = Count;

    uint32 CurrentIndex = TimerIndex;
    double CurrentIncl = InclMs;
    double CurrentExcl = ExclMs;

    constexpr auto MaxCollapseDepth = 10;
    for (int32 Iter = 0; Iter < MaxCollapseDepth; ++Iter)
    {
        // `script::<Class>` IS the attribution the tree exists to show — see CkInsightsAnalyzer/Claude.md.
        const bool IsScriptProcessorScope =
            GetTimerName(TimerNames, CurrentIndex).StartsWith(TEXT("script::"));
        if (IsScriptProcessorScope)
        {
            break;
        }

        const bool DoesSignificantSelfWork = CurrentExcl > CurrentIncl * 0.05 && CurrentExcl > 0.3;
        if (DoesSignificantSelfWork)
        {
            break;
        }

        TArray<FChildInfo> Children = GetSignificantChildren(CurrentIndex, Result, 0.5);
        if (Children.Num() == 0)
        {
            break;
        }

        const FChildInfo& TopChild = Children[0];

        const double ChildIncl = FMath::Min(TopChild.InclusiveMs, CurrentIncl);

        constexpr auto SecondaryPathMaxPctOfParent = 0.20;
        const bool SinglePath =
            (Children.Num() == 1) ||
            (Children.Num() >= 2 && Children[1].InclusiveMs < CurrentIncl * SecondaryPathMaxPctOfParent);

        if (SinglePath && ChildIncl > CurrentIncl * 0.7)
        {
            Collapsed.Breadcrumbs.Add(GetTimerName(TimerNames, CurrentIndex));
            CurrentIndex = TopChild.TimerIndex;
            CurrentIncl = ChildIncl;
            CurrentExcl = TopChild.ExclusiveMs;
            Collapsed.Count = TopChild.Count;
        }
        else
        {
            break;
        }
    }

    Collapsed.TimerIndex = CurrentIndex;
    Collapsed.InclusiveMs = CurrentIncl;
    Collapsed.ExclusiveMs = CurrentExcl;
    return Collapsed;
}

auto
    FCk_FrameReport::
    MakeTreePrefix(int32 Depth, const TMap<int32, bool>& IsLastAtDepth)
    -> FString
{
    if (Depth == 0) return FString();

    FString Prefix;
    for (int32 D = 1; D < Depth; ++D)
    {
        const auto IsLast = IsLastAtDepth.Find(D);
        if (IsLast && *IsLast)
        {
            Prefix += ck_frame_report::Space;
        }
        else
        {
            Prefix += ck_frame_report::Pipe;
        }
    }

    const auto IsLast = IsLastAtDepth.Find(Depth);
    if (IsLast && *IsLast)
    {
        Prefix += ck_frame_report::Ell;
    }
    else
    {
        Prefix += ck_frame_report::Tee;
    }

    return Prefix;
}

auto
    FCk_FrameReport::
    BuildTreeLines(uint32 TimerIndex, int32 Depth,
                   double InclMs, double ExclMs, uint32 Count,
                   const FCk_FrameAnalysisResult& Result,
                   const FTimerNameMap& TimerNames,
                   TSet<uint32>& ShownTimers,
                   TMap<int32, bool>& IsLastAtDepth,
                   const TArray<FString>* PreBreadcrumbs) const
    -> TArray<FString>
{
    TArray<FString> Lines;

    FCollapsedTimer Collapsed = CollapseWrappers(
        TimerIndex, InclMs, ExclMs, Count, Result, TimerNames);

    if (ShownTimers.Contains(Collapsed.TimerIndex))
    {
        return Lines;
    }
    ShownTimers.Add(Collapsed.TimerIndex);

    TArray<FString> AllBreadcrumbs;
    if (PreBreadcrumbs)
    {
        AllBreadcrumbs = *PreBreadcrumbs;
    }
    AllBreadcrumbs.Append(Collapsed.Breadcrumbs);

    const FString RawName = GetTimerName(TimerNames, Collapsed.TimerIndex);
    FString DisplayName = FCk_TimerCategorizer::SimplifyName(RawName);
    if (DisplayName.Len() > 50)
    {
        DisplayName = DisplayName.Left(50) + TEXT("...");
    }

    const FString Prefix = MakeTreePrefix(Depth, IsLastAtDepth);
    const FString Icon = FCk_TimerCategorizer::SeverityIcon(Collapsed.InclusiveMs);

    const bool ShowSelf = Collapsed.ExclusiveMs > 0.3
                       && Collapsed.ExclusiveMs > Collapsed.InclusiveMs * 0.08;
    const bool ShowCount = Collapsed.Count > 1;

    FString Line = FString::Printf(TEXT("%s%s `%s`  *%s*"),
        *Prefix, *Icon, *DisplayName,
        *FCk_TimerCategorizer::FormatMs(Collapsed.InclusiveMs));

    if (ShowSelf)
    {
        Line += FString::Printf(TEXT("  _%s self_"),
            *FCk_TimerCategorizer::FormatMs(Collapsed.ExclusiveMs));
    }
    if (ShowCount)
    {
        Line += FString::Printf(TEXT("  %s"),
            *FCk_TimerCategorizer::FormatCount(Collapsed.Count));
    }

    if (AllBreadcrumbs.Num() > 0)
    {
        TArray<FString> BcNames;
        const int32 Start = FMath::Max(0, AllBreadcrumbs.Num() - 2);
        for (int32 i = Start; i < AllBreadcrumbs.Num(); ++i)
        {
            FString Simplified = FCk_TimerCategorizer::SimplifyName(AllBreadcrumbs[i]);
            if (Simplified.Len() <= 30)
            {
                BcNames.Add(MoveTemp(Simplified));
            }
        }
        if (BcNames.Num() > 0)
        {
            Line += TEXT("  _(");
            for (int32 i = 0; i < BcNames.Num(); ++i)
            {
                if (i > 0) Line += TEXT(" \u2192 ");
                Line += BcNames[i];
            }
            Line += TEXT(")_");
        }
    }

    Lines.Add(MoveTemp(Line));

    if (Depth >= _Config.MaxTreeDepth)
    {
        return Lines;
    }

    const double MinChildMs = _Config.ShowAllChildren
        ? 0.0
        : FMath::Max(_Config.MinChildMs, Collapsed.InclusiveMs * _Config.MinChildPctOfParent);
    TArray<FChildInfo> Children = GetSignificantChildren(
        Collapsed.TimerIndex, Result, MinChildMs);

    struct FDedupedChild
    {
        uint32 TimerIndex;
        double InclusiveMs;
        double ExclusiveMs;
        uint32 Count;
        TArray<FString> Breadcrumbs;
    };
    TMap<uint32, FDedupedChild> Seen;

    for (const FChildInfo& Child : Children)
    {
        FCollapsedTimer CC = CollapseWrappers(
            Child.TimerIndex, Child.InclusiveMs, Child.ExclusiveMs, Child.Count,
            Result, TimerNames);

        const double GlobalIncl = FMath::Min(
            Result.GetInclusiveMs(CC.TimerIndex), Collapsed.InclusiveMs);
        // Global exclusive can exceed this parent's slice, which would print "self" larger than "incl".
        const double GlobalExcl = FMath::Min(Result.GetExclusiveMs(CC.TimerIndex), GlobalIncl);
        const uint32 GlobalCount = Result.GetCount(CC.TimerIndex);

        const auto Existing = Seen.Find(CC.TimerIndex);
        if (NOT Existing || GlobalIncl > Existing->InclusiveMs)
        {
            Seen.FindOrAdd(CC.TimerIndex) = FDedupedChild{
                CC.TimerIndex, GlobalIncl, GlobalExcl, GlobalCount, MoveTemp(CC.Breadcrumbs)
            };
        }
    }

    TArray<FDedupedChild> Deduped;
    for (auto& [Key, Val] : Seen)
    {
        Deduped.Add(MoveTemp(Val));
    }
    ck::algo::Sort(Deduped, [](const FDedupedChild& A, const FDedupedChild& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    // Deliberately NOT filtered by ShownTimers — each branch shows its full subtree; the check at
    // the top of BuildTreeLines is what prevents infinite recursion.
    const int32 MaxVisible = _Config.ShowAllChildren ? MAX_int32 : _Config.MaxVisibleChildren;
    TArray<FDedupedChild> Visible;
    for (int32 i = 0; i < Deduped.Num() && Visible.Num() < MaxVisible; ++i)
    {
        if (Deduped[i].TimerIndex == Collapsed.TimerIndex) continue;
        Visible.Add(MoveTemp(Deduped[i]));
    }

    // Reconciliation row (mirrors DoBuildTreeNode): children dropped by the threshold, the 8-child
    // cap, or dedup otherwise read as a silent gap under the parent. Computed before the recursion
    // so the last real child keeps a "├" glyph when the synthetic row takes the "└".
    double ShownChildrenMs = 0.0;
    for (const FDedupedChild& Child : Visible)
    {
        ShownChildrenMs += Child.InclusiveMs;
    }

    const auto ChildMap = Result.ChildrenOf.Find(Collapsed.TimerIndex);
    const double HiddenMs = Collapsed.InclusiveMs - Collapsed.ExclusiveMs - ShownChildrenMs;
    const int32 HiddenChildCount = ChildMap ? ChildMap->Num() - Visible.Num() : 0;
    const bool EmitHiddenRow = HiddenChildCount > 0
                            && HiddenMs >= FMath::Max(0.1, Collapsed.InclusiveMs * 0.02);

    for (int32 i = 0; i < Visible.Num(); ++i)
    {
        TMap<int32, bool> ChildIsLast = IsLastAtDepth;
        ChildIsLast.Add(Depth + 1, i == Visible.Num() - 1 && NOT EmitHiddenRow);

        TArray<FString> ChildLines = BuildTreeLines(
            Visible[i].TimerIndex, Depth + 1,
            Visible[i].InclusiveMs, Visible[i].ExclusiveMs, Visible[i].Count,
            Result, TimerNames, ShownTimers, ChildIsLast,
            &Visible[i].Breadcrumbs);

        Lines.Append(MoveTemp(ChildLines));
    }

    if (EmitHiddenRow)
    {
        TMap<int32, bool> RowIsLast = IsLastAtDepth;
        RowIsLast.Add(Depth + 1, true);

        Lines.Add(FString::Printf(TEXT("%s(+%d below threshold)  *%s*"),
            *MakeTreePrefix(Depth + 1, RowIsLast), HiddenChildCount,
            *FCk_TimerCategorizer::FormatMs(HiddenMs)));
    }

    return Lines;
}

auto
    FCk_FrameReport::
    GenerateHotPaths(const FCk_FrameAnalysisResult& Result,
                     const FTimerNameMap& TimerNames,
                     TArray<FString>& Lines) const
    -> void
{
    Lines.Add(TEXT("*Game Thread Hot Paths*\n"));

    if (Result.FrameRootTimerIndex == static_cast<uint32>(INDEX_NONE))
    {
        Lines.Add(TEXT("(No frame root found)"));
        return;
    }

    TMap<uint32, double> RootChildren = UnwrapRoots(
        Result.FrameRootTimerIndex, Result, TimerNames);

    struct FRootEntry
    {
        uint32 TimerIndex;
        double InclusiveMs;
        double ExclusiveMs;
        uint32 Count;
    };
    TArray<FRootEntry> RootList;

    for (const auto& [TimerIndex, InclSeconds] : RootChildren)
    {
        const double InclMs = InclSeconds * 1000.0;
        if (InclMs >= _Config.MinInclusiveMs)
        {
            RootList.Add(FRootEntry{
                TimerIndex,
                InclMs,
                Result.GetExclusiveMs(TimerIndex),
                Result.GetCount(TimerIndex)
            });
        }
    }

    ck::algo::Sort(RootList, [](const FRootEntry& A, const FRootEntry& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    const int32 MaxRoots = FMath::Min(RootList.Num(), _Config.MaxRootTimers);
    for (int32 i = 0; i < MaxRoots; ++i)
    {
        const FRootEntry& Root = RootList[i];
        TMap<int32, bool> IsLastAtDepth;
        TSet<uint32> ShownTimers; // per-root dedup

        TArray<FString> TreeLines = BuildTreeLines(
            Root.TimerIndex, 0,
            Root.InclusiveMs, Root.ExclusiveMs, Root.Count,
            Result, TimerNames, ShownTimers, IsLastAtDepth);

        Lines.Append(MoveTemp(TreeLines));
        Lines.Add(TEXT(""));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    DoBuildTreeNode(uint32 TimerIndex, int32 Depth,
                    double InclMs, double ExclMs, uint32 Count,
                    const FCk_FrameAnalysisResult& Result,
                    const FTimerNameMap& TimerNames,
                    TSet<uint32>& ShownTimers,
                    const TArray<FString>* PreBreadcrumbs) const
    -> TSharedPtr<FCk_HotPathNode>
{
    FCollapsedTimer Collapsed = CollapseWrappers(
        TimerIndex, InclMs, ExclMs, Count, Result, TimerNames);

    // Skipping an already-shown timer is what prevents infinite recursion.
    if (ShownTimers.Contains(Collapsed.TimerIndex))
    {
        return nullptr;
    }
    ShownTimers.Add(Collapsed.TimerIndex);

    TArray<FString> AllBreadcrumbs;
    if (PreBreadcrumbs)
    {
        AllBreadcrumbs = *PreBreadcrumbs;
    }
    AllBreadcrumbs.Append(Collapsed.Breadcrumbs);

    auto Node = MakeShared<FCk_HotPathNode>();
    Node->RawName = GetTimerName(TimerNames, Collapsed.TimerIndex);
    Node->DisplayName = FCk_TimerCategorizer::SimplifyName(Node->RawName);
    Node->Breadcrumbs = MoveTemp(AllBreadcrumbs);
    Node->InclusiveMs = Collapsed.InclusiveMs;
    Node->ExclusiveMs = Collapsed.ExclusiveMs;
    Node->Count = Collapsed.Count;

    if (Depth >= _Config.MaxTreeDepth)
    {
        return Node;
    }

    const double MinChildMs = _Config.ShowAllChildren
        ? 0.0
        : FMath::Max(_Config.MinChildMs, Collapsed.InclusiveMs * _Config.MinChildPctOfParent);
    TArray<FChildInfo> Children = GetSignificantChildren(
        Collapsed.TimerIndex, Result, MinChildMs);

    struct FDedupedChild
    {
        uint32 TimerIndex;
        double InclusiveMs;
        double ExclusiveMs;
        uint32 Count;
        TArray<FString> Breadcrumbs;
    };
    TMap<uint32, FDedupedChild> Seen;

    for (const FChildInfo& Child : Children)
    {
        FCollapsedTimer CC = CollapseWrappers(
            Child.TimerIndex, Child.InclusiveMs, Child.ExclusiveMs, Child.Count,
            Result, TimerNames);

        const double GlobalIncl = FMath::Min(
            Result.GetInclusiveMs(CC.TimerIndex), Collapsed.InclusiveMs);
        // Global exclusive can exceed this parent's slice, which would print "self" larger than "incl".
        const double GlobalExcl = FMath::Min(Result.GetExclusiveMs(CC.TimerIndex), GlobalIncl);
        const uint32 GlobalCount = Result.GetCount(CC.TimerIndex);

        const auto Existing = Seen.Find(CC.TimerIndex);
        if (NOT Existing || GlobalIncl > Existing->InclusiveMs)
        {
            Seen.FindOrAdd(CC.TimerIndex) = FDedupedChild{
                CC.TimerIndex, GlobalIncl, GlobalExcl, GlobalCount, MoveTemp(CC.Breadcrumbs)
            };
        }
    }

    TArray<FDedupedChild> Deduped;
    for (auto& [Key, Val] : Seen)
    {
        Deduped.Add(MoveTemp(Val));
    }
    ck::algo::Sort(Deduped, [](const FDedupedChild& A, const FDedupedChild& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    const int32 MaxVisible = _Config.ShowAllChildren ? MAX_int32 : _Config.MaxVisibleChildren;
    TArray<FDedupedChild> Visible;
    for (int32 i = 0; i < Deduped.Num() && Visible.Num() < MaxVisible; ++i)
    {
        if (Deduped[i].TimerIndex == Collapsed.TimerIndex) continue;
        Visible.Add(MoveTemp(Deduped[i]));
    }

    for (FDedupedChild& Child : Visible)
    {
        TSharedPtr<FCk_HotPathNode> ChildNode = DoBuildTreeNode(
            Child.TimerIndex, Depth + 1,
            Child.InclusiveMs, Child.ExclusiveMs, Child.Count,
            Result, TimerNames, ShownTimers,
            &Child.Breadcrumbs);

        if (ChildNode.IsValid())
        {
            Node->Children.Add(MoveTemp(ChildNode));
        }
    }

    // Reconciliation row — makes the sums visibly add up (self + shown children + this row ≈
    // inclusive) for children dropped by the threshold, the child cap, or the dedup above.
    // Deliberately also emitted when EVERY child was pruned.
    if (const auto ChildMap = Result.ChildrenOf.Find(Collapsed.TimerIndex))
    {
        double ShownChildrenMs = 0.0;
        for (const TSharedPtr<FCk_HotPathNode>& Child : Node->Children)
        {
            ShownChildrenMs += Child->InclusiveMs;
        }

        // Global child stats make this remainder go slightly negative when a child also appears
        // under another parent — the threshold below is what clamps it.
        const double HiddenMs = Node->InclusiveMs - Node->ExclusiveMs - ShownChildrenMs;
        const int32 HiddenChildCount = ChildMap->Num() - Node->Children.Num();

        if (HiddenChildCount > 0 && HiddenMs >= FMath::Max(0.1, Node->InclusiveMs * 0.02))
        {
            auto HiddenNode = MakeShared<FCk_HotPathNode>();
            HiddenNode->RawName = FString::Printf(TEXT("(+%d below threshold)"), HiddenChildCount);
            HiddenNode->DisplayName = HiddenNode->RawName;
            HiddenNode->InclusiveMs = HiddenMs;
            HiddenNode->ExclusiveMs = 0.0;
            HiddenNode->Count = static_cast<uint32>(HiddenChildCount);
            HiddenNode->bIsAggregate = true;
            Node->Children.Add(MoveTemp(HiddenNode));
        }
    }

    return Node;
}

auto
    FCk_FrameReport::
    BuildHotPathTree(const FCk_TraceSession& Session,
                     const FCk_FrameAnalysisResult& Result) const
    -> TArray<TSharedPtr<FCk_HotPathNode>>
{
    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();

    return BuildHotPathTree(Session, Result, BuildTimerNameMap(Session));
}

auto
    FCk_FrameReport::
    BuildHotPathTree(const FCk_TraceSession& Session,
                     const FCk_FrameAnalysisResult& Result,
                     const FTimerNameMap& TimerNames) const
    -> TArray<TSharedPtr<FCk_HotPathNode>>
{
    // A synthesized average's parent-child edges are divided by EVERY analysed frame, while the
    // floors below are absolute — an edge present in a minority of frames sinks under them, and the
    // tree comes back empty rather than wrong-looking.
    const auto ResultIsRealFrame = NOT Result.IsSynthesizedAverage;
    CK_ENSURE_IF_NOT(ResultIsRealFrame,
        TEXT("BuildHotPathTree needs a real frame - a synthesized averaged frame's parent-child edges ")
        TEXT("are presence-diluted means that the tree's thresholds then reject. Use ")
        TEXT("FCk_MultiFrameStats::MergedHotPaths instead."))
    { return {}; }

    TArray<TSharedPtr<FCk_HotPathNode>> Roots;

    if (NOT Result.IsValid() ||
        Result.FrameRootTimerIndex == static_cast<uint32>(INDEX_NONE))
    {
        return Roots;
    }

    TMap<uint32, double> RootChildren = UnwrapRoots(
        Result.FrameRootTimerIndex, Result, TimerNames);

    struct FRootEntry
    {
        uint32 TimerIndex;
        double InclusiveMs;
        double ExclusiveMs;
        uint32 Count;
    };
    TArray<FRootEntry> RootList;

    for (const auto& [TimerIndex, InclSeconds] : RootChildren)
    {
        const double InclMs = InclSeconds * 1000.0;
        if (InclMs >= _Config.MinInclusiveMs)
        {
            RootList.Add(FRootEntry{
                TimerIndex,
                InclMs,
                Result.GetExclusiveMs(TimerIndex),
                Result.GetCount(TimerIndex)
            });
        }
    }

    ck::algo::Sort(RootList, [](const FRootEntry& A, const FRootEntry& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    const int32 MaxRoots = FMath::Min(RootList.Num(), _Config.MaxRootTimers);
    for (int32 i = 0; i < MaxRoots; ++i)
    {
        const FRootEntry& Root = RootList[i];
        TSet<uint32> ShownTimers; // per-root dedup, same as the markdown tree

        TSharedPtr<FCk_HotPathNode> Node = DoBuildTreeNode(
            Root.TimerIndex, 0,
            Root.InclusiveMs, Root.ExclusiveMs, Root.Count,
            Result, TimerNames, ShownTimers);

        if (Node.IsValid())
        {
            Roots.Add(MoveTemp(Node));
        }
    }

    return Roots;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    ComputeCategorySummary(const FCk_FrameAnalysisResult& Result,
                           const FTimerNameMap& TimerNames) const
    -> TArray<FCk_CategorySummaryEntry>
{
    TMap<FString, double> CategoryExclMs;

    for (const auto& [TimerIndex, ExclSeconds] : Result.TimerExclusive)
    {
        const double ExclMs = ExclSeconds * 1000.0;

        const FString TimerName = GetTimerName(TimerNames, TimerIndex);
        const FString Category = _Categorizer.Categorize(TimerName);

        double& Total = CategoryExclMs.FindOrAdd(Category, 0.0);
        Total += ExclMs;
    }

    const double FrameMs = Result.FrameDurationMs;

    TArray<FCk_CategorySummaryEntry> Entries;
    for (const auto& [CatName, ExclMs] : CategoryExclMs)
    {
        if (ExclMs >= _Config.MinCategoryMs)
        {
            Entries.Add(FCk_CategorySummaryEntry{
                CatName, ExclMs,
                (FrameMs > 0.0) ? (ExclMs / FrameMs) * 100.0 : 0.0
            });
        }
    }

    ck::algo::Sort(Entries, [](const FCk_CategorySummaryEntry& A, const FCk_CategorySummaryEntry& B)
    {
        return A.ExclusiveMs > B.ExclusiveMs;
    });

    return Entries;
}

auto
    FCk_FrameReport::
    ComputeTopTimers(const FCk_FrameAnalysisResult& Result,
                     const FTimerNameMap& TimerNames,
                     int32 MaxCount) const
    -> TArray<FCk_TopTimerEntry>
{
    TArray<TPair<uint32, double>> Sorted;
    for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
    {
        Sorted.Emplace(TimerIndex, ExclSec * 1000.0);
    }
    ck::algo::Sort(Sorted, [](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
    {
        return A.Value > B.Value;
    });

    const double FrameMs = Result.FrameDurationMs;
    const int32 Count = FMath::Min(Sorted.Num(), MaxCount);

    TArray<FCk_TopTimerEntry> Entries;
    Entries.Reserve(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        const uint32 TimerIndex = Sorted[i].Key;
        const double ExclMs = Sorted[i].Value;

        Entries.Add(FCk_TopTimerEntry{
            FCk_TimerCategorizer::SimplifyName(GetTimerName(TimerNames, TimerIndex)),
            ExclMs,
            Result.GetInclusiveMs(TimerIndex),
            Result.GetCount(TimerIndex),
            (FrameMs > 0.0) ? (ExclMs / FrameMs) * 100.0 : 0.0
        });
    }

    return Entries;
}

auto
    FCk_FrameReport::
    GenerateCategorySummary(const FCk_FrameAnalysisResult& Result,
                           const FTimerNameMap& TimerNames,
                           TArray<FString>& Lines) const
    -> void
{
    const TArray<FCk_CategorySummaryEntry> SortedCats = ComputeCategorySummary(Result, TimerNames);

    if (SortedCats.Num() == 0) return;

    Lines.Add(FString::Printf(TEXT("\n%s"), *ck_frame_report::HRule));
    Lines.Add(TEXT("*Category Summary (exclusive time)*\n"));

    for (const FCk_CategorySummaryEntry& Cat : SortedCats)
    {
        const FString Icon = FCk_TimerCategorizer::SeverityIcon(Cat.ExclusiveMs);
        const FString FormattedMs = FCk_TimerCategorizer::FormatMs(Cat.ExclusiveMs);

        Lines.Add(FString::Printf(TEXT("%s *%8s*  %4.0f%%  %s"),
            *Icon, *FormattedMs, Cat.PctOfFrame, *Cat.Name));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    ComputeWorkerThreadSummaries(const FCk_TraceSession& Session,
                                 const FCk_FrameAnalysisResult& GameThreadResult,
                                 double MinWorkerThreadMs)
    -> TArray<FCk_WorkerThreadSummary>
{
    // Worker summaries are read back out of the session over the frame's time window, which a
    // synthesized average does not have — see FCk_FrameAnalysisResult::IsSynthesizedAverage.
    const auto ResultIsRealFrame = NOT GameThreadResult.IsSynthesizedAverage;
    CK_ENSURE_IF_NOT(ResultIsRealFrame,
        TEXT("ComputeWorkerThreadSummaries needs a real frame - a synthesized averaged frame has no ")
        TEXT("session time window to re-read. Use FCk_MultiFrameStats aggregates instead."))
    { return {}; }

    const TArray<TraceServices::FThreadInfo> ThreadInfos = Session.GetThreadInfos();
    const uint32 GameThreadId = GameThreadResult.ThreadId;

    const FTimerNameMap TimerNames = BuildTimerNameMap(Session);

    TArray<FCk_WorkerThreadSummary> Workers;

    for (const TraceServices::FThreadInfo& Info : ThreadInfos)
    {
        if (Info.Id == GameThreadId) continue;

        FCk_FrameAnalysisResult ThreadResult = FCk_FrameAnalyzer::AnalyzeTimeRange(
            Session, Info.Id,
            GameThreadResult.FrameStartTime, GameThreadResult.FrameEndTime);

        if (NOT ThreadResult.IsValid()) continue;

        const double WallMs = ck_frame_report::Get_WallMsAtMinDepth(ThreadResult.Events);

        if (WallMs < MinWorkerThreadMs) continue;

        FCk_WorkerThreadSummary Summary;
        Summary.ThreadId = Info.Id;
        Summary.ThreadName = Info.Name ? FString(Info.Name) : FString::Printf(TEXT("Thread %u"), Info.Id);
        Summary.ThreadGroup = Info.GroupName ? FString(Info.GroupName) : FString();
        Summary.WallTimeMs = WallMs;
        Summary.EventCount = ThreadResult.Events.Num();

        TArray<TPair<uint32, double>> ExclSorted;
        for (const auto& [TimerIndex, ExclSec] : ThreadResult.TimerExclusive)
        {
            ExclSorted.Emplace(TimerIndex, ExclSec * 1000.0);
        }
        ck::algo::Sort(ExclSorted, [](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
        {
            return A.Value > B.Value;
        });

        constexpr auto TopTimersPerThread = 3;
        for (int32 i = 0; i < FMath::Min(ExclSorted.Num(), TopTimersPerThread); ++i)
        {
            const FString Name = GetTimerName(TimerNames, ExclSorted[i].Key);
            Summary.TopTimers.Add(FCk_WorkerThreadSummary::FTopTimer{
                FCk_TimerCategorizer::SimplifyName(Name),
                ExclSorted[i].Value,
                ThreadResult.GetCount(ExclSorted[i].Key)
            });
        }

        Workers.Add(MoveTemp(Summary));
    }

    ck::algo::Sort(Workers, [](const FCk_WorkerThreadSummary& A, const FCk_WorkerThreadSummary& B)
    {
        return A.WallTimeMs > B.WallTimeMs;
    });

    return Workers;
}

auto
    FCk_FrameReport::
    GenerateWorkerThreads(const FCk_TraceSession& Session,
                          const FCk_FrameAnalysisResult& GameThreadResult,
                          TArray<FString>& Lines) const
    -> void
{
    const TArray<FCk_WorkerThreadSummary> Workers = ComputeWorkerThreadSummaries(
        Session, GameThreadResult, _Config.MinWorkerThreadMs);

    if (Workers.Num() == 0) return;

    Lines.Add(FString::Printf(TEXT("\n%s"), *ck_frame_report::HRule));
    Lines.Add(FString::Printf(TEXT("*Worker Threads (>%.0fms)*\n"), _Config.MinWorkerThreadMs));

    const int32 MaxWorkers = FMath::Min(Workers.Num(), _Config.MaxWorkerThreads);
    for (int32 i = 0; i < MaxWorkers; ++i)
    {
        const FCk_WorkerThreadSummary& W = Workers[i];

        FString Label = W.ThreadName;
        if (NOT W.ThreadGroup.IsEmpty())
        {
            Label += FString::Printf(TEXT(" (%s)"), *W.ThreadGroup);
        }

        Lines.Add(FString::Printf(TEXT("*%s* %s *%s* wall"),
            *Label,
            *FCk_TimerCategorizer::SeverityIcon(W.WallTimeMs),
            *FCk_TimerCategorizer::FormatMs(W.WallTimeMs)));

        for (const auto& Top : W.TopTimers)
        {
            if (Top.ExclusiveMs < 0.5) break;

            FString ShortName = Top.Name;
            if (ShortName.Len() > 50)
            {
                ShortName = ShortName.Left(50) + TEXT("...");
            }

            Lines.Add(FString::Printf(TEXT("    %s *%s*  %s  `%s`"),
                *FCk_TimerCategorizer::SeverityIcon(Top.ExclusiveMs),
                *FCk_TimerCategorizer::FormatMs(Top.ExclusiveMs),
                *FCk_TimerCategorizer::FormatCount(Top.Count),
                *ShortName));
        }
    }
    Lines.Add(TEXT(""));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    ComputeWaitSummaries(const FCk_TraceSession& Session,
                         const FCk_FrameAnalysisResult& GameThreadResult,
                         double MinWaitMs)
    -> TArray<FCk_WaitThreadSummary>
{
    return ComputeWaitSummaries(Session, GameThreadResult, MinWaitMs, BuildTimerNameMap(Session));
}

auto
    FCk_FrameReport::
    ComputeWaitSummaries(const FCk_TraceSession& Session,
                         const FCk_FrameAnalysisResult& GameThreadResult,
                         double MinWaitMs,
                         const FTimerNameMap& TimerNames)
    -> TArray<FCk_WaitThreadSummary>
{
    // Every non-game-thread row is read back out of the session over the frame's time window, which
    // a synthesized average does not have — see FCk_FrameAnalysisResult::IsSynthesizedAverage.
    const auto ResultIsRealFrame = NOT GameThreadResult.IsSynthesizedAverage;
    CK_ENSURE_IF_NOT(ResultIsRealFrame,
        TEXT("ComputeWaitSummaries needs a real frame - a synthesized averaged frame has no session ")
        TEXT("time window to re-read. Use FCk_MultiFrameStats::WaitAverages instead."))
    { return {}; }

    const uint32 GameThreadId = GameThreadResult.ThreadId;

    // Exclusive time, so nested waits never double count.
    auto MakeSummary = [&TimerNames, MinWaitMs](
        const FCk_FrameAnalysisResult& ThreadResult, uint32 ThreadId, const FString& ThreadName,
        bool bIsGameThread, double WallMs)
        -> TOptional<FCk_WaitThreadSummary>
    {
        TArray<TPair<uint32, double>> WaitTimers;
        double WaitMs = 0.0;
        for (const auto& [TimerIndex, ExclSec] : ThreadResult.TimerExclusive)
        {
            if (NOT FCk_TimerCategorizer::IsWaitTimer(GetTimerName(TimerNames, TimerIndex)))
            {
                continue;
            }
            WaitMs += ExclSec * 1000.0;
            WaitTimers.Emplace(TimerIndex, ExclSec * 1000.0);
        }

        if (WaitMs < MinWaitMs)
        {
            return {};
        }

        ck::algo::Sort(WaitTimers, [](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
        {
            return A.Value > B.Value;
        });

        FCk_WaitThreadSummary Summary;
        Summary.ThreadId = ThreadId;
        Summary.ThreadName = ThreadName;
        Summary.bIsGameThread = bIsGameThread;
        Summary.WaitMs = WaitMs;
        Summary.WallMs = WallMs;

        for (int32 Index = 0; Index < FMath::Min(WaitTimers.Num(), 3); ++Index)
        {
            Summary.TopWaits.Add(FCk_WaitThreadSummary::FWaitScope{
                FCk_TimerCategorizer::SimplifyName(GetTimerName(TimerNames, WaitTimers[Index].Key)),
                WaitTimers[Index].Value,
                ThreadResult.GetCount(WaitTimers[Index].Key)});
        }

        return Summary;
    };

    TArray<FCk_WaitThreadSummary> Waits;

    for (const TraceServices::FThreadInfo& Info : Session.GetThreadInfos())
    {
        const FString ThreadName = Info.Name
            ? FString(Info.Name)
            : FString::Printf(TEXT("Thread %u"), Info.Id);

        if (Info.Id == GameThreadId)
        {
            constexpr auto IsGameThread = true;
            if (auto Summary = MakeSummary(GameThreadResult, Info.Id, ThreadName,
                    IsGameThread, GameThreadResult.FrameDurationMs))
            {
                Waits.Add(MoveTemp(*Summary));
            }
            continue;
        }

        const FCk_FrameAnalysisResult ThreadResult = FCk_FrameAnalyzer::AnalyzeTimeRange(
            Session, Info.Id,
            GameThreadResult.FrameStartTime, GameThreadResult.FrameEndTime);

        if (NOT ThreadResult.IsValid())
        {
            continue;
        }

        const double WallMs = ck_frame_report::Get_WallMsAtMinDepth(ThreadResult.Events);

        constexpr auto IsGameThread = false;
        if (auto Summary = MakeSummary(ThreadResult, Info.Id, ThreadName, IsGameThread, WallMs))
        {
            Waits.Add(MoveTemp(*Summary));
        }
    }

    ck::algo::Sort(Waits, [](const FCk_WaitThreadSummary& A, const FCk_WaitThreadSummary& B)
    {
        if (A.bIsGameThread != B.bIsGameThread)
        {
            return A.bIsGameThread;
        }
        return A.WaitMs > B.WaitMs;
    });

    return Waits;
}

auto
    FCk_FrameReport::
    GenerateWaitBreakdown(const FCk_TraceSession& Session,
                          const FCk_FrameAnalysisResult& GameThreadResult,
                          TArray<FString>& Lines) const
    -> void
{
    const TArray<FCk_WaitThreadSummary> Waits = ComputeWaitSummaries(
        Session, GameThreadResult, _Config.MinWaitMs);

    if (Waits.Num() == 0) return;

    Lines.Add(FString::Printf(TEXT("\n%s"), *ck_frame_report::HRule));
    Lines.Add(FString::Printf(TEXT("*Wait/Stall Breakdown (>%.0fms)*\n"), _Config.MinWaitMs));

    for (const FCk_WaitThreadSummary& W : Waits)
    {
        const double PctOfWall = W.WallMs > 0.0 ? (W.WaitMs / W.WallMs) * 100.0 : 0.0;

        Lines.Add(FString::Printf(TEXT("*%s* %s *%s* waiting  _(%.0f%% of %s wall)_"),
            *W.ThreadName,
            *FCk_TimerCategorizer::SeverityIcon(W.WaitMs),
            *FCk_TimerCategorizer::FormatMs(W.WaitMs),
            PctOfWall,
            *FCk_TimerCategorizer::FormatMs(W.WallMs)));

        for (const auto& Top : W.TopWaits)
        {
            if (Top.ExclusiveMs < 0.5) break;

            Lines.Add(FString::Printf(TEXT("    %s *%s*  %s  `%s`"),
                *FCk_TimerCategorizer::SeverityIcon(Top.ExclusiveMs),
                *FCk_TimerCategorizer::FormatMs(Top.ExclusiveMs),
                *FCk_TimerCategorizer::FormatCount(Top.Count),
                *Top.Name));
        }
    }
    Lines.Add(TEXT(""));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    GenerateRawTimerList(const FCk_FrameAnalysisResult& Result,
                         const FTimerNameMap& TimerNames,
                         TArray<FString>& Lines) const
    -> void
{
    Lines.Add(FString::Printf(TEXT("*Top %d Game Thread Timers by Exclusive Time*\n"),
        _Config.RawTimerCount));

    TArray<TPair<uint32, double>> Sorted;
    for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
    {
        Sorted.Emplace(TimerIndex, ExclSec * 1000.0);
    }
    ck::algo::Sort(Sorted, [](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
    {
        return A.Value > B.Value;
    });

    const double FrameMs = Result.FrameDurationMs;
    const int32 Count = FMath::Min(Sorted.Num(), _Config.RawTimerCount);

    for (int32 i = 0; i < Count; ++i)
    {
        const uint32 TimerIndex = Sorted[i].Key;
        const double ExclMs = Sorted[i].Value;
        const double InclMs = Result.GetInclusiveMs(TimerIndex);
        const uint32 Cnt = Result.GetCount(TimerIndex);
        const double Pct = (FrameMs > 0.0) ? (ExclMs / FrameMs) * 100.0 : 0.0;

        FString Name = FCk_TimerCategorizer::SimplifyName(
            GetTimerName(TimerNames, TimerIndex));
        if (Name.Len() > 50)
        {
            Name = Name.Left(50) + TEXT("...");
        }

        const FString Icon = FCk_TimerCategorizer::SeverityIcon(ExclMs);

        Lines.Add(FString::Printf(TEXT("%3d. %s *%8s* excl  *%8s* incl  %7s  %4.1f%%  `%s`"),
            i + 1, *Icon,
            *FCk_TimerCategorizer::FormatMs(ExclMs),
            *FCk_TimerCategorizer::FormatMs(InclMs),
            *FCk_TimerCategorizer::FormatCount(Cnt),
            Pct, *Name));
    }
    Lines.Add(TEXT(""));
}

// --------------------------------------------------------------------------------------------------------------------

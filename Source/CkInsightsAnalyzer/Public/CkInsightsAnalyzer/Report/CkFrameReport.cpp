#include "CkInsightsAnalyzer/Report/CkFrameReport.h"
#include "CkInsightsAnalyzer/Core/CkTraceSession.h"
#include "CkInsightsAnalyzer_Log.h"

// --------------------------------------------------------------------------------------------------------------------
// Tree-drawing characters (Unicode box-drawing)
// --------------------------------------------------------------------------------------------------------------------

namespace CkTree
{
    static const FString Pipe  = TEXT("\u2502  ");    // │  (continuing branch)
    static const FString Tee   = TEXT("\u251C\u2500 "); // ├─ (sibling)
    static const FString Ell   = TEXT("\u2514\u2500 "); // └─ (last child)
    static const FString Space = TEXT("   ");           //    (after last child)
    static const FString HRule = TEXT("\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500");
}

// Frame wrapper timer names to skip when finding root timers
static const TSet<FString> FrameWrapperNames = {
    TEXT("FEngineLoop::Tick"),
    TEXT("FrameTime"),
    TEXT("Frame"),
    TEXT("BeginFrame"),
    TEXT("STAT_EventLoop_TEventLoop_RunOnce"),
    TEXT("FStats::AdvanceFrame"),
    TEXT("FRHIBreadcrumbEvent_GameThread_Begin"),
};

// --------------------------------------------------------------------------------------------------------------------
// Construction
// --------------------------------------------------------------------------------------------------------------------

FCk_FrameReport::FCk_FrameReport() = default;

FCk_FrameReport::FCk_FrameReport(const FCk_FrameReportConfig& Config)
    : _Config(Config)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Timer Name Resolution
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

// --------------------------------------------------------------------------------------------------------------------
// Main Generate
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    Generate(const FCk_TraceSession& Session,
             const FCk_FrameAnalysisResult& Result) const
    -> FString
{
    if (!Result.IsValid())
    {
        return TEXT("(No analysis data)");
    }

    TraceServices::FAnalysisSessionReadScope ReadScope = Session.CreateReadScope();
    const FTimerNameMap TimerNames = BuildTimerNameMap(Session);

    TArray<FString> Lines;
    Lines.Reserve(128);

    GenerateHeader(Result, Lines);
    GenerateHotPaths(Result, TimerNames, Lines);

    if (_Config.bShowCategorySummary)
    {
        GenerateCategorySummary(Result, TimerNames, Lines);
    }

    if (_Config.bShowWorkerThreads)
    {
        GenerateWorkerThreads(Session, Result, Lines);
    }

    if (_Config.bShowRawTimerList)
    {
        GenerateRawTimerList(Result, TimerNames, Lines);
    }

    return FString::Join(Lines, TEXT("\n"));
}

// --------------------------------------------------------------------------------------------------------------------
// Header
// --------------------------------------------------------------------------------------------------------------------

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
// Hot Path Tree
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    IsFrameWrapper(const FString& TimerName)
    -> bool
{
    return FrameWrapperNames.Contains(TimerName);
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

    const TMap<uint32, double>* Children = Result.ChildrenOf.Find(ParentTimerIndex);
    if (!Children) return Roots;

    for (const auto& [ChildIndex, ChildInclSec] : *Children)
    {
        const double ChildInclMs = ChildInclSec * 1000.0;
        if (ChildInclMs < 0.5)
        {
            continue;
        }

        const FString ChildName = GetTimerName(TimerNames, ChildIndex);

        if (IsFrameWrapper(ChildName) && Depth < 5)
        {
            // Drill deeper through wrapper
            TMap<uint32, double> Deeper = UnwrapRoots(ChildIndex, Result, TimerNames, Depth + 1);
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

    const TMap<uint32, double>* ChildMap = Result.ChildrenOf.Find(ParentTimerIndex);
    if (!ChildMap) return Children;

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

    Children.Sort([](const FChildInfo& A, const FChildInfo& B)
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

    for (int32 Iter = 0; Iter < 10; ++Iter) // max collapse depth
    {
        // If this timer does significant self-work, stop collapsing
        if (CurrentExcl > CurrentIncl * 0.05 && CurrentExcl > 0.3)
        {
            break;
        }

        TArray<FChildInfo> Children = GetSignificantChildren(CurrentIndex, Result, 0.5);
        if (Children.Num() == 0)
        {
            break;
        }

        const FChildInfo& TopChild = Children[0];

        // Cap child inclusive at parent inclusive to prevent inflation
        const double ChildIncl = FMath::Min(TopChild.InclusiveMs, CurrentIncl);

        // Only collapse when there's essentially one path through.
        // Use 20% threshold so branches with meaningful secondary paths are preserved.
        const bool bSinglePath =
            (Children.Num() == 1) ||
            (Children.Num() >= 2 && Children[1].InclusiveMs < CurrentIncl * 0.20);

        if (bSinglePath && ChildIncl > CurrentIncl * 0.7)
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
        const bool* bIsLast = IsLastAtDepth.Find(D);
        if (bIsLast && *bIsLast)
        {
            Prefix += CkTree::Space;
        }
        else
        {
            Prefix += CkTree::Pipe;
        }
    }

    const bool* bIsLast = IsLastAtDepth.Find(Depth);
    if (bIsLast && *bIsLast)
    {
        Prefix += CkTree::Ell;
    }
    else
    {
        Prefix += CkTree::Tee;
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

    // Collapse wrapper chain for this node
    FCollapsedTimer Collapsed = CollapseWrappers(
        TimerIndex, InclMs, ExclMs, Count, Result, TimerNames);

    // If already shown by a sibling's subtree, skip
    if (ShownTimers.Contains(Collapsed.TimerIndex))
    {
        return Lines;
    }
    ShownTimers.Add(Collapsed.TimerIndex);

    // Merge breadcrumbs from parent-level pre-collapse
    TArray<FString> AllBreadcrumbs;
    if (PreBreadcrumbs)
    {
        AllBreadcrumbs = *PreBreadcrumbs;
    }
    AllBreadcrumbs.Append(Collapsed.Breadcrumbs);

    // Build the display line
    const FString RawName = GetTimerName(TimerNames, Collapsed.TimerIndex);
    FString DisplayName = FCk_TimerCategorizer::SimplifyName(RawName);
    if (DisplayName.Len() > 50)
    {
        DisplayName = DisplayName.Left(50) + TEXT("...");
    }

    const FString Prefix = MakeTreePrefix(Depth, IsLastAtDepth);
    const FString Icon = FCk_TimerCategorizer::SeverityIcon(Collapsed.InclusiveMs);

    // Only show self-time when meaningfully different from inclusive
    const bool bShowSelf = Collapsed.ExclusiveMs > 0.3
                        && Collapsed.ExclusiveMs > Collapsed.InclusiveMs * 0.08;
    // Only show count when it suggests per-actor work
    const bool bShowCount = Collapsed.Count > 1;

    FString Line = FString::Printf(TEXT("%s%s `%s`  *%s*"),
        *Prefix, *Icon, *DisplayName,
        *FCk_TimerCategorizer::FormatMs(Collapsed.InclusiveMs));

    if (bShowSelf)
    {
        Line += FString::Printf(TEXT("  _%s self_"),
            *FCk_TimerCategorizer::FormatMs(Collapsed.ExclusiveMs));
    }
    if (bShowCount)
    {
        Line += FString::Printf(TEXT("  %s"),
            *FCk_TimerCategorizer::FormatCount(Collapsed.Count));
    }

    // Inline breadcrumb trail
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
            // Join with → arrow
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

    // Get children with adaptive threshold (3% of parent, min 0.3ms)
    const double MinChildMs = FMath::Max(0.3, Collapsed.InclusiveMs * 0.03);
    TArray<FChildInfo> Children = GetSignificantChildren(
        Collapsed.TimerIndex, Result, MinChildMs);

    // Pre-collapse each child and deduplicate by collapsed timer_id
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

        // Use global stats for display consistency
        const double GlobalIncl = FMath::Min(
            Result.GetInclusiveMs(CC.TimerIndex), Collapsed.InclusiveMs);
        const double GlobalExcl = Result.GetExclusiveMs(CC.TimerIndex);
        const uint32 GlobalCount = Result.GetCount(CC.TimerIndex);

        FDedupedChild* Existing = Seen.Find(CC.TimerIndex);
        if (!Existing || GlobalIncl > Existing->InclusiveMs)
        {
            Seen.FindOrAdd(CC.TimerIndex) = FDedupedChild{
                CC.TimerIndex, GlobalIncl, GlobalExcl, GlobalCount, MoveTemp(CC.Breadcrumbs)
            };
        }
    }

    // Sort deduped children by inclusive time, filter
    TArray<FDedupedChild> Deduped;
    for (auto& [Key, Val] : Seen)
    {
        Deduped.Add(MoveTemp(Val));
    }
    Deduped.Sort([](const FDedupedChild& A, const FDedupedChild& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    // Limit to 8 children, filter out self-references.
    // Don't filter by ShownTimers here — let each branch show its full subtree
    // even if the timer appeared in a sibling's subtree. The ShownTimers check
    // at the top of BuildTreeLines prevents infinite recursion.
    TArray<FDedupedChild> Visible;
    for (int32 i = 0; i < Deduped.Num() && Visible.Num() < 8; ++i)
    {
        if (Deduped[i].TimerIndex == Collapsed.TimerIndex) continue;
        Visible.Add(MoveTemp(Deduped[i]));
    }

    for (int32 i = 0; i < Visible.Num(); ++i)
    {
        TMap<int32, bool> ChildIsLast = IsLastAtDepth;
        ChildIsLast.Add(Depth + 1, i == Visible.Num() - 1);

        TArray<FString> ChildLines = BuildTreeLines(
            Visible[i].TimerIndex, Depth + 1,
            Visible[i].InclusiveMs, Visible[i].ExclusiveMs, Visible[i].Count,
            Result, TimerNames, ShownTimers, ChildIsLast,
            &Visible[i].Breadcrumbs);

        Lines.Append(MoveTemp(ChildLines));
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

    // Find root-level timers by unwrapping frame wrappers
    if (Result.FrameRootTimerIndex == static_cast<uint32>(INDEX_NONE))
    {
        Lines.Add(TEXT("(No frame root found)"));
        return;
    }

    TMap<uint32, double> RootChildren = UnwrapRoots(
        Result.FrameRootTimerIndex, Result, TimerNames);

    // Build sorted root list (above threshold)
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

    RootList.Sort([](const FRootEntry& A, const FRootEntry& B)
    {
        return A.InclusiveMs > B.InclusiveMs;
    });

    // Build tree lines for each root timer
    // Use per-root ShownTimers so each root tree can independently show its full call hierarchy.
    // A global set was too aggressive — timers appearing in one root's subtree would be hidden
    // from all subsequent roots, even when they represent different call paths.
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
        Lines.Add(TEXT("")); // blank line between root trees
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Category Summary
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    GenerateCategorySummary(const FCk_FrameAnalysisResult& Result,
                           const FTimerNameMap& TimerNames,
                           TArray<FString>& Lines) const
    -> void
{
    // Accumulate exclusive time per category
    TMap<FString, double> CategoryExclMs;

    for (const auto& [TimerIndex, ExclSeconds] : Result.TimerExclusive)
    {
        const double ExclMs = ExclSeconds * 1000.0;
        if (ExclMs < 0.01) continue;

        const FString TimerName = GetTimerName(TimerNames, TimerIndex);
        const FString Category = _Categorizer.Categorize(TimerName);

        double& Total = CategoryExclMs.FindOrAdd(Category, 0.0);
        Total += ExclMs;
    }

    // Sort: known categories by exclusive time, then "Other" at end
    struct FCatEntry
    {
        FString Name;
        double ExclMs;
        int32 Priority;
    };
    TArray<FCatEntry> SortedCats;

    for (const auto& [CatName, ExclMs] : CategoryExclMs)
    {
        if (ExclMs >= _Config.MinCategoryMs)
        {
            SortedCats.Add(FCatEntry{
                CatName, ExclMs, _Categorizer.GetCategoryPriority(CatName)
            });
        }
    }

    // Sort by exclusive time descending (matching Python behavior)
    SortedCats.Sort([](const FCatEntry& A, const FCatEntry& B)
    {
        return A.ExclMs > B.ExclMs;
    });

    if (SortedCats.Num() == 0) return;

    Lines.Add(FString::Printf(TEXT("\n%s"), *CkTree::HRule));
    Lines.Add(TEXT("*Category Summary (exclusive time)*\n"));

    const double FrameMs = Result.FrameDurationMs;
    for (const FCatEntry& Cat : SortedCats)
    {
        const double Pct = (FrameMs > 0.0) ? (Cat.ExclMs / FrameMs) * 100.0 : 0.0;
        const FString Icon = FCk_TimerCategorizer::SeverityIcon(Cat.ExclMs);
        const FString FormattedMs = FCk_TimerCategorizer::FormatMs(Cat.ExclMs);

        Lines.Add(FString::Printf(TEXT("%s *%8s*  %4.0f%%  %s"),
            *Icon, *FormattedMs, Pct, *Cat.Name));
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Worker Threads
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_FrameReport::
    GenerateWorkerThreads(const FCk_TraceSession& Session,
                          const FCk_FrameAnalysisResult& GameThreadResult,
                          TArray<FString>& Lines) const
    -> void
{
    const TArray<TraceServices::FThreadInfo> ThreadInfos = Session.GetThreadInfos();
    const uint32 GameThreadId = GameThreadResult.ThreadId;

    TArray<FCk_WorkerThreadSummary> Workers;

    for (const TraceServices::FThreadInfo& Info : ThreadInfos)
    {
        if (Info.Id == GameThreadId) continue;

        // Analyze this thread for the same time range as the game thread frame
        FCk_FrameAnalysisResult ThreadResult = FCk_FrameAnalyzer::AnalyzeTimeRange(
            Session, Info.Id,
            GameThreadResult.FrameStartTime, GameThreadResult.FrameEndTime);

        if (!ThreadResult.IsValid()) continue;

        // Compute wall time from depth-0 events
        double WallMs = 0.0;
        uint32 MinDepth = MAX_uint32;
        for (const FCk_TimingEvent& Evt : ThreadResult.Events)
        {
            MinDepth = FMath::Min(MinDepth, Evt.Depth);
        }
        for (const FCk_TimingEvent& Evt : ThreadResult.Events)
        {
            if (Evt.Depth == MinDepth)
            {
                WallMs += (Evt.EndTime - Evt.StartTime) * 1000.0;
            }
        }

        if (WallMs < _Config.MinWorkerThreadMs) continue;

        FCk_WorkerThreadSummary Summary;
        Summary.ThreadId = Info.Id;
        Summary.ThreadName = Info.Name ? FString(Info.Name) : FString::Printf(TEXT("Thread %u"), Info.Id);
        Summary.ThreadGroup = Info.GroupName ? FString(Info.GroupName) : FString();
        Summary.WallTimeMs = WallMs;
        Summary.EventCount = ThreadResult.Events.Num();

        // Build timer name map for this thread
        FTimerNameMap ThreadTimerNames = BuildTimerNameMap(Session);

        // Top 3 by exclusive time
        TArray<TPair<uint32, double>> ExclSorted;
        for (const auto& [TimerIndex, ExclSec] : ThreadResult.TimerExclusive)
        {
            ExclSorted.Emplace(TimerIndex, ExclSec * 1000.0);
        }
        ExclSorted.Sort([](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
        {
            return A.Value > B.Value;
        });

        for (int32 i = 0; i < FMath::Min(ExclSorted.Num(), 3); ++i)
        {
            const FString Name = GetTimerName(ThreadTimerNames, ExclSorted[i].Key);
            Summary.TopTimers.Add(FCk_WorkerThreadSummary::FTopTimer{
                FCk_TimerCategorizer::SimplifyName(Name),
                ExclSorted[i].Value,
                ThreadResult.GetCount(ExclSorted[i].Key)
            });
        }

        Workers.Add(MoveTemp(Summary));
    }

    // Sort by wall time descending
    Workers.Sort([](const FCk_WorkerThreadSummary& A, const FCk_WorkerThreadSummary& B)
    {
        return A.WallTimeMs > B.WallTimeMs;
    });

    if (Workers.Num() == 0) return;

    Lines.Add(FString::Printf(TEXT("\n%s"), *CkTree::HRule));
    Lines.Add(FString::Printf(TEXT("*Worker Threads (>%.0fms)*\n"), _Config.MinWorkerThreadMs));

    const int32 MaxWorkers = FMath::Min(Workers.Num(), _Config.MaxWorkerThreads);
    for (int32 i = 0; i < MaxWorkers; ++i)
    {
        const FCk_WorkerThreadSummary& W = Workers[i];

        FString Label = W.ThreadName;
        if (!W.ThreadGroup.IsEmpty())
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
// Raw Timer List
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

    // Sort by exclusive time
    TArray<TPair<uint32, double>> Sorted;
    for (const auto& [TimerIndex, ExclSec] : Result.TimerExclusive)
    {
        Sorted.Emplace(TimerIndex, ExclSec * 1000.0);
    }
    Sorted.Sort([](const TPair<uint32, double>& A, const TPair<uint32, double>& B)
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

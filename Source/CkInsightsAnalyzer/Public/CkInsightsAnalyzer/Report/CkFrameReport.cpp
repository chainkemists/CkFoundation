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
    GenerateHotPaths(const FCk_FrameAnalysisResult& Result,
                     const FTimerNameMap& TimerNames,
                     TArray<FString>& Lines) const
    -> void
{
    Lines.Add(TEXT("*Game Thread Hot Paths*\n"));
    const auto Roots = BuildEventHotPaths(Result, TimerNames);
    if (Roots.IsEmpty())
    {
        Lines.Add(TEXT("(No frame root found)"));
        return;
    }
    TFunction<void(const TSharedPtr<FCk_HotPathNode>&, int32, TMap<int32, bool>)> AppendNode;
    AppendNode = [&](const auto& Node, int32 Depth, TMap<int32, bool> LastAtDepth)
    {
        const auto Prefix = MakeTreePrefix(Depth, LastAtDepth);
        FString Line;
        if (Node->bIsAggregate)
        {
            Line = FString::Printf(TEXT("%s%s  *%s*"), *Prefix, *Node->DisplayName,
                *FCk_TimerCategorizer::FormatMs(Node->InclusiveMs));
        }
        else
        {
            const auto Name = Node->DisplayName.Len() > 50 ? Node->DisplayName.Left(50) + TEXT("...") : Node->DisplayName;
            Line = FString::Printf(TEXT("%s%s `%s`  *%s*"), *Prefix,
                *FCk_TimerCategorizer::SeverityIcon(Node->InclusiveMs), *Name,
                *FCk_TimerCategorizer::FormatMs(Node->InclusiveMs));
            if (Node->ExclusiveMs > 0.3 && Node->ExclusiveMs > Node->InclusiveMs * 0.08)
            { Line += FString::Printf(TEXT("  _%s self_"), *FCk_TimerCategorizer::FormatMs(Node->ExclusiveMs)); }
            if (Node->Count > 1)
            { Line += FString::Printf(TEXT("  %s"), *FCk_TimerCategorizer::FormatCount(Node->Count)); }
            TArray<FString> Breadcrumbs;
            for (int32 Index = FMath::Max(0, Node->Breadcrumbs.Num() - 2); Index < Node->Breadcrumbs.Num(); ++Index)
            {
                auto NamePart = FCk_TimerCategorizer::SimplifyName(Node->Breadcrumbs[Index]);
                if (NamePart.Len() <= 30) { Breadcrumbs.Add(MoveTemp(NamePart)); }
            }
            if (NOT Breadcrumbs.IsEmpty())
            { Line += TEXT("  _(") + FString::Join(Breadcrumbs, TEXT(" \u2192 ")) + TEXT(")_"); }
        }
        Lines.Add(MoveTemp(Line));
        for (int32 Index = 0; Index < Node->Children.Num(); ++Index)
        {
            auto ChildLast = LastAtDepth;
            ChildLast.Add(Depth + 1, Index == Node->Children.Num() - 1);
            AppendNode(Node->Children[Index], Depth + 1, MoveTemp(ChildLast));
        }
    };
    for (const auto& Root : Roots)
    {
        AppendNode(Root, 0, {});
        Lines.Add(TEXT(""));
    }
}


auto
    FCk_FrameReport::
    BuildEventHotPaths(const FCk_FrameAnalysisResult& Result, const FTimerNameMap& TimerNames) const
    -> TArray<TSharedPtr<FCk_HotPathNode>>
{
    // Timer-global ChildrenOf loses occurrence identity: A -> A counts A twice,
    // and an A below another parent can inflate this parent's row. Retain paths
    // only for the duration of report construction; frame accounting is unchanged.
    struct FPath
    {
        uint32 Timer = 0;
        double InclusiveMs = 0.0;
        double ExclusiveMs = 0.0;
        double CoveredEnd = TNumericLimits<double>::Lowest();
        uint32 Count = 0;
        TMap<uint32, int32> Children;
    };
    struct FOccurrence
    {
        int32 EventIndex;
        int32 PathIndex;
        double ChildrenSeconds = 0.0;
        double ChildrenEnd = TNumericLimits<double>::Lowest();
    };
    TArray<FPath> Paths;
    TArray<FOccurrence> Occurrences;
    TArray<int32> Stack;
    TMap<uint32, int32> RootPaths;
    Paths.Reserve(Result.Events.Num());
    Occurrences.Reserve(Result.Events.Num());
    Stack.Reserve(64);

    // AnalyzeEvents already clips and sorts by start/depth/end/timer. Use the
    // same containing-parent rule as its exclusive-time reduction, including
    // missing depth levels. Union direct-child intervals within each occurrence.
    for (int32 EventIndex = 0; EventIndex < Result.Events.Num(); ++EventIndex)
    {
        const auto& Event = Result.Events[EventIndex];
        while (Stack.Num() > 0 && Result.Events[Occurrences[Stack.Last()].EventIndex].EndTime <= Event.StartTime)
        { Stack.Pop(EAllowShrinking::No); }

        int32 ParentOccurrence = INDEX_NONE;
        for (int32 Index = Stack.Num() - 1; Index >= 0; --Index)
        {
            const auto& Parent = Result.Events[Occurrences[Stack[Index]].EventIndex];
            if (Parent.Depth < Event.Depth && Parent.EndTime >= Event.EndTime)
            { ParentOccurrence = Stack[Index]; break; }
        }

        const int32 ParentPath = ParentOccurrence == INDEX_NONE
            ? INDEX_NONE : Occurrences[ParentOccurrence].PathIndex;
        auto& Siblings = ParentPath == INDEX_NONE ? RootPaths : Paths[ParentPath].Children;
        const int32* Existing = Siblings.Find(Event.TimerIndex);
        int32 PathIndex = Existing != nullptr ? *Existing : INDEX_NONE;
        if (PathIndex == INDEX_NONE)
        {
            PathIndex = Paths.AddDefaulted();
            Paths[PathIndex].Timer = Event.TimerIndex;
            // Do not keep a reference into Paths across AddDefaulted.
            auto& UpdatedSiblings = ParentPath == INDEX_NONE ? RootPaths : Paths[ParentPath].Children;
            UpdatedSiblings.Add(Event.TimerIndex, PathIndex);
        }
        auto& Path = Paths[PathIndex];
        Path.InclusiveMs += FMath::Max(0.0, Event.EndTime - FMath::Max(Event.StartTime, Path.CoveredEnd)) * 1000.0;
        Path.CoveredEnd = FMath::Max(Path.CoveredEnd, Event.EndTime);
        ++Path.Count;

        if (ParentOccurrence != INDEX_NONE)
        {
            auto& Parent = Occurrences[ParentOccurrence];
            Parent.ChildrenSeconds += FMath::Max(0.0,
                Event.EndTime - FMath::Max(Event.StartTime, Parent.ChildrenEnd));
            Parent.ChildrenEnd = FMath::Max(Parent.ChildrenEnd, Event.EndTime);
        }
        Stack.Add(Occurrences.Add(FOccurrence{EventIndex, PathIndex}));
    }
    for (const auto& Occurrence : Occurrences)
    {
        const auto& Event = Result.Events[Occurrence.EventIndex];
        Paths[Occurrence.PathIndex].ExclusiveMs +=
            FMath::Max(0.0, Event.EndTime - Event.StartTime - Occurrence.ChildrenSeconds) * 1000.0;
    }

    TFunction<void(int32, int32)> MergePath = [&](int32 Into, int32 From)
    {
        Paths[Into].InclusiveMs += Paths[From].InclusiveMs;
        Paths[Into].ExclusiveMs += Paths[From].ExclusiveMs;
        Paths[Into].Count += Paths[From].Count;
        const auto Children = Paths[From].Children;
        for (const auto& Child : Children)
        {
            if (const auto* Existing = Paths[Into].Children.Find(Child.Key))
            { MergePath(*Existing, Child.Value); }
            else
            { Paths[Into].Children.Add(Child.Key, Child.Value); }
        }
    };
    // Suppress direct same-name recursion without suppressing its named work.
    // Inclusive is already the outer occurrence; add only its self and calls.
    for (int32 Index = Paths.Num() - 1; Index >= 0; --Index)
    {
        if (const auto* SameTimer = Paths[Index].Children.Find(Paths[Index].Timer))
        {
            const int32 ChildIndex = *SameTimer;
            Paths[Index].ExclusiveMs += Paths[ChildIndex].ExclusiveMs;
            Paths[Index].Count += Paths[ChildIndex].Count;
            Paths[Index].Children.Remove(Paths[Index].Timer);
            const auto Grandchildren = Paths[ChildIndex].Children;
            for (const auto& Child : Grandchildren)
            {
                if (const auto* Existing = Paths[Index].Children.Find(Child.Key))
                { MergePath(*Existing, Child.Value); }
                else
                { Paths[Index].Children.Add(Child.Key, Child.Value); }
            }
        }
    }

    const auto SortedChildren = [&Paths](int32 Index)
    {
        TArray<int32> Children;
        Paths[Index].Children.GenerateValueArray(Children);
        Children.Sort([&Paths](int32 A, int32 B)
        {
            if (Paths[A].InclusiveMs != Paths[B].InclusiveMs)
            { return Paths[A].InclusiveMs > Paths[B].InclusiveMs; }
            return Paths[A].Timer < Paths[B].Timer;
        });
        return Children;
    };
    TFunction<void(TSharedPtr<FCk_HotPathNode>&, const TSharedPtr<FCk_HotPathNode>&)> MergeNode;
    MergeNode = [&MergeNode](TSharedPtr<FCk_HotPathNode>& Into, const TSharedPtr<FCk_HotPathNode>& From)
    {
        Into->InclusiveMs += From->InclusiveMs;
        Into->ExclusiveMs += From->ExclusiveMs;
        Into->Count += From->Count;
        for (const auto& Child : From->Children)
        {
            auto* Existing = Into->Children.FindByPredicate([&Child](const auto& Other)
            { return Other->RawName == Child->RawName && Other->Breadcrumbs == Child->Breadcrumbs; });
            if (Existing != nullptr) { MergeNode(*Existing, Child); }
            else { Into->Children.Add(Child); }
        }
    };
    TFunction<TSharedPtr<FCk_HotPathNode>(int32, int32)> BuildNode;
    BuildNode = [&](int32 Index, int32 Depth)
    {
        auto Node = MakeShared<FCk_HotPathNode>();
        for (int32 Iteration = 0; Iteration < 10; ++Iteration)
        {
            const auto& Path = Paths[Index];
            const auto Name = GetTimerName(TimerNames, Path.Timer);
            if (Name.StartsWith(TEXT("script::")) ||
                (Path.ExclusiveMs > Path.InclusiveMs * 0.05 && Path.ExclusiveMs > 0.3))
            { break; }
            const auto Children = SortedChildren(Index);
            if (Children.IsEmpty() || Paths[Children[0]].InclusiveMs < 0.5)
            { break; }
            const bool SinglePath = Children.Num() == 1 ||
                Paths[Children[1]].InclusiveMs < Path.InclusiveMs * 0.20;
            if (NOT SinglePath || Paths[Children[0]].InclusiveMs <= Path.InclusiveMs * 0.7)
            { break; }
            Node->Breadcrumbs.Add(Name);
            Index = Children[0];
        }
        const auto& Path = Paths[Index];
        Node->RawName = GetTimerName(TimerNames, Path.Timer);
        Node->DisplayName = FCk_TimerCategorizer::SimplifyName(Node->RawName);
        Node->InclusiveMs = Path.InclusiveMs;
        Node->ExclusiveMs = FMath::Min(Path.ExclusiveMs, Path.InclusiveMs);
        Node->Count = Path.Count;
        if (Depth >= _Config.MaxTreeDepth) { return Node; }
        for (int32 ChildIndex : SortedChildren(Index))
        {
            auto Child = BuildNode(ChildIndex, Depth + 1);
            auto* Existing = Node->Children.FindByPredicate([&Child](const auto& Other)
            { return Other->RawName == Child->RawName && Other->Breadcrumbs == Child->Breadcrumbs; });
            if (Existing != nullptr) { MergeNode(*Existing, Child); }
            else { Node->Children.Add(MoveTemp(Child)); }
        }
        return Node;
    };

    TArray<TSharedPtr<FCk_HotPathNode>> Roots;
    TFunction<void(int32, int32)> AddRoot = [&](int32 Index, int32 Depth)
    {
        const auto Name = GetTimerName(TimerNames, Paths[Index].Timer);
        const bool Unnamed = Name.StartsWith(TEXT("UNKNOWN_"));
        if (Depth < 5 && (IsFrameWrapper(Name) || Unnamed) && NOT Paths[Index].Children.IsEmpty())
        {
            for (int32 Child : SortedChildren(Index)) { AddRoot(Child, Depth + 1); }
            return;
        }
        auto Node = BuildNode(Index, 0);
        auto* Existing = Roots.FindByPredicate([&Node](const auto& Other)
        { return Other->RawName == Node->RawName && Other->Breadcrumbs == Node->Breadcrumbs; });
        if (Existing != nullptr) { MergeNode(*Existing, Node); }
        else { Roots.Add(MoveTemp(Node)); }
    };
    for (const auto& Root : RootPaths) { AddRoot(Root.Value, 0); }
    // Apply the root floor only after identical displayed paths have combined;
    // two individually small disjoint calls can together be significant.
    Roots.RemoveAll([this](const auto& Node) { return Node->InclusiveMs < _Config.MinInclusiveMs; });
    const auto SortNodes = [](auto& Nodes)
    {
        Nodes.Sort([](const auto& A, const auto& B)
        {
            if (A->InclusiveMs != B->InclusiveMs) { return A->InclusiveMs > B->InclusiveMs; }
            return A->RawName < B->RawName;
        });
    };
    SortNodes(Roots);
    if (Roots.Num() > _Config.MaxRootTimers)
    { Roots.SetNum(FMath::Max(0, _Config.MaxRootTimers)); }

    TFunction<void(const TSharedPtr<FCk_HotPathNode>&)> PruneNode = [&](const auto& Node)
    {
        SortNodes(Node->Children);
        const auto AllChildren = MoveTemp(Node->Children);
        const double MinChild = FMath::Max(_Config.MinChildMs, Node->InclusiveMs * _Config.MinChildPctOfParent);
        const double HiddenBudget = FMath::Min(_Config.MinChildMs, Node->InclusiveMs * _Config.MinChildPctOfParent);
        double Remaining = 0.0;
        double Shown = 0.0;
        for (const auto& Child : AllChildren) { Remaining += Child->InclusiveMs; }
        for (const auto& Child : AllChildren)
        {
            if (_Config.ShowAllChildren || (Node->Children.Num() < _Config.MaxVisibleChildren &&
                (Child->InclusiveMs >= MinChild || Remaining > HiddenBudget)))
            {
                Remaining -= Child->InclusiveMs;
                Shown += Child->InclusiveMs;
                PruneNode(Child);
                Node->Children.Add(Child);
            }
        }
        const int32 HiddenCount = AllChildren.Num() - Node->Children.Num();
        const double HiddenMs = Node->InclusiveMs - Node->ExclusiveMs - Shown;
        if (NOT AllChildren.IsEmpty() && HiddenMs >= FMath::Max(0.1, Node->InclusiveMs * 0.02))
        {
            auto Hidden = MakeShared<FCk_HotPathNode>();
            Hidden->RawName = HiddenCount > 0
                ? FString::Printf(TEXT("(+%d below threshold)"), HiddenCount) : TEXT("(other children)");
            Hidden->DisplayName = Hidden->RawName;
            Hidden->InclusiveMs = HiddenMs;
            Hidden->Count = static_cast<uint32>(HiddenCount);
            Hidden->bIsAggregate = true;
            Node->Children.Add(MoveTemp(Hidden));
        }
    };
    for (const auto& Root : Roots) { PruneNode(Root); }
    return Roots;
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

    // Recovery must remain present when ensure diagnostics are compiled out.
    if (NOT ResultIsRealFrame || NOT Result.IsValid())
    { return {}; }
    return BuildEventHotPaths(Result, TimerNames);
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

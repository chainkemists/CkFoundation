#include "CkProcessorGraph.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    Build(
        const TArray<FProcessorDescriptor>& InDescriptors,
        const FCk_Registry& InRegistry,
        const FCk_Handle& InTransientEntity,
        ECk_UnresolvedRefPolicy InUnresolvedPolicy)
    -> FProcessorGraph
{
    const auto StartTime = FPlatformTime::Seconds();
    _UnresolvedPolicy = InUnresolvedPolicy;

    _Nodes.Reset();
    _NameToNodeIndex.Reset();
    _GroupStartIndices.Reset();
    _GroupEndIndices.Reset();
    _GroupNames.Reset();
    _ResolvedTickGroupCache.Reset();
    _ProcessorToGroupName.Reset();
    _ProcessorTickGroupMode.Reset();
    _ProcessorTickGroupValue.Reset();

    for (const auto& Descriptor : InDescriptors)
    {
        _ProcessorToGroupName.Add(Descriptor._Name, Descriptor._GroupName);
        _ProcessorTickGroupMode.Add(Descriptor._Name, Descriptor._TickGroupMode);
        _ProcessorTickGroupValue.Add(Descriptor._Name, Descriptor._TickGroupValue);
    }

    DoIdentifyGroups(InDescriptors);
    DoCreateNodes(InDescriptors, InTransientEntity);
    DoAddGroupEdges(InDescriptors);
    DoAddExplicitEdges(InDescriptors);
    DoResolveTickGroups(InDescriptors);
    DoValidate(InDescriptors);

    auto Partitions = DoPartitionAndSort();
    DoInstantiateProcessors(InRegistry, Partitions);

    _BuildTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

    ck::ecs::Verbose(TEXT("Processor graph built in [{:.3f}] ms. Nodes: [{}], Partitions: [{}]"),
        _BuildTimeMs, _Nodes.Num(), Partitions.Num());

    auto Graph = FProcessorGraph{};
    Graph._AllNodes = MoveTemp(_Nodes);
    Graph._Partitions = MoveTemp(Partitions);
    return Graph;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoIdentifyGroups(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> void
{
    for (const auto& Descriptor : InDescriptors)
    {
        if (Descriptor._GroupName != NAME_None)
        {
            _GroupNames.Add(Descriptor._GroupName);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoCreateNodes(
        const TArray<FProcessorDescriptor>& InDescriptors,
        const FCk_Handle& InTransientEntity)
    -> void
{
    for (const auto& Descriptor : InDescriptors)
    {
        const auto IsGhost = (Descriptor._NetModeRequirement == ECk_ProcessorNetMode::Override)
            and not ShouldCreateProcessorForNetMode(Descriptor._NetModeRequirementValue, InTransientEntity);

        const auto IsGroup = _GroupNames.Contains(Descriptor._Name);

        if (IsGroup)
        {
            auto StartNode = FProcessorGraphNode{};
            StartNode._Index = _Nodes.Num();
            StartNode._ProcessorName = Descriptor._Name;
            StartNode._HasDirtyMarker = Descriptor._HasDirtyMarker;
            StartNode._IsDirtyChecker = Descriptor._IsDirtyChecker;
            StartNode._Factory = Descriptor._Factory;
            StartNode._IsGroupStart = true;
            StartNode._IsGhost = IsGhost;

            if (Descriptor._TickGroupMode == ECk_TickGroupMode::Explicit)
            {
                StartNode._ResolvedTickGroup = Descriptor._TickGroupValue;
            }

            const auto StartIndex = StartNode._Index;
            _NameToNodeIndex.Add(Descriptor._Name, StartIndex);
            _GroupStartIndices.Add(Descriptor._Name, StartIndex);
            _Nodes.Emplace(MoveTemp(StartNode));

            auto EndNode = FProcessorGraphNode{};
            EndNode._Index = _Nodes.Num();
            EndNode._ProcessorName = FName{*FString::Printf(TEXT("%s.End"), *Descriptor._Name.ToString())};
            EndNode._IsGroupEnd = true;
            EndNode._IsGhost = IsGhost;
            EndNode._PairedGroupNodeIndex = StartIndex;

            const auto EndIndex = EndNode._Index;
            _GroupEndIndices.Add(Descriptor._Name, EndIndex);
            _Nodes.Emplace(MoveTemp(EndNode));

            _Nodes[StartIndex]._PairedGroupNodeIndex = EndIndex;
        }
        else
        {
            auto Node = FProcessorGraphNode{};
            Node._Index = _Nodes.Num();
            Node._ProcessorName = Descriptor._Name;
            Node._HasDirtyMarker = Descriptor._HasDirtyMarker;
            Node._IsDirtyChecker = Descriptor._IsDirtyChecker;
            Node._Factory = Descriptor._Factory;
            Node._IsGhost = IsGhost;

            if (Descriptor._TickGroupMode == ECk_TickGroupMode::Explicit)
            {
                Node._ResolvedTickGroup = Descriptor._TickGroupValue;
            }

            _NameToNodeIndex.Add(Descriptor._Name, Node._Index);
            _Nodes.Emplace(MoveTemp(Node));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoAddGroupEdges(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> void
{
    for (const auto& Descriptor : InDescriptors)
    {
        if (Descriptor._GroupName == NAME_None)
        { continue; }

        const auto GroupStartIndex = FindStartNodeIndex(Descriptor._GroupName);
        const auto GroupEndIndex = FindEndNodeIndex(Descriptor._GroupName);

        if (GroupStartIndex == INDEX_NONE || GroupEndIndex == INDEX_NONE)
        {
            CK_TRIGGER_ENSURE(
                TEXT("Processor [{}] references group [{}] which is not registered."),
                Descriptor._Name, Descriptor._GroupName);

            if (_UnresolvedPolicy == ECk_UnresolvedRefPolicy::Strict)
            { return; }

            continue;
        }

        const auto ChildIsGroup = _GroupNames.Contains(Descriptor._Name);

        if (ChildIsGroup)
        {
            const auto ChildStartIndex = FindStartNodeIndex(Descriptor._Name);
            const auto ChildEndIndex = FindEndNodeIndex(Descriptor._Name);

            if (ChildStartIndex == INDEX_NONE || ChildEndIndex == INDEX_NONE)
            { continue; }

            DoAddEdge(GroupStartIndex, ChildStartIndex);
            DoAddEdge(ChildEndIndex, GroupEndIndex);
        }
        else
        {
            const auto ChildIndex = FindNodeIndex(Descriptor._Name);
            if (ChildIndex == INDEX_NONE)
            { continue; }

            DoAddEdge(GroupStartIndex, ChildIndex);
            DoAddEdge(ChildIndex, GroupEndIndex);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoAddExplicitEdges(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> void
{
    for (const auto& Descriptor : InDescriptors)
    {
        const auto SourceIndex = FindNodeIndex(Descriptor._Name);
        if (SourceIndex == INDEX_NONE)
        { continue; }

        for (const auto& DepName : Descriptor._RunAfter)
        {
            // Ghost nodes are real nodes in the graph — FindNodeIndex returns a valid
            // index for them. Edges attach normally; ghosts preserve transitive ordering
            // without ever ticking.
            const auto DepIndex = FindNodeIndex(DepName);
            if (DepIndex == INDEX_NONE)
            {
                CK_TRIGGER_ENSURE(
                    TEXT("Processor [{}] declares RunAfter [{}] which is not registered."),
                    Descriptor._Name, DepName);

                if (_UnresolvedPolicy == ECk_UnresolvedRefPolicy::Strict)
                { return; }

                continue;
            }

            const auto IsDepGroup = _GroupEndIndices.Contains(DepName);
            const auto EffectiveDepIndex = IsDepGroup
                ? FindEndNodeIndex(DepName)
                : DepIndex;

            if (EffectiveDepIndex != INDEX_NONE)
            {
                DoAddEdge(EffectiveDepIndex, SourceIndex);
            }
        }

        for (const auto& DepName : Descriptor._RunBefore)
        {
            const auto DepIndex = FindNodeIndex(DepName);
            if (DepIndex == INDEX_NONE)
            {
                CK_TRIGGER_ENSURE(
                    TEXT("Processor [{}] declares RunBefore [{}] which is not registered."),
                    Descriptor._Name, DepName);

                if (_UnresolvedPolicy == ECk_UnresolvedRefPolicy::Strict)
                { return; }

                continue;
            }

            const auto IsDepGroup = _GroupStartIndices.Contains(DepName);
            const auto EffectiveDepIndex = IsDepGroup
                ? FindStartNodeIndex(DepName)
                : DepIndex;

            if (EffectiveDepIndex != INDEX_NONE)
            {
                DoAddEdge(SourceIndex, EffectiveDepIndex);
            }
        }

        DoAddTagBasedEdges(Descriptor, SourceIndex);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoAddTagBasedEdges(
        const FProcessorDescriptor& InDescriptor,
        int32 InSourceIndex)
    -> void
{
    if (NOT InDescriptor._RunAfterTags.IsValid() and not InDescriptor._RunBeforeTags.IsValid())
    { return; }

    for (const auto& Node : _Nodes)
    {
        if (NOT InDescriptor._RunAfterTags.IsEmpty())
        {
            for (const auto& Tag : InDescriptor._RunAfterTags)
            {
                const auto NodeTagStr = Node._ProcessorName.ToString();
                if (NodeTagStr.Contains(Tag.ToString()))
                {
                    const auto IsGroup = _GroupEndIndices.Contains(Node._ProcessorName);
                    const auto EffectiveIndex = IsGroup
                        ? FindEndNodeIndex(Node._ProcessorName)
                        : Node._Index;

                    if (EffectiveIndex != INDEX_NONE)
                    {
                        DoAddEdge(EffectiveIndex, InSourceIndex);
                    }
                }
            }
        }

        if (NOT InDescriptor._RunBeforeTags.IsEmpty())
        {
            for (const auto& Tag : InDescriptor._RunBeforeTags)
            {
                const auto NodeTagStr = Node._ProcessorName.ToString();
                if (NodeTagStr.Contains(Tag.ToString()))
                {
                    const auto IsGroup = _GroupStartIndices.Contains(Node._ProcessorName);
                    const auto EffectiveIndex = IsGroup
                        ? FindStartNodeIndex(Node._ProcessorName)
                        : Node._Index;

                    if (EffectiveIndex != INDEX_NONE)
                    {
                        DoAddEdge(InSourceIndex, EffectiveIndex);
                    }
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoAddEdge(
        int32 InFromIndex,
        int32 InToIndex)
    -> void
{
    if (InFromIndex == InToIndex)
    { return; }

    auto& FromNode = _Nodes[InFromIndex];
    auto& ToNode = _Nodes[InToIndex];

    if (NOT FromNode._OutEdges.Contains(InToIndex))
    {
        FromNode._OutEdges.Add(InToIndex);
    }

    if (NOT ToNode._InEdges.Contains(InFromIndex))
    {
        ToNode._InEdges.Add(InFromIndex);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoResolveTickGroups(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> void
{
    for (auto Index = 0; Index < _Nodes.Num(); ++Index)
    {
        _Nodes[Index]._ResolvedTickGroup = ResolveTickGroup(Index);
    }

    for (const auto& [GroupName, EndIndex] : _GroupEndIndices)
    {
        const auto* StartIndexPtr = _GroupStartIndices.Find(GroupName);
        if (StartIndexPtr != nullptr)
        {
            _Nodes[EndIndex]._ResolvedTickGroup = _Nodes[*StartIndexPtr]._ResolvedTickGroup;
        }
    }

    DoValidateCrossTickGroupChildren(InDescriptors);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    ResolveTickGroup(
        int32 InNodeIndex)
    -> ETickingGroup
{
    if (const auto* Cached = _ResolvedTickGroupCache.Find(InNodeIndex))
    {
        return *Cached;
    }

    const auto& Node = _Nodes[InNodeIndex];

    if (Node._IsGroupEnd)
    {
        const auto Result = ResolveTickGroup(Node._PairedGroupNodeIndex);
        _ResolvedTickGroupCache.Add(InNodeIndex, Result);
        return Result;
    }

    const auto ProcessorName = Node._ProcessorName;

    if (const auto* Mode = _ProcessorTickGroupMode.Find(ProcessorName))
    {
        if (*Mode == ECk_TickGroupMode::Explicit)
        {
            const auto Value = _ProcessorTickGroupValue.FindChecked(ProcessorName);
            _ResolvedTickGroupCache.Add(InNodeIndex, Value);
            return Value;
        }
    }

    if (const auto* GroupName = _ProcessorToGroupName.Find(ProcessorName))
    {
        if (*GroupName != NAME_None)
        {
            const auto GroupStartIndex = FindStartNodeIndex(*GroupName);
            if (GroupStartIndex != INDEX_NONE)
            {
                const auto Result = ResolveTickGroup(GroupStartIndex);
                _ResolvedTickGroupCache.Add(InNodeIndex, Result);
                return Result;
            }
        }
    }

    constexpr auto DefaultTickGroup = TG_PrePhysics;
    _ResolvedTickGroupCache.Add(InNodeIndex, DefaultTickGroup);
    return DefaultTickGroup;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoValidate(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> bool
{
    const auto HasCycle = DoDetectCycles();

    CK_ENSURE_IF_NOT(NOT HasCycle,
        TEXT("Cycle detected in processor dependency graph. See log for details."))
    { return false; }

    DoValidateSharedDirtyMarkers(InDescriptors);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoDetectCycles() const
    -> bool
{
    enum class EVisitState : uint8
    {
        Unvisited,
        InProgress,
        Done
    };

    auto States = TArray<EVisitState>{};
    States.SetNumZeroed(_Nodes.Num());

    auto CycleStack = TArray<int32>{};
    auto FoundCycle = false;

    const auto DFS = [&](auto& Self, int32 InIndex) -> void
    {
        if (FoundCycle)
        { return; }

        States[InIndex] = EVisitState::InProgress;
        CycleStack.Push(InIndex);

        for (const auto OutIndex : _Nodes[InIndex]._OutEdges)
        {
            if (States[OutIndex] == EVisitState::InProgress)
            {
                FoundCycle = true;

                auto CyclePath = FString{};
                auto InCycle = false;
                for (const auto StackIndex : CycleStack)
                {
                    if (StackIndex == OutIndex)
                    { InCycle = true; }

                    if (InCycle)
                    {
                        CyclePath += _Nodes[StackIndex]._ProcessorName.ToString();
                        CyclePath += TEXT(" -> ");
                    }
                }
                CyclePath += _Nodes[OutIndex]._ProcessorName.ToString();

                ck::ecs::Error(TEXT("Cycle detected in processor graph: [{}]"), CyclePath);
                return;
            }

            if (States[OutIndex] == EVisitState::Unvisited)
            {
                Self(Self, OutIndex);
            }
        }

        CycleStack.Pop();
        States[InIndex] = EVisitState::Done;
    };

    for (auto Index = 0; Index < _Nodes.Num(); ++Index)
    {
        if (States[Index] == EVisitState::Unvisited)
        {
            DFS(DFS, Index);
            if (FoundCycle)
            { break; }
        }
    }

    return FoundCycle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoValidateSharedDirtyMarkers(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> void
{
    // TODO: Detect when multiple processors share the same MarkedDirtyBy type
    //       but have no explicit RunAfter/RunBefore between them.
    //       Requires tracking the MarkedDirtyBy type identity in the descriptor.
    //       For now, this is a placeholder that will be implemented when
    //       we add type identity tracking to FProcessorDescriptor.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoValidateCrossTickGroupChildren(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> void
{
    for (const auto& Descriptor : InDescriptors)
    {
        if (Descriptor._GroupName == NAME_None)
        { continue; }

        const auto ChildIndex = FindNodeIndex(Descriptor._Name);
        const auto GroupStartIndex = FindStartNodeIndex(Descriptor._GroupName);

        if (ChildIndex == INDEX_NONE || GroupStartIndex == INDEX_NONE)
        { continue; }

        const auto ChildTickGroup = _Nodes[ChildIndex]._ResolvedTickGroup;
        const auto GroupTickGroup = _Nodes[GroupStartIndex]._ResolvedTickGroup;

        if (ChildTickGroup == GroupTickGroup)
        { continue; }

        if (ChildTickGroup < GroupTickGroup)
        {
            ck::ecs::Warning(
                TEXT("Processor [{}] is in tick group [{}] which is EARLIER than its group [{}] in tick group [{}]. ")
                TEXT("The child runs before the group starts — group membership has no effect."),
                Descriptor._Name, static_cast<int32>(ChildTickGroup),
                Descriptor._GroupName, static_cast<int32>(GroupTickGroup));
        }
        else
        {
            ck::ecs::Verbose(
                TEXT("Processor [{}] overrides tick group to [{}] (group [{}] is in [{}]). Cross-partition edges are implicit."),
                Descriptor._Name, static_cast<int32>(ChildTickGroup),
                Descriptor._GroupName, static_cast<int32>(GroupTickGroup));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoPartitionAndSort()
    -> TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>
{
    auto NodesByTickGroup = TMap<TEnumAsByte<ETickingGroup>, TArray<int32>>{};

    for (auto Index = 0; Index < _Nodes.Num(); ++Index)
    {
        NodesByTickGroup.FindOrAdd(_Nodes[Index]._ResolvedTickGroup).Add(Index);
    }

    auto Partitions = TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>{};

    for (auto& [TickGroup, OriginalIndices] : NodesByTickGroup)
    {
        auto OldToNewIndex = TMap<int32, int32>{};

        auto Partition = FProcessorGraphPartition{};

        for (auto OrigIdx = 0; OrigIdx < OriginalIndices.Num(); ++OrigIdx)
        {
            OldToNewIndex.Add(OriginalIndices[OrigIdx], OrigIdx);
        }

        for (const auto OriginalIndex : OriginalIndices)
        {
            auto Node = _Nodes[OriginalIndex];
            Node._Index = OldToNewIndex[OriginalIndex];

            auto RemappedInEdges = TArray<int32>{};
            for (const auto InEdge : Node._InEdges)
            {
                if (const auto* NewIdx = OldToNewIndex.Find(InEdge))
                {
                    RemappedInEdges.Add(*NewIdx);
                }
            }
            Node._InEdges = MoveTemp(RemappedInEdges);

            auto RemappedOutEdges = TArray<int32>{};
            for (const auto OutEdge : Node._OutEdges)
            {
                if (const auto* NewIdx = OldToNewIndex.Find(OutEdge))
                {
                    RemappedOutEdges.Add(*NewIdx);
                }
            }
            Node._OutEdges = MoveTemp(RemappedOutEdges);

            if (Node._PairedGroupNodeIndex != INDEX_NONE)
            {
                if (const auto* NewIdx = OldToNewIndex.Find(Node._PairedGroupNodeIndex))
                {
                    Node._PairedGroupNodeIndex = *NewIdx;
                }
                else
                {
                    Node._PairedGroupNodeIndex = INDEX_NONE;
                }
            }

            Partition._Nodes.Emplace(MoveTemp(Node));
        }

        Partition._ExecutionOrder = DoTopologicalSort(Partition._Nodes);

        Partitions.Add(TickGroup, MoveTemp(Partition));
    }

    return Partitions;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoTopologicalSort(
        const TArray<FProcessorGraphNode>& InNodes)
    -> TArray<int32>
{
    auto InDegree = TArray<int32>{};
    InDegree.SetNumZeroed(InNodes.Num());

    for (const auto& Node : InNodes)
    {
        for (const auto OutIndex : Node._OutEdges)
        {
            if (OutIndex >= 0 and OutIndex < InNodes.Num())
            {
                ++InDegree[OutIndex];
            }
        }
    }

    auto ReadyQueue = TArray<int32>{};

    for (auto Index = 0; Index < InNodes.Num(); ++Index)
    {
        if (InDegree[Index] == 0)
        {
            ReadyQueue.Add(Index);
        }
    }

    auto Result = TArray<int32>{};
    Result.Reserve(InNodes.Num());

    while (NOT ReadyQueue.IsEmpty())
    {
        ReadyQueue.Sort([&](const int32 A, const int32 B)
        {
            return InNodes[A]._ProcessorName.LexicalLess(InNodes[B]._ProcessorName);
        });

        const auto CurrentIndex = ReadyQueue[0];
        ReadyQueue.RemoveAt(0);
        Result.Add(CurrentIndex);

        for (const auto OutIndex : InNodes[CurrentIndex]._OutEdges)
        {
            if (OutIndex >= 0 and OutIndex < InNodes.Num())
            {
                --InDegree[OutIndex];
                if (InDegree[OutIndex] == 0)
                {
                    ReadyQueue.Add(OutIndex);
                }
            }
        }
    }

    CK_ENSURE_IF_NOT(Result.Num() == InNodes.Num(),
        TEXT("Topological sort produced [{}] nodes but expected [{}]. Possible cycle in partition."),
        Result.Num(), InNodes.Num())
    { /* cycle within partition — already detected earlier */ }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoInstantiateProcessors(
        const FCk_Registry& InRegistry,
        TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>& InPartitions)
    -> void
{
    for (auto& [TickGroup, Partition] : InPartitions)
    {
        for (auto& Node : Partition._Nodes)
        {
            if (Node._IsGhost)
            { continue; }

            if (Node._IsGroupEnd)
            { continue; }

            if (NOT Node._Factory)
            { continue; }

            Node._Instance = Node._Factory(InRegistry);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    FindNodeIndex(
        FName InProcessorName) const
    -> int32
{
    if (const auto* Found = _NameToNodeIndex.Find(InProcessorName))
    {
        return *Found;
    }
    return INDEX_NONE;
}

auto
    ck::FProcessorGraphBuilder::
    FindStartNodeIndex(
        FName InGroupName) const
    -> int32
{
    if (const auto* Found = _GroupStartIndices.Find(InGroupName))
    {
        return *Found;
    }
    return INDEX_NONE;
}

auto
    ck::FProcessorGraphBuilder::
    FindEndNodeIndex(
        FName InGroupName) const
    -> int32
{
    if (const auto* Found = _GroupEndIndices.Find(InGroupName))
    {
        return *Found;
    }
    return INDEX_NONE;
}

// --------------------------------------------------------------------------------------------------------------------

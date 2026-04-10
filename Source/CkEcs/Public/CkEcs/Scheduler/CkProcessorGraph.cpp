#include "CkProcessorGraph.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"

// --------------------------------------------------------------------------------------------------------------------
// Transitive closure / reduction helpers.
//
// These are lifted from entt/graph/flow.hpp (private member functions of basic_flow, not exposed standalone) and
// applied to an entt::adjacency_matrix directly. Both run in O(V^3); acceptable at graph-build time.
// Closure adds every (i,j) where i can reach j via any path. Reduction removes direct edges that are implied by
// longer paths, leaving the minimal edge set that preserves reachability.

namespace ck::detail
{
    using FDirectedAdjacencyMatrix = entt::adjacency_matrix<entt::directed_tag>;

    static auto
    DoTransitiveClosure(
        FDirectedAdjacencyMatrix& InMatrix) -> void
    {
        const auto Length = InMatrix.size();

        for (auto K = std::size_t{0}; K < Length; ++K)
        {
            for (auto I = std::size_t{0}; I < Length; ++I)
            {
                for (auto J = std::size_t{0}; J < Length; ++J)
                {
                    if (InMatrix.contains(I, K) and InMatrix.contains(K, J))
                    {
                        InMatrix.insert(I, J);
                    }
                }
            }
        }
    }

    static auto
    DoTransitiveReduction(
        FDirectedAdjacencyMatrix& InMatrix) -> void
    {
        const auto Length = InMatrix.size();

        // Drop self-loops first so they can't mask transitive edges.
        for (auto V = std::size_t{0}; V < Length; ++V)
        {
            InMatrix.erase(V, V);
        }

        for (auto J = std::size_t{0}; J < Length; ++J)
        {
            for (auto I = std::size_t{0}; I < Length; ++I)
            {
                if (InMatrix.contains(I, J))
                {
                    for (auto K = std::size_t{0}; K < Length; ++K)
                    {
                        if (InMatrix.contains(J, K))
                        {
                            InMatrix.erase(I, K);
                        }
                    }
                }
            }
        }
    }
}

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
    _WriteConflicts.Reset();

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

    if (not DoValidate(InDescriptors))
    {
        // Cycle detected — can't reduce a non-DAG. Bail out with an empty graph.
        return FProcessorGraph{};
    }

    if (not DoValidateAndResolveWriteConflicts(InDescriptors))
    {
        // Strict mode: write-write conflicts found and no explicit ordering to resolve them.
        return FProcessorGraph{};
    }

    // Reduction happens INSIDE DoPartitionAndSort, per-partition, after cross-partition edges
    // have been dropped. Reducing whole-graph first would incorrectly remove direct in-partition
    // edges whose transitive path routes through a node in another partition.
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
            StartNode._DirtyMarkerHash = Descriptor._DirtyMarkerHash;
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
            Node._DirtyMarkerHash = Descriptor._DirtyMarkerHash;
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

    if (not DoValidateSharedDirtyMarkers(InDescriptors))
    { return false; }

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
    -> bool
{
    // ---- Step 1: bucket processors by dirty marker hash ----
    // Descriptor iteration preserves declaration order for deterministic diagnostics.
    auto ProcessorsByMarkerHash = TMap<uint32, TArray<FName>>{};

    for (const auto& Descriptor : InDescriptors)
    {
        if (NOT Descriptor._HasDirtyMarker)
        { continue; }

        ProcessorsByMarkerHash.FindOrAdd(Descriptor._DirtyMarkerHash).Add(Descriptor._Name);
    }

    // Early out: nothing to check if no marker is shared by 2+ processors.
    auto HasShared = false;
    for (const auto& [MarkerHashUnused, Users] : ProcessorsByMarkerHash)
    {
        if (Users.Num() >= 2)
        {
            HasShared = true;
            break;
        }
    }

    if (NOT HasShared)
    { return true; }

    // ---- Step 2: whole-graph adjacency matrix + closure for O(1) path queries ----
    const auto NodeCount = static_cast<std::size_t>(_Nodes.Num());
    auto Closure = detail::FDirectedAdjacencyMatrix{NodeCount};

    for (auto From = 0; From < _Nodes.Num(); ++From)
    {
        for (const auto To : _Nodes[From]._OutEdges)
        {
            if (To >= 0 and To < _Nodes.Num())
            {
                Closure.insert(static_cast<std::size_t>(From), static_cast<std::size_t>(To));
            }
        }
    }

    detail::DoTransitiveClosure(Closure);

    // ---- Step 3: walk every pair that shares a marker ----
    auto AllOk = true;

    for (const auto& [MarkerHashUnused, Users] : ProcessorsByMarkerHash)
    {
        if (Users.Num() < 2)
        { continue; }

        for (auto I = 0; I < Users.Num(); ++I)
        {
            for (auto J = I + 1; J < Users.Num(); ++J)
            {
                const auto FirstIndex = FindNodeIndex(Users[I]);
                const auto SecondIndex = FindNodeIndex(Users[J]);

                if (FirstIndex == INDEX_NONE or SecondIndex == INDEX_NONE)
                { continue; }

                // Cross-tick-group pairs are implicitly ordered by the engine's ticking group
                // partitioning; the dirty pump pass runs independently inside each partition.
                if (_Nodes[FirstIndex]._ResolvedTickGroup != _Nodes[SecondIndex]._ResolvedTickGroup)
                { continue; }

                const auto FirstClosureIdx = static_cast<std::size_t>(FirstIndex);
                const auto SecondClosureIdx = static_cast<std::size_t>(SecondIndex);

                if (Closure.contains(FirstClosureIdx, SecondClosureIdx) or
                    Closure.contains(SecondClosureIdx, FirstClosureIdx))
                { continue; }

                // Two processors consume the same dirty signal with no explicit ordering.
                // Auto-inserting an edge would be arbitrary (no obvious "winner"), so this
                // is logged and — in Strict mode — aborts the build.
                if (_UnresolvedPolicy == ECk_UnresolvedRefPolicy::Strict)
                {
                    ck::ecs::Error(
                        TEXT("Dirty marker conflict: processors [{}] and [{}] both declare ")
                        TEXT("MarkedDirtyBy for the same fragment with no explicit RunAfter/RunBefore ")
                        TEXT("ordering between them. Add a TDepList dependency to make the ordering explicit."),
                        Users[I], Users[J]);
                    AllOk = false;
                }
                else
                {
                    ck::ecs::Warning(
                        TEXT("Dirty marker conflict: processors [{}] and [{}] both declare ")
                        TEXT("MarkedDirtyBy for the same fragment with no explicit RunAfter/RunBefore ")
                        TEXT("ordering between them. Execution order is not guaranteed — add a TDepList ")
                        TEXT("dependency to make it explicit."),
                        Users[I], Users[J]);
                }
            }
        }
    }

    return AllOk;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoValidateAndResolveWriteConflicts(
        const TArray<FProcessorDescriptor>& InDescriptors)
    -> bool
{
    // ---- Step 1: bucket processors by the fragment hashes they write ----
    // Declaration order is preserved by iterating InDescriptors; that order is used as the
    // tie-breaker when auto-inserting edges in Permissive mode. FragmentName is captured from
    // the first descriptor that references the hash (all references are the same type).
    struct FWritersForFragment
    {
        FName _FragmentName;
        TArray<FName> _WriterNames;
    };

    auto WritersByFragmentHash = TMap<uint32, FWritersForFragment>{};

    for (const auto& Descriptor : InDescriptors)
    {
        for (auto HashIdx = 0; HashIdx < Descriptor._RW_FragmentHashes.Num(); ++HashIdx)
        {
            const auto Hash = Descriptor._RW_FragmentHashes[HashIdx];
            const auto FragmentName = Descriptor._RW_FragmentNames.IsValidIndex(HashIdx)
                ? Descriptor._RW_FragmentNames[HashIdx]
                : NAME_None;

            auto& Bucket = WritersByFragmentHash.FindOrAdd(Hash);
            if (Bucket._FragmentName == NAME_None)
            {
                Bucket._FragmentName = FragmentName;
            }
            Bucket._WriterNames.Add(Descriptor._Name);
        }
    }

    // Early out: nothing to check if no fragment has multiple writers.
    auto HasAnyMultiWriter = false;
    for (const auto& [HashUnused, Bucket] : WritersByFragmentHash)
    {
        if (Bucket._WriterNames.Num() >= 2)
        {
            HasAnyMultiWriter = true;
            break;
        }
    }

    if (NOT HasAnyMultiWriter)
    { return true; }

    // ---- Step 2: build a whole-graph adjacency matrix + transitive closure ----
    // The closure gives O(1) reachability checks while iterating pairs. It is updated
    // incrementally when Permissive-mode edges are inserted so that follow-up pairs observe
    // the new ordering and avoid spurious conflicts.
    const auto NodeCount = static_cast<std::size_t>(_Nodes.Num());
    auto Closure = detail::FDirectedAdjacencyMatrix{NodeCount};

    for (auto From = 0; From < _Nodes.Num(); ++From)
    {
        for (const auto To : _Nodes[From]._OutEdges)
        {
            if (To >= 0 and To < _Nodes.Num())
            {
                Closure.insert(static_cast<std::size_t>(From), static_cast<std::size_t>(To));
            }
        }
    }

    detail::DoTransitiveClosure(Closure);

    // Incrementally updates Closure to reflect a newly inserted edge (InFromIndex → InToIndex).
    // After this update, any X that can reach InFromIndex (or is InFromIndex itself) gets edges to
    // everything InToIndex can reach (or InToIndex itself). O(V^2) per update.
    const auto UpdateClosureForNewEdge = [&](const std::size_t InFrom, const std::size_t InTo) -> void
    {
        for (auto X = std::size_t{0}; X < NodeCount; ++X)
        {
            const auto X_Reaches_From = (X == InFrom) or Closure.contains(X, InFrom);
            if (NOT X_Reaches_From)
            { continue; }

            for (auto Y = std::size_t{0}; Y < NodeCount; ++Y)
            {
                const auto To_Reaches_Y = (Y == InTo) or Closure.contains(InTo, Y);
                if (To_Reaches_Y)
                {
                    Closure.insert(X, Y);
                }
            }
        }
    };

    // ---- Step 3: walk every writer pair and check for unresolved ordering ----
    auto AllOk = true;

    for (const auto& [FragmentHashUnused, Bucket] : WritersByFragmentHash)
    {
        const auto& Writers = Bucket._WriterNames;
        const auto& FragmentName = Bucket._FragmentName;

        if (Writers.Num() < 2)
        { continue; }

        for (auto I = 0; I < Writers.Num(); ++I)
        {
            for (auto J = I + 1; J < Writers.Num(); ++J)
            {
                const auto FirstIndex = FindNodeIndex(Writers[I]);
                const auto SecondIndex = FindNodeIndex(Writers[J]);

                if (FirstIndex == INDEX_NONE or SecondIndex == INDEX_NONE)
                { continue; }

                // Cross-tick-group pairs are implicitly ordered by the engine's ticking group
                // partitioning; they cannot race because they run in distinct Unreal tick phases.
                if (_Nodes[FirstIndex]._ResolvedTickGroup != _Nodes[SecondIndex]._ResolvedTickGroup)
                { continue; }

                const auto FirstClosureIdx = static_cast<std::size_t>(FirstIndex);
                const auto SecondClosureIdx = static_cast<std::size_t>(SecondIndex);

                if (Closure.contains(FirstClosureIdx, SecondClosureIdx) or
                    Closure.contains(SecondClosureIdx, FirstClosureIdx))
                { continue; }

                // Unresolved write-write conflict.
                auto ConflictRecord = FCk_WriteConflictInfo{};
                ConflictRecord._First = Writers[I];
                ConflictRecord._Second = Writers[J];
                ConflictRecord._FragmentName = FragmentName;

                if (_UnresolvedPolicy == ECk_UnresolvedRefPolicy::Strict)
                {
                    ck::ecs::Error(
                        TEXT("Write conflict: processors [{}] and [{}] both write fragment [{}] ")
                        TEXT("with no explicit RunAfter/RunBefore ordering between them. ")
                        TEXT("Add a TDepList dependency in one of them to establish order."),
                        Writers[I], Writers[J], FragmentName);

                    ConflictRecord._AutoInserted = false;
                    _WriteConflicts.Add(MoveTemp(ConflictRecord));
                    AllOk = false;
                }
                else
                {
                    ck::ecs::Warning(
                        TEXT("Write conflict: processors [{}] and [{}] both write fragment [{}] ")
                        TEXT("with no explicit RunAfter/RunBefore ordering. ")
                        TEXT("Inserting implicit edge [{}] -> [{}] from descriptor declaration order. ")
                        TEXT("Consider adding an explicit TDepList to make the ordering intentional."),
                        Writers[I], Writers[J], FragmentName, Writers[I], Writers[J]);

                    DoAddEdge(FirstIndex, SecondIndex);
                    UpdateClosureForNewEdge(FirstClosureIdx, SecondClosureIdx);

                    ConflictRecord._AutoInserted = true;
                    _WriteConflicts.Add(MoveTemp(ConflictRecord));
                }
            }
        }
    }

    return AllOk;
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

namespace ck::detail
{
    static auto
    DoTickGroupLabel(
        ETickingGroup InTickGroup)
        -> FString
    {
        switch (InTickGroup)
        {
            case TG_PrePhysics:         return TEXT("PrePhysics");
            case TG_StartPhysics:       return TEXT("StartPhysics");
            case TG_DuringPhysics:      return TEXT("DuringPhysics");
            case TG_EndPhysics:         return TEXT("EndPhysics");
            case TG_PostPhysics:        return TEXT("PostPhysics");
            case TG_PostUpdateWork:     return TEXT("PostUpdateWork");
            case TG_LastDemotable:      return TEXT("LastDemotable");
            default:                    return FString::Printf(TEXT("TickGroup_%d"), static_cast<int32>(InTickGroup));
        }
    }

    // Graphviz ignores trailing quotes inside labels but not embedded ones, so escape any literal
    // double-quotes that might appear in processor names.
    static auto
    DoEscapeDotLabel(
        const FString& InLabel)
        -> FString
    {
        auto Result = InLabel;
        Result.ReplaceInline(TEXT("\""), TEXT("\\\""));
        return Result;
    }
}

auto
    ck::DoSerializeProcessorGraphToDot(
        const TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>& InPartitions)
    -> FString
{
    auto Output = FString{};
    Output.Reserve(4096);

    Output += TEXT("digraph CkEcsScheduler {\n");
    Output += TEXT("    rankdir=LR;\n");
    Output += TEXT("    node [fontname=\"Helvetica\", fontsize=10];\n");
    Output += TEXT("    edge [fontname=\"Helvetica\", fontsize=8];\n");
    Output += TEXT("\n");

    // Each partition gets a numeric prefix for node ids to avoid collisions between group-start/end
    // pseudo-nodes that share processor names across partitions.
    auto PartitionIndex = 0;

    for (const auto& [TickGroup, Partition] : InPartitions)
    {
        const auto GroupLabel = detail::DoTickGroupLabel(TickGroup);
        const auto ClusterPrefix = FString::Printf(TEXT("p%d_"), PartitionIndex);

        Output += FString::Printf(TEXT("    subgraph cluster_%s {\n"), *GroupLabel);
        Output += FString::Printf(TEXT("        label=\"%s\";\n"), *GroupLabel);
        Output += TEXT("        style=rounded;\n");
        Output += TEXT("        color=\"#cccccc\";\n");
        Output += TEXT("\n");

        for (auto NodeIdx = 0; NodeIdx < Partition._Nodes.Num(); ++NodeIdx)
        {
            const auto& Node = Partition._Nodes[NodeIdx];
            const auto NodeId = FString::Printf(TEXT("%sn%d"), *ClusterPrefix, NodeIdx);
            const auto Label = detail::DoEscapeDotLabel(Node._ProcessorName.ToString());

            // Shape and style convey role
            auto Shape = FString{TEXT("box")};
            auto Style = FString{TEXT("filled")};
            auto FillColor = FString{TEXT("#f0f0f0")};
            auto FontColor = FString{TEXT("#222222")};

            if (Node._IsGroupStart)
            {
                Shape = TEXT("house");
                FillColor = TEXT("#dce6f1");
            }
            else if (Node._IsGroupEnd)
            {
                Shape = TEXT("invhouse");
                FillColor = TEXT("#dce6f1");
            }
            else if (Node._HasDirtyMarker)
            {
                Style = TEXT("filled,bold");
                FillColor = TEXT("#fff2cc");
            }

            if (Node._IsGhost)
            {
                Style = Node._HasDirtyMarker ? TEXT("dashed,bold") : TEXT("dashed");
                FillColor = TEXT("#ffffff");
                FontColor = TEXT("#888888");
            }

            Output += FString::Printf(
                TEXT("        \"%s\" [label=\"%s\", shape=%s, style=\"%s\", fillcolor=\"%s\", fontcolor=\"%s\"];\n"),
                *NodeId, *Label, *Shape, *Style, *FillColor, *FontColor);
        }

        Output += TEXT("    }\n\n");
        ++PartitionIndex;
    }

    // Edges (emitted after all subgraphs so Graphviz can resolve the cluster node ids).
    PartitionIndex = 0;

    for (const auto& [TickGroupUnused, Partition] : InPartitions)
    {
        const auto ClusterPrefix = FString::Printf(TEXT("p%d_"), PartitionIndex);

        for (auto FromIdx = 0; FromIdx < Partition._Nodes.Num(); ++FromIdx)
        {
            const auto& FromNode = Partition._Nodes[FromIdx];
            const auto FromId = FString::Printf(TEXT("%sn%d"), *ClusterPrefix, FromIdx);

            for (const auto ToIdx : FromNode._OutEdges)
            {
                if (ToIdx < 0 or ToIdx >= Partition._Nodes.Num())
                { continue; }

                const auto ToId = FString::Printf(TEXT("%sn%d"), *ClusterPrefix, ToIdx);
                Output += FString::Printf(TEXT("    \"%s\" -> \"%s\";\n"), *FromId, *ToId);
            }
        }

        ++PartitionIndex;
    }

    Output += TEXT("}\n");
    return Output;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessorGraphBuilder::
    DoReducePartitionEdges(
        FProcessorGraphPartition& InOutPartition) const
    -> void
{
    // Partition indices are already local (0..N within the partition), so the matrix
    // is sized to the partition's node count and fed from the partition's _OutEdges.
    const auto PartitionNodeCount = static_cast<std::size_t>(InOutPartition._Nodes.Num());

    if (PartitionNodeCount == 0)
    { return; }

    auto WorkMatrix = detail::FDirectedAdjacencyMatrix{PartitionNodeCount};

    for (auto From = 0; From < InOutPartition._Nodes.Num(); ++From)
    {
        for (const auto To : InOutPartition._Nodes[From]._OutEdges)
        {
            if (To >= 0 and To < InOutPartition._Nodes.Num())
            {
                WorkMatrix.insert(static_cast<std::size_t>(From), static_cast<std::size_t>(To));
            }
        }
    }

    // Closure then reduction — matches entt::basic_flow::graph() semantics.
    detail::DoTransitiveClosure(WorkMatrix);
    detail::DoTransitiveReduction(WorkMatrix);

    // Write reduced edges back to the partition nodes.
    for (auto& Node : InOutPartition._Nodes)
    {
        Node._InEdges.Reset();
        Node._OutEdges.Reset();
    }

    for (auto From = std::size_t{0}; From < PartitionNodeCount; ++From)
    {
        for (const auto& Edge : WorkMatrix.out_edges(From))
        {
            // out_edges yields (From, To) pairs; Edge.first always equals From here.
            const auto FromIdx = static_cast<int32>(From);
            const auto ToIdx = static_cast<int32>(Edge.second);
            InOutPartition._Nodes[FromIdx]._OutEdges.Add(ToIdx);
            InOutPartition._Nodes[ToIdx]._InEdges.Add(FromIdx);
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

        // Collapse redundant edges within the partition before sorting. This happens AFTER
        // cross-partition edges have been dropped by the remap above, so the closure/reduction
        // operates only on reachable-in-partition paths.
        DoReducePartitionEdges(Partition);

        Partition._ExecutionOrder = DoTopologicalSort(Partition._Nodes);

        Partitions.Add(TickGroup, MoveTemp(Partition));
    }

    // Distribute the builder's transient write-conflict list into the matching partition.
    // All conflicts recorded by DoValidateAndResolveWriteConflicts are same-tick-group by
    // construction, so each record lands in exactly one partition.
    for (const auto& Conflict : _WriteConflicts)
    {
        const auto FirstGlobalIdx = FindNodeIndex(Conflict._First);
        if (FirstGlobalIdx == INDEX_NONE)
        { continue; }

        const auto ConflictTickGroup = _Nodes[FirstGlobalIdx]._ResolvedTickGroup;
        if (auto* Partition = Partitions.Find(ConflictTickGroup))
        {
            Partition->_WriteConflicts.Add(Conflict);
        }
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

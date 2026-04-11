#pragma once

#include "CkProcessorDescriptor.h"

#include "CkCore/Concepts/CkConcepts.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <entt/graph/adjacency_matrix.hpp>
#include <entt/graph/fwd.hpp>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorGraphNode
    {
        CK_GENERATED_BODY(FProcessorGraphNode);

        int32 _Index = INDEX_NONE;
        FName _ProcessorName;

        bool _HasDirtyMarker = false;
        FDirtyChecker _IsDirtyChecker;
        FProcessorFactory _Factory;

        // Hash of the MarkedDirtyBy fragment type (0 when _HasDirtyMarker is false).
        // Used by the scheduler's pump pass to query FCk_Registry::Get_DirtyMarkerVersion
        // without needing the original type.
        uint32 _DirtyMarkerHash = 0;
        FName _DirtyMarkerName;

        // Per-node cache of the last observed dirty marker version, updated by the pump pass.
        // Reset to 0 at the start of each scheduler Tick() so the first pass of the frame
        // always re-evaluates _IsDirtyChecker. Subsequent pump passes compare against this
        // value and skip the expensive Has_AnyEntityWith scan when nothing has changed.
        mutable uint64 _LastSeenDirtyVersion = 0;

        TOptional<concepts::FTickableType> _Instance;

        TArray<int32> _InEdges;
        TArray<int32> _OutEdges;

        ETickingGroup _ResolvedTickGroup = TG_PrePhysics;

        bool _IsGhost = false;
        bool _IsGroupStart = false;
        bool _IsGroupEnd = false;
        int32 _PairedGroupNodeIndex = INDEX_NONE;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorGraphPartition
    {
        CK_GENERATED_BODY(FProcessorGraphPartition);

        TArray<FProcessorGraphNode> _Nodes;
        TArray<int32> _ExecutionOrder;

        // Write-write conflicts detected during graph construction whose resolution lives inside this
        // partition's tick group. Populated after partitioning from the builder's transient conflict list.
        // Consumed by the scheduler debugger to surface error diagnostics in the Inspector panel.
        TArray<FCk_WriteConflictInfo> _WriteConflicts;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorGraph
    {
        CK_GENERATED_BODY(FProcessorGraph);

        TArray<FProcessorGraphNode> _AllNodes;
        TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition> _Partitions;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Serializes a set of partitioned processor graphs to Graphviz DOT syntax. Each tick group becomes
    // a labelled subgraph cluster; nodes encode their role via shape and style (ghost = dashed, dirty
    // marker = bold, group start/end = house/invhouse). Edges are drawn from every _OutEdges entry.
    //
    // This reflects the POST-reduction graph (after DoReducePartitionEdges), so redundant transitive
    // edges are already collapsed. Auto-inserted edges from R1.3's write-conflict resolution are
    // indistinguishable from explicit RunAfter/RunBefore edges in the output.
    CKECS_API auto
    DoSerializeProcessorGraphToDot(
        const TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>& InPartitions) -> FString;

    // ----------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessorGraphBuilder
    {
    public:
        CK_GENERATED_BODY(FProcessorGraphBuilder);

    public:
        auto Build(
            const TArray<FProcessorDescriptor>& InDescriptors,
            const FCk_Registry& InRegistry,
            const FCk_Handle& InTransientEntity,
            ECk_UnresolvedRefPolicy InUnresolvedPolicy = ECk_UnresolvedRefPolicy::Permissive) -> FProcessorGraph;

    private:
        auto DoIdentifyGroups(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        auto DoCreateNodes(
            const TArray<FProcessorDescriptor>& InDescriptors,
            const FCk_Handle& InTransientEntity) -> void;

        auto DoAddGroupEdges(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        auto DoAddExplicitEdges(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        auto DoAddTagBasedEdges(
            const FProcessorDescriptor& InDescriptor,
            int32 InSourceIndex) -> void;

        auto DoAddEdge(
            int32 InFromIndex,
            int32 InToIndex) -> void;

        auto DoResolveTickGroups(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        auto DoValidate(
            const TArray<FProcessorDescriptor>& InDescriptors) -> bool;

        auto DoDetectCycles() const -> bool;

        // Detect pairs of processors that share the same MarkedDirtyBy fragment without any
        // directed ordering between them. Cross-tick-group pairs are skipped (engine orders
        // them). Same-tick-group pairs with no path either direction are treated per _UnresolvedPolicy:
        //   - Strict     → log error and return false (Build bails with empty graph)
        //   - Permissive → log warning and return true
        // Unlike write-write conflicts, no auto-edge is inserted because there is no reasonable
        // default ordering for two processors that consume the same dirty signal.
        auto DoValidateSharedDirtyMarkers(
            const TArray<FProcessorDescriptor>& InDescriptors) -> bool;

        auto DoValidateCrossTickGroupChildren(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        // Detect pairs of processors that both write the same fragment without any explicit
        // RunAfter/RunBefore ordering between them. Cross-tick-group pairs are skipped because
        // the engine orders them implicitly via Unreal ticking groups. Same-tick-group pairs
        // with no directed path either direction are treated per _UnresolvedPolicy:
        //   - Strict     → log error and return false (Build bails with empty graph)
        //   - Permissive → log warning and auto-insert an edge in descriptor declaration order
        // Returns true on success, false only in Strict mode when conflicts were found.
        auto DoValidateAndResolveWriteConflicts(
            const TArray<FProcessorDescriptor>& InDescriptors) -> bool;

        // Apply transitive closure + reduction to a single partition's adjacency data.
        // Reduction is intentionally per-partition (not whole-graph) because cross-partition edges
        // are dropped during partitioning — reducing whole-graph first can remove direct in-partition
        // edges when the transitive path routes through a node in another partition.
        auto DoReducePartitionEdges(
            FProcessorGraphPartition& InOutPartition) const -> void;

        auto DoPartitionAndSort() -> TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>;

        auto DoTopologicalSort(
            const TArray<FProcessorGraphNode>& InNodes) -> TArray<int32>;

        auto DoInstantiateProcessors(
            const FCk_Registry& InRegistry,
            TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>& InPartitions) -> void;

    private:
        auto FindNodeIndex(FName InProcessorName) const -> int32;
        auto FindStartNodeIndex(FName InGroupName) const -> int32;
        auto FindEndNodeIndex(FName InGroupName) const -> int32;

        auto ResolveTickGroup(int32 InNodeIndex) -> ETickingGroup;

    private:
        TArray<FProcessorGraphNode> _Nodes;
        TMap<FName, int32> _NameToNodeIndex;
        TMap<FName, int32> _GroupStartIndices;
        TMap<FName, int32> _GroupEndIndices;
        TSet<FName> _GroupNames;

        TMap<FName, FName> _ProcessorToGroupName;
        TMap<FName, ECk_TickGroupMode> _ProcessorTickGroupMode;
        TMap<FName, ETickingGroup> _ProcessorTickGroupValue;

        TMap<int32, ETickingGroup> _ResolvedTickGroupCache;

        // Transient, per-Build() conflict collection. Populated during DoValidateAndResolveWriteConflicts
        // and distributed into partitions by DoPartitionAndSort. Cleared at the top of Build().
        TArray<FCk_WriteConflictInfo> _WriteConflicts;

        ECk_UnresolvedRefPolicy _UnresolvedPolicy = ECk_UnresolvedRefPolicy::Permissive;

        double _BuildTimeMs = 0.0;

    public:
        CK_PROPERTY_GET(_BuildTimeMs);
    };
}

// --------------------------------------------------------------------------------------------------------------------

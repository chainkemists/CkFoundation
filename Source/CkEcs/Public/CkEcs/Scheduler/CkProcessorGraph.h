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

    // Never equals a real version sum (per-hash counters start at 0 and only increment), so a node
    // carrying it always runs the real check on its next visit.
    inline constexpr uint64 kDirtyVersion_ForceEvaluate = MAX_uint64;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorGraphNode
    {
        CK_GENERATED_BODY(FProcessorGraphNode);

        int32 _Index = INDEX_NONE;
        FName _ProcessorName;
        FName _LocalSettleAfterGroupName;
        bool _IsLocalSettleTrigger = false;

        // Per-processor Insights CPU scope name, plus its lazily-created trace event spec id
        // (0 = not traced yet; created on first dispatch with the `cpu` trace channel enabled).
        FName _TraceName;
        uint32 _TraceSpecId = 0;

        bool _HasDirtyMarker = false;
        FDirtyChecker _IsDirtyChecker;
        FDirtyChecker _IsDirtyChecker_Consumable;
        FProcessorFactory _Factory;

        // Lets the pump pass query FCk_Registry::Get_DirtyMarkerVersion without the original types.
        // Index-aligned with _DirtyMarkerNames (diagnostics only).
        TArray<uint32> _DirtyMarkerHashes;
        TArray<FName> _DirtyMarkerNames;

        // PERSISTENT across frames: _IsDirtyChecker re-runs only once a registry mutation has bumped
        // one of this node's marker versions, which is what keeps Has_AnyEntityWith's tombstone
        // false-positives (an in_place_delete pool never reports empty() again after first use) from
        // re-pumping idle processors every frame. The sentinel forces one evaluation on a fresh
        // graph, including a post-snapshot rebuild whose loader writes bypass the version counters.
        mutable uint64 _LastSeenDirtyVersion = kDirtyVersion_ForceEvaluate;

        // Main-pass empty-view skip (mirrors FProcessorDescriptor — see ECk_ProcessorEmptyViewPolicy).
        // _LastSeenIncludeVersionSum caches the include types' summed mutation counters exactly as
        // _LastSeenDirtyVersion does above, with the same force-evaluate sentinel.
        bool _CanSkipWhenViewEmpty = false;
        FEmptyViewChecker _IsViewProvablyEmpty;
        TArray<uint32> _ViewIncludeHashes;
        TArray<FName> _ViewIncludeNames;
        mutable uint64 _LastSeenIncludeVersionSum = kDirtyVersion_ForceEvaluate;
        mutable bool _LastKnownViewProvablyEmpty = false;

        TOptional<concepts::FTickableType> _Instance;

        TArray<int32> _InEdges;
        TArray<int32> _OutEdges;

        ETickingGroup _ResolvedTickGroup = TG_PrePhysics;

        bool _IsGhost = false;
        bool _IsGroupStart = false;
        bool _IsGroupEnd = false;
        int32 _PairedGroupNodeIndex = INDEX_NONE;

        ECk_ProcessorPumpPolicy _PumpPolicy = ECk_ProcessorPumpPolicy::Default;
        ECk_ProcessorLoadPolicy _LoadPolicy = ECk_ProcessorLoadPolicy::GatedDuringLoad;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorGraphPartition
    {
        CK_GENERATED_BODY(FProcessorGraphPartition);

        TArray<FProcessorGraphNode> _Nodes;
        TArray<int32> _ExecutionOrder;

        // Consumed by the scheduler debugger to surface diagnostics in the Inspector panel.
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

    // Graphviz DOT: one labelled subgraph cluster per tick group; role encoded via shape and style
    // (ghost = dashed, dirty marker = bold, group start/end = house/invhouse).
    // Reflects the POST-reduction graph, and auto-inserted write-conflict edges are indistinguishable
    // from explicit RunAfter/RunBefore ones.
    CKECS_API auto
    DoSerializeProcessorGraphToDot(
        const TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>& InPartitions) -> FString;

    // Human-readable execution order, one section per tick group. Ghost nodes are marked "(ghost)"
    // and nodes with dirty markers "(dirty)".
    CKECS_API auto
    DoSerializeProcessorExecutionOrder(
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
            ECk_UnresolvedRefPolicy InUnresolvedPolicy = ECk_UnresolvedRefPolicy::Permissive,
            ECk_ProcessorWorldTypeContext InWorldTypeContext = ECk_ProcessorWorldTypeContext::Runtime) -> FProcessorGraph;

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

        // Same-tick-group pairs sharing a MarkedDirtyBy fragment with no path either direction:
        // Strict → error + false (Build bails with an empty graph); Permissive → warning + true.
        // No auto-edge, unlike write conflicts — there is no reasonable default ordering.
        auto DoValidateSharedDirtyMarkers(
            const TArray<FProcessorDescriptor>& InDescriptors) -> bool;

        auto DoValidateCrossTickGroupChildren(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        // Same-tick-group pairs writing the same fragment with no path either direction:
        // Strict → error + false (Build bails with an empty graph); Permissive → warning + an
        // auto-inserted edge in descriptor declaration order.
        auto DoValidateAndResolveWriteConflicts(
            const TArray<FProcessorDescriptor>& InDescriptors) -> bool;

        // Per-partition, never whole-graph: cross-partition edges are dropped during partitioning, so
        // a whole-graph reduction can remove a direct in-partition edge whose transitive path routes
        // through another partition's node.
        auto DoReducePartitionEdges(
            FProcessorGraphPartition& InOutPartition) const -> void;

        auto DoPartitionAndSort() -> TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition>;

        auto DoTopologicalSort(
            const TArray<FProcessorGraphNode>& InNodes) const -> TArray<int32>;

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

        // Transient per-Build(): cleared at the top of Build, distributed into partitions by
        // DoPartitionAndSort.
        TArray<FCk_WriteConflictInfo> _WriteConflicts;

        ECk_UnresolvedRefPolicy _UnresolvedPolicy = ECk_UnresolvedRefPolicy::Permissive;

        ECk_ProcessorWorldTypeContext _WorldTypeContext = ECk_ProcessorWorldTypeContext::Runtime;

        double _BuildTimeMs = 0.0;

    public:
        CK_PROPERTY_GET(_BuildTimeMs);
    };
}

// --------------------------------------------------------------------------------------------------------------------

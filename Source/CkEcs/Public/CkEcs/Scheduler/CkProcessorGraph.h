#pragma once

#include "CkProcessorDescriptor.h"

#include "CkCore/Concepts/CkConcepts.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

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
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKECS_API FProcessorGraph
    {
        CK_GENERATED_BODY(FProcessorGraph);

        TArray<FProcessorGraphNode> _AllNodes;
        TMap<TEnumAsByte<ETickingGroup>, FProcessorGraphPartition> _Partitions;
    };

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

        auto DoValidateSharedDirtyMarkers(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

        auto DoValidateCrossTickGroupChildren(
            const TArray<FProcessorDescriptor>& InDescriptors) -> void;

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

        ECk_UnresolvedRefPolicy _UnresolvedPolicy = ECk_UnresolvedRefPolicy::Permissive;

        double _BuildTimeMs = 0.0;

    public:
        CK_PROPERTY_GET(_BuildTimeMs);
    };
}

// --------------------------------------------------------------------------------------------------------------------

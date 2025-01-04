#include "CkFlowGraph_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_FlowGraph_Subsystem_UE::
    Request_AddFlowGraph(
        const FCk_Handle_FlowGraph& InEntity,
        UFlowAsset* InFlowGraph)
    -> void
{
    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(TryGet_FlowGraphAsset(InEntity)),
        TEXT("Entity [{}] is ALREADY associated with a FlowGraph [{}]"),
        InEntity, TryGet_FlowGraphAsset(InEntity))
    { return; }

    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(TryGet_FlowGraphEntity(InFlowGraph)),
        TEXT("FlowGraph [{}] is ALREADY associated with Entity [{}]"),
        InFlowGraph, TryGet_FlowGraphEntity(InFlowGraph))
    { return; }

    _EntityToFlowGraph.Add(InEntity, InFlowGraph);
    _FlowGraphToEntity.Add(InFlowGraph, InEntity);
}

auto
    UCk_FlowGraph_Subsystem_UE::
    Request_RemoveFlowGraph(
        const FCk_Handle_FlowGraph& InEntity)
    -> void
{
    const auto& FlowGraphAsset = TryGet_FlowGraphAsset(InEntity);

    CK_ENSURE_IF_NOT(ck::IsValid(FlowGraphAsset),
        TEXT("Entity [{}] is NOT associated with a FlowGraph"), InEntity)
    { return; }

    _EntityToFlowGraph.Remove(InEntity);
    _FlowGraphToEntity.Remove(FlowGraphAsset);
}

auto
    UCk_FlowGraph_Subsystem_UE::
    TryGet_FlowGraphAsset(
        const FCk_Handle_FlowGraph& InEntity) const
    -> UFlowAsset*
{
    const auto& FoundFlowGraphAsset = _EntityToFlowGraph.Find(InEntity);

    if (ck::Is_NOT_Valid(FoundFlowGraphAsset, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    return *FoundFlowGraphAsset;
}

auto
    UCk_FlowGraph_Subsystem_UE::
    TryGet_FlowGraphEntity(
        const UFlowAsset* InFlowGraph) const
    -> FCk_Handle_FlowGraph
{
    const auto* FoundFlowGraphEntity = _FlowGraphToEntity.Find(InFlowGraph);

    if (ck::Is_NOT_Valid(FoundFlowGraphEntity, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    return *FoundFlowGraphEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FlowGraph_Subsystem_UE::
    Get_Subsystem(
        const UWorld* InWorld)
    -> SubsystemType*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Unable to get the FlowGraph Subsystem. InWorld is [{}]"),
        InWorld)
    { return {}; }

    const auto Subsystem = InWorld->GetSubsystem<SubsystemType>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("World* is Valid but could NOT get a valid [{}]. Is this being called from another WorldSubsystem resulting in an ordering issue?"),
        ck::TypeToString<SubsystemType>)
    { return {}; }

    return Subsystem;
}

// --------------------------------------------------------------------------------------------------------------------
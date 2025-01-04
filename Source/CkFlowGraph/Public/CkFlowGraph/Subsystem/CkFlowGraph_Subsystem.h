#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkFlowGraph/CkFlowGraph_Fragment_Data.h"

#include "CkFlowGraph_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKFLOWGRAPH_API UCk_FlowGraph_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_FlowGraph_Subsystem_UE);

public:
    auto
    Request_AddFlowGraph(
        const FCk_Handle_FlowGraph& InEntity,
        UFlowAsset* InFlowGraph) -> void;

    auto
    Request_RemoveFlowGraph(
        const FCk_Handle_FlowGraph& InEntity) -> void;

    auto
    TryGet_FlowGraphAsset(
        const FCk_Handle_FlowGraph& InEntity) const -> UFlowAsset*;

    auto
    TryGet_FlowGraphEntity(
        const UFlowAsset* InFlowGraph) const-> FCk_Handle_FlowGraph;

private:
    UPROPERTY(Transient)
    TMap<FCk_Handle_FlowGraph, UFlowAsset*> _EntityToFlowGraph;

    UPROPERTY(Transient)
    TMap<UFlowAsset*, FCk_Handle_FlowGraph> _FlowGraphToEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKFLOWGRAPH_API UCk_Utils_FlowGraph_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_FlowGraph_Subsystem_UE;

public:
    static auto
    Get_Subsystem(
        const UWorld* InWorld) -> SubsystemType*;
};

// --------------------------------------------------------------------------------------------------------------------

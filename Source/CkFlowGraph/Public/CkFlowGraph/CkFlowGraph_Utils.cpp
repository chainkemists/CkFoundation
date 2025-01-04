#include "CkFlowGraph_Utils.h"

#include "CkFlowGraph/CkFlowGraph_Fragment.h"
#include "CkFlowGraph/CkFlowGraph_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FlowGraph_UE::
    Add(
        FCk_Handle& InFlowGraphOwnerEntity,
        const FCk_Fragment_FlowGraph_ParamsData& InParams)
    -> FCk_Handle_FlowGraph
{
    auto NewFlowGraphEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_FlowGraph>(InFlowGraphOwnerEntity);

    UCk_Utils_GameplayLabel_UE::Add(NewFlowGraphEntity, InParams.Get_FlowGraphName());

    NewFlowGraphEntity.Add<ck::FFragment_FlowGraph_Params>(InParams);
    NewFlowGraphEntity.Add<ck::FFragment_FlowGraph_Current>();

    RecordOfFlowGraphs_Utils::AddIfMissing(InFlowGraphOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfFlowGraphs_Utils::Request_Connect(InFlowGraphOwnerEntity, NewFlowGraphEntity);

    return NewFlowGraphEntity;
}

auto
    UCk_Utils_FlowGraph_UE::
    AddMultiple(
        FCk_Handle& InFlowGraphOwnerEntity,
        const FCk_Fragment_MultipleFlowGraph_ParamsData& InParams)
    -> TArray<FCk_Handle_FlowGraph>
{
    return ck::algo::Transform<TArray<FCk_Handle_FlowGraph>>(InParams.Get_FlowGraphParams(),
    [&](const FCk_Fragment_FlowGraph_ParamsData& InFlowGraphParams)
    {
        return Add(InFlowGraphOwnerEntity, InFlowGraphParams);
    });
}

auto
    UCk_Utils_FlowGraph_UE::
    Has_Any(
        const FCk_Handle& InFlowGraphOwnerEntity)
    -> bool
{
    return RecordOfFlowGraphs_Utils::Has(InFlowGraphOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_FlowGraph_UE, FCk_Handle_FlowGraph, ck::FFragment_FlowGraph_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FlowGraph_UE::
    TryGet_FlowGraph(
        const FCk_Handle& InFlowGraphOwnerEntity,
        FGameplayTag InFlowGraphGoalName)
    -> FCk_Handle_FlowGraph
{
    return RecordOfFlowGraphs_Utils::Get_ValidEntry_If(InFlowGraphOwnerEntity, ck::algo::MatchesGameplayLabelExact{InFlowGraphGoalName});
}

auto
    UCk_Utils_FlowGraph_UE::
    ForEach_FlowGraph(
        FCk_Handle& InFlowGraphOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_FlowGraph>
{
    auto FlowGraph = TArray<FCk_Handle_FlowGraph>{};

    ForEach_FlowGraph(InFlowGraphOwnerEntity, [&](FCk_Handle_FlowGraph& InFlowGraph)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InFlowGraph, InOptionalPayload); }
        else
        { FlowGraph.Emplace(InFlowGraph); }
    });

    return FlowGraph;
}

auto
    UCk_Utils_FlowGraph_UE::
    ForEach_FlowGraph(
        FCk_Handle& InFlowGraphOwnerEntity,
        const TFunction<void(FCk_Handle_FlowGraph&)>& InFunc)
    -> void
{
    RecordOfFlowGraphs_Utils::ForEach_ValidEntry
    (
        InFlowGraphOwnerEntity,
        [&](FCk_Handle_FlowGraph InFlowGraph)
        {
            InFunc(InFlowGraph);
        }
    );
}

auto
    UCk_Utils_FlowGraph_UE::
    Get_Status(
        const FCk_Handle_FlowGraph& InFlowGraphEntity)
    -> ECk_FlowGraph_Status
{
    return InFlowGraphEntity.Get<ck::FFragment_FlowGraph_Current>().Get_Status();
}

auto
    UCk_Utils_FlowGraph_UE::
    Request_Start(
        FCk_Handle_FlowGraph& InHandle,
        const FCk_Request_FlowGraph_Start& InRequest)
    -> FCk_Handle_FlowGraph
{
    InHandle.AddOrGet<ck::FFragment_FlowGraph_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_FlowGraph_UE::
    Request_Stop(
        FCk_Handle_FlowGraph& InHandle,
        const FCk_Request_FlowGraph_Stop& InRequest)
    -> FCk_Handle_FlowGraph
{
    InHandle.AddOrGet<ck::FFragment_FlowGraph_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

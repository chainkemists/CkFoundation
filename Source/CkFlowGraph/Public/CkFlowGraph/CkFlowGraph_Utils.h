#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkFlowGraph/CkFlowGraph_Fragment_Data.h"
#include "CkFlowGraph/CkFlowGraph_Fragment.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkFlowGraph_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKFLOWGRAPH_API UCk_Utils_FlowGraph_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FlowGraph_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_FlowGraph);

private:
    struct RecordOfFlowGraphs_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFlowGraphs> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][FlowGraph] Add New FlowGraph")
    static FCk_Handle_FlowGraph
    Add(
        UPARAM(ref) FCk_Handle& InFlowGraphOwnerEntity,
        const FCk_Fragment_FlowGraph_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] Add Multiple New FlowGraphs")
    static TArray<FCk_Handle_FlowGraph>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InFlowGraphOwnerEntity,
        const FCk_Fragment_MultipleFlowGraph_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InFlowGraphOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] Has Any FlowGraph")
    static bool
    Has_Any(
        const FCk_Handle& InFlowGraphOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|FlowGraph",
        DisplayName="[Ck][FlowGraph] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_FlowGraph
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|FlowGraph",
        DisplayName="[Ck][FlowGraph] Handle -> FlowGraph Handle",
        meta = (CompactNodeTitle = "<AsFlowGraph>", BlueprintAutocast))
    static FCk_Handle_FlowGraph
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] Try Get FlowGraph")
    static FCk_Handle_FlowGraph
    TryGet_FlowGraph(
        const FCk_Handle& InFlowGraphOwnerEntity,
        UPARAM(meta = (Categories = "FlowGraph")) FGameplayTag InFlowGraphGoalName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_FlowGraph>
    ForEach_FlowGraph(
        UPARAM(ref) FCk_Handle& InFlowGraphOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    static auto
    ForEach_FlowGraph(
        FCk_Handle& InFlowGraphOwnerEntity,
        const TFunction<void(FCk_Handle_FlowGraph&)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] Get Status")
    static ECk_FlowGraph_Status
    Get_Status(
        const FCk_Handle_FlowGraph& InFlowGraphEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] Request Start Flow")
    static FCk_Handle_FlowGraph
    Request_Start(
        UPARAM(ref) FCk_Handle_FlowGraph& InHandle,
        const FCk_Request_FlowGraph_Start& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FlowGraph",
              DisplayName="[Ck][FlowGraph] Request Stop Flow")
    static FCk_Handle_FlowGraph
    Request_Stop(
        UPARAM(ref) FCk_Handle_FlowGraph& InHandle,
        const FCk_Request_FlowGraph_Stop& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
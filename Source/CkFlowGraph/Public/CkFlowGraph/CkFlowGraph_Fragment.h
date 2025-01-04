#pragma once

#include "CkFlowGraph_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_FlowGraph_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_FlowGraph_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFLOWGRAPH_API FFragment_FlowGraph_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_FlowGraph_Params);

    public:
        using ParamsType = FCk_Fragment_FlowGraph_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_FlowGraph_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFLOWGRAPH_API FFragment_FlowGraph_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_FlowGraph_Current);

    public:
        friend class FProcessor_FlowGraph_HandleRequests;

    private:
        ECk_FlowGraph_Status _Status = ECk_FlowGraph_Status::NotRunning;

    public:
        CK_PROPERTY_GET(_Status);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFLOWGRAPH_API FFragment_FlowGraph_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_FlowGraph_Requests);

    public:
        friend class FProcessor_FlowGraph_HandleRequests;
        friend class UCk_Utils_FlowGraph_UE;

    public:
        using RequestType = std::variant<FCk_Request_FlowGraph_Start, FCk_Request_FlowGraph_Stop>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfFlowGraphs, FCk_Handle_FlowGraph);
}

// --------------------------------------------------------------------------------------------------------------------
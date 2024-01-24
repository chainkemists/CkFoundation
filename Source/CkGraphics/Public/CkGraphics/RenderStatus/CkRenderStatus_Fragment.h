#pragma once

#include "CkRenderStatus_Fragment_Data.h"
#include "CkCore/Time/CkTime.h"
#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RenderStatus_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_RenderStatus_Group_WorldStatic);
    CK_DEFINE_ECS_TAG(FTag_RenderStatus_Group_WorldDynamic);
    CK_DEFINE_ECS_TAG(FTag_RenderStatus_Group_Pawn);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRAPHICS_API FFragment_RenderStatus_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderStatus_Params);

    public:
        using ParamsType = FCk_Fragment_RenderStatus_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_RenderStatus_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRAPHICS_API FFragment_RenderStatus_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderStatus_Requests);

    public:
        friend class FProcessor_RenderStatus_HandleRequests;
        friend class UCk_Utils_RenderStatus_UE;

    public:
        using RequestType = FCk_Request_RenderStatus_QueryRenderedActors;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGRAPHICS_API,
        OnRenderedActorsQueried,
        FCk_Delegate_RenderStatus_OnRenderedActorsQueried_MC,
        FCk_RenderedActorsList,
        FInstancedStruct);
}

// --------------------------------------------------------------------------------------------------------------------

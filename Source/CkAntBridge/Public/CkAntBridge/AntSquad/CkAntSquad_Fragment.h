#pragma once

#include "CkAntSquad_Fragment_Data.h"

#include <AntHandle.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AntSquad_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AntAgent_Renderer_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    // TODO: Move into Agent specific file
    struct CKANTBRIDGE_API FFragment_AntAgent_Renderer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AntAgent_Renderer_Params);

    public:
        using ParamsType = FCk_Fragment_AntAgent_Renderer_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AntAgent_Renderer_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntAgent_Renderer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AntAgent_Renderer_Current);

    public:
        friend class FProcessor_AntAgent_Renderer_Setup;

    private:
        TWeakObjectPtr<UInstancedStaticMeshComponent> _IsmComponent;

    public:
        CK_PROPERTY_GET(_IsmComponent);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntSquad_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AntSquad_Params);

    public:
        using ParamsType = FCk_Fragment_AntSquad_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AntSquad_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntSquad_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AntSquad_Current);

    public:
        friend class FProcessor_AntSquad_HandleRequests;
        friend class UCk_Utils_AntSquad_UE;

    private:
        TArray<FAntHandle> _AntHandles;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntSquad_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AntSquad_Requests);

    public:
        using RequestType = std::variant<FCk_Request_AntSquad_AddAgent>;

        using RequestList = TArray<RequestType>;

    public:
        friend class FProcessor_AntSquad_HandleRequests;
        friend class UCk_Utils_AntSquad_UE;

    private:
        RequestList _Requests;
    };
}

// --------------------------------------------------------------------------------------------------------------------

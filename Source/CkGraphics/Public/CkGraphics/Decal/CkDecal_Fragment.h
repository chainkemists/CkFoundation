#pragma once

#include "CkDecal_Fragment_Data.h"
#include "CkCore/Time/CkTime.h"
#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Decal_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Decal_Group_Pawn);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRAPHICS_API FFragment_Decal_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Decal_Params);

    public:
        using ParamsType = FCk_Fragment_Decal_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Decal_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRAPHICS_API FFragment_Decal_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Decal_Current);

    private:
        TWeakObjectPtr<UDecalComponent> _DecalComponent;

    public:
        CK_PROPERTY_GET(_DecalComponent);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Decal_Current, _DecalComponent);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRAPHICS_API FFragment_Decal_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Decal_Requests);

    public:
        friend class FProcessor_Decal_HandleRequests;
        friend class UCk_Utils_Decal_UE;

    public:
        using RequestType = FCk_Request_Decal_Resize;
        using RequestList = TOptional<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------

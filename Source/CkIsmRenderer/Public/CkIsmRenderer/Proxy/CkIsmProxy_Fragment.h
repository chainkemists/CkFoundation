#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IsmProxy_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_Dynamic);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Params);

    public:
        using ParamsType = FCk_Fragment_IsmProxy_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_IsmProxy_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

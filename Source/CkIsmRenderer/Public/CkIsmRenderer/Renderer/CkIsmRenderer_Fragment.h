#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include <variant>
#include <Components/InstancedStaticMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IsmRenderer_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_IsmRenderer_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmRenderer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmRenderer_Params);

    public:
        using ParamsType = FCk_Fragment_IsmRenderer_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_IsmRenderer_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmRenderer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmRenderer_Current);

    public:
        friend class FProcessor_IsmRenderer_Setup;

    private:
        TWeakObjectPtr<UInstancedStaticMeshComponent> _IsmComponent_Static;
        TWeakObjectPtr<UInstancedStaticMeshComponent> _IsmComponent_Movable;

    public:
        CK_PROPERTY_GET(_IsmComponent_Static);
        CK_PROPERTY_GET(_IsmComponent_Movable);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_InstancedStaticMeshRenderer_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_InstancedStaticMeshRenderer_Requests);

    public:
        using RequestType = std::variant<FCk_Request_IsmRenderer_NewInstance>;

        using RequestList = TArray<RequestType>;

    public:
        friend class FProcessor_IsmRenderer_HandleRequests;
        friend class UCk_Utils_IsmRenderer_UE;

    private:
        RequestList _Requests;
    };

}

// --------------------------------------------------------------------------------------------------------------------

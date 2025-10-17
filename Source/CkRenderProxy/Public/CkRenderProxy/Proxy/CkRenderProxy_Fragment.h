#pragma once

#include "CkRenderProxy_Fragment_Data.h"
#include "PrimitiveSceneDesc.h"
#include "StaticMeshSceneProxyDesc.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <PrimitiveSceneProxy.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RenderProxy_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_RenderProxy_Setup;
    class FProcessor_RenderProxy_UpdateTransform;
    class FProcessor_RenderProxy_HandleRequests;
    class FProcessor_RenderProxy_EndPlay;

    CK_DEFINE_ECS_TAG(FTag_RenderProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_RenderProxy_Disabled);
    CK_DEFINE_ECS_TAG(FTag_RenderProxy_Movable);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_RenderProxy_Params = FCk_Fragment_RenderProxy_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRENDERPROXY_API FRenderProxyData
    {
    public:
        CK_GENERATED_BODY(FRenderProxyData);

    public:
        FCustomPrimitiveData CustomData;
        FStaticMeshSceneProxyDesc ProxyDesc;
        FPrimitiveSceneInfoData SceneInfoData;
        FPrimitiveSceneDesc SceneDesc;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRENDERPROXY_API FFragment_RenderProxy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderProxy_Current);

    public:
        friend class FProcessor_RenderProxy_Setup;
        friend class FProcessor_RenderProxy_UpdateTransform;
        friend class FProcessor_RenderProxy_HandleRequests;
        friend class FProcessor_RenderProxy_EndPlay;
        friend class UCk_Utils_RenderProxy_UE;

    private:
        TUniquePtr<FRenderProxyData> _Data;
        FPrimitiveSceneProxy* _Proxy = nullptr;
        FGuid _InstanceId;
        FBoxSphereBounds _CachedBounds;

    public:
        CK_PROPERTY_GET(_Data);
        CK_PROPERTY_GET(_Proxy);
        CK_PROPERTY_GET(_InstanceId);
        CK_PROPERTY_GET(_CachedBounds);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRENDERPROXY_API FFragment_RenderProxy_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderProxy_Requests);

        using RequestType = std::variant<FCk_Request_RenderProxy_EnableDisable>;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
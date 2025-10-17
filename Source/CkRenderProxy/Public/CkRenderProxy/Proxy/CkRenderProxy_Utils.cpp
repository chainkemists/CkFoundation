#include "CkRenderProxy_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkRenderProxy/Proxy/CkRenderProxy_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_RenderProxy_UE, FCk_Handle_RenderProxy,
    ck::FFragment_RenderProxy_Params, ck::FFragment_RenderProxy_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderProxy_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_RenderProxy_ParamsData& InParams)
    -> FCk_Handle_RenderProxy
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Mesh()),
        TEXT("RenderProxy mesh is invalid for Handle [{}]"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_RenderProxy_Params>(InParams);
    InHandle.Add<ck::FFragment_RenderProxy_Current>();
    InHandle.Add<ck::FTag_RenderProxy_NeedsSetup>();

    return Cast(InHandle);
}

auto
    UCk_Utils_RenderProxy_UE::
    Request_EnableDisable(
        FCk_Handle_RenderProxy& InHandle,
        const FCk_Request_RenderProxy_EnableDisable& InRequest)
    -> FCk_Handle_RenderProxy
{
    InHandle.AddOrGet<ck::FFragment_RenderProxy_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InHandle;
}

auto
    UCk_Utils_RenderProxy_UE::
    Get_Mobility(
        const FCk_Handle_RenderProxy& InHandle)
    -> ECk_Mobility
{
    return InHandle.Get<ck::FFragment_RenderProxy_Params>().Get_Mobility();
}

auto
    UCk_Utils_RenderProxy_UE::
    Get_Mesh(
        const FCk_Handle_RenderProxy& InHandle)
    -> UStaticMesh*
{
    return InHandle.Get<ck::FFragment_RenderProxy_Params>().Get_Mesh();
}

auto
    UCk_Utils_RenderProxy_UE::
    Get_Bounds(
        const FCk_Handle_RenderProxy& InHandle)
    -> FBoxSphereBounds
{
    return InHandle.Get<ck::FFragment_RenderProxy_Current>().Get_CachedBounds();
}

auto
    UCk_Utils_RenderProxy_UE::
    Get_IsEnabled(
        const FCk_Handle_RenderProxy& InHandle)
    -> bool
{
    return NOT InHandle.Has<ck::FTag_RenderProxy_Disabled>();
}

// --------------------------------------------------------------------------------------------------------------------
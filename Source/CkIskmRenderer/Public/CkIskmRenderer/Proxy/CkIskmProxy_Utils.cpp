#include "CkIskmRenderer/Proxy/CkIskmProxy_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"

auto
    UCk_Utils_IskmProxy_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IskmProxy_ParamsData& InParams)
    -> FCk_Handle_IskmProxy
{
    auto RendererHandle = InParams.Get_Renderer();
    CK_ENSURE_IF_NOT(ck::IsValid(RendererHandle),
        TEXT("IskmProxy::Add: params has invalid renderer for [{}]"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_IskmProxy_Params>(InParams);
    InHandle.Add<ck::FFragment_IskmProxy_Current>();
    InHandle.Add<ck::FFragment_IskmProxy_AnimState>();
    InHandle.Add<ck::FFragment_IskmProxy_PoseSource>();
    InHandle.Add<ck::FFragment_IskmProxy_CustomData>();
    InHandle.Add<ck::FFragment_IskmProxy_Requests>();
    InHandle.Add<ck::FTag_IskmProxy_NeedsSetup>();

    return Cast(InHandle);
}

auto
    UCk_Utils_IskmProxy_UE::
    Has(const FCk_Handle& InHandle) -> bool
{
    return InHandle.Has_All<
        ck::FFragment_IskmProxy_Params,
        ck::FFragment_IskmProxy_Current,
        ck::FFragment_IskmProxy_AnimState>();
}

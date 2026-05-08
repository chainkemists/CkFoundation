#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"

auto
    UCk_Utils_IskmRenderer_UE::
    Add(
        FCk_Handle& InHandle,
        UCk_IskmRenderer_Data* InRendererData)
    -> FCk_Handle_IskmRenderer
{
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData),
        TEXT("IskmRenderer::Add: invalid RendererData for [{}]"), InHandle)
    { return {}; }
    CK_ENSURE_IF_NOT(ck::IsValid(InRendererData->Get_AnimCollection()),
        TEXT("IskmRenderer::Add: RendererData [{}] has no AnimCollection (entity [{}])"), InRendererData, InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_IskmRenderer_Params>(InRendererData);
    InHandle.Add<ck::FFragment_IskmRenderer_Current>();
    InHandle.Add<ck::FTag_IskmRenderer_NeedsSetup>();

    return Cast(InHandle);
}

auto
    UCk_Utils_IskmRenderer_UE::
    Has(const FCk_Handle& InHandle) -> bool
{
    return InHandle.Has_All<ck::FFragment_IskmRenderer_Params, ck::FFragment_IskmRenderer_Current>();
}

auto
    UCk_Utils_IskmRenderer_UE::
    Get_RendererData(const FCk_Handle_IskmRenderer& InHandle)
    -> UCk_IskmRenderer_Data*
{
    if (NOT InHandle.IsValid()) { return nullptr; }
    return InHandle.Get<ck::FFragment_IskmRenderer_Params>().Get_RendererData().Get();
}

auto
    UCk_Utils_IskmRenderer_UE::
    Get_AnimCollection(const FCk_Handle_IskmRenderer& InHandle)
    -> UCk_IskmAnimCollection_Data*
{
    auto* Renderer = Get_RendererData(InHandle);
    return ck::IsValid(Renderer) ? Renderer->Get_AnimCollection() : nullptr;
}

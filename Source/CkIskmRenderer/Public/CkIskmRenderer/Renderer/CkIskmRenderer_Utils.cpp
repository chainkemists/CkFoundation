#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
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
    Create(
        FCk_Handle& InOwner,
        UCk_IskmRenderer_Data* InRendererData)
    -> FCk_Handle_IskmRenderer
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InRendererData);
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
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }
    return InHandle.Get<ck::FFragment_IskmRenderer_Params>().Get_RendererData().Get();
}

auto
    UCk_Utils_IskmRenderer_UE::
    Get_AnimCollection(const FCk_Handle_IskmRenderer& InHandle)
    -> UCk_IskmAnimCollection_Data*
{
    auto* Renderer = Get_RendererData(InHandle);
    return ck::IsValid(Renderer) ? Renderer->Get_AnimCollection().Get() : nullptr;
}

#include "CkIsmRenderer_Utils.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_IsmRenderer_UE, FCk_Handle_IsmRenderer,
    ck::FFragment_IsmRenderer_Current, ck::FFragment_IsmRenderer_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IsmRenderer_ParamsData& InParams)
    -> FCk_Handle_IsmRenderer
{
    // It's possible that later we want dependent Entities to be able to have an IsmRenderer and they simply
    // use whatever Actor is available in their chain. For now, we're not going to support that since we do
    // not fully understand the implications (if every Entity has its own ISM ActorComponent, then that's
    // an increase in draw-calls which diminishes the returns we get from ISMs).
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Handle [{}] does NOT have an OwningActor. An Actor is needed for InstancedStaticMesh Renderer"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_IsmRenderer_Params>(InParams);
    InHandle.Add<ck::FFragment_IsmRenderer_Current>();
    InHandle.Add<ck::FTag_IsmRenderer_NeedsSetup>();

    return Cast(InHandle);
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_RendererName(
        const FCk_Handle_IsmRenderer& InHandle)
    -> FGameplayTag
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Params>().Get_Params().Get_RendererName();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_MeshToRender(
        const FCk_Handle_IsmRenderer& InHandle)
    -> UStaticMesh*
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Params>().Get_Params().Get_Mesh();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_MeshShadowCasting(
        const FCk_Handle_IsmRenderer& InHandle)
    -> ECk_EnableDisable
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Params>().Get_Params().Get_CastShadows();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_MeshCollision(
        const FCk_Handle_IsmRenderer& InHandle)
    -> ECk_Collision
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Params>().Get_Params().Get_Collision();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_MeshNumCustomData(
        const FCk_Handle_IsmRenderer& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Params>().Get_Params().Get_NumCustomData();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_CurrentInstanceCount_Static(
        const FCk_Handle_IsmRenderer& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Current>().Get_IsmComponent_Static()->GetInstanceCount();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_CurrentInstanceCount_Movable(
        const FCk_Handle_IsmRenderer& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_IsmRenderer_Current>().Get_IsmComponent_Movable()->GetInstanceCount();
}

auto
    UCk_Utils_IsmRenderer_UE::
    Get_CurrentInstanceCount_Total(
        const FCk_Handle_IsmRenderer& InHandle)
    -> int32
{
    return Get_CurrentInstanceCount_Movable(InHandle) + Get_CurrentInstanceCount_Static(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

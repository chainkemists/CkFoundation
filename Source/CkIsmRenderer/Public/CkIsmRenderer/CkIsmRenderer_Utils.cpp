#include "CkIsmRenderer_Utils.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkIsmRenderer/CkIsmRenderer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AntAgent_Renderer_UE, FCk_Handle_IsmRenderer,
    ck::FFragment_IsmRenderer_Current, ck::FFragment_IsmRenderer_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntAgent_Renderer_UE::
    Request_RenderInstance(
        FCk_Handle_IsmRenderer& InHandle,
        const FCk_Request_IsmRenderer_NewInstance& InRequest)
    -> FCk_Handle_IsmRenderer
{
        InHandle.AddOrGet<ck::FFragment_InstancedStaticMeshRenderer_Requests>()._Requests.Emplace(InRequest);
        return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntAgent_Renderer_UE::
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmProxy_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams)
    -> FCk_Handle_IsmProxy
{
    InHandle.Add<ck::FFragment_IsmProxy_Params>(InParams);
    InHandle.Add<ck::FTag_IsmProxy_NeedsSetup>();

    if (InParams.Get_Mobility() == ECk_Mobility::Movable)
    { InHandle.Add<ck::FTag_IsmProxy_Dynamic>(); }

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_IsmProxy_UE, FCk_Handle_IsmProxy, ck::FFragment_IsmProxy_Params)

// --------------------------------------------------------------------------------------------------------------------

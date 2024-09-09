#include "CkAntSquad_Utils.h"

#include "CkAntBridge_Log.h"
#include "CkAntSquad_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AntSquad_UE, FCk_Handle_AntSquad, ck::FFragment_AntSquad_Current, ck::FFragment_AntSquad_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntSquad_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AntSquad_ParamsData& InParams)
    -> FCk_Handle_AntSquad
{
    InHandle.Add<ck::FFragment_AntSquad_Params>(InParams);
    InHandle.Add<ck::FFragment_AntSquad_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntSquad_UE::
    Request_AddAgent(
        FCk_Handle_AntSquad& InAntSquadHandle,
        const FCk_Request_AntSquad_AddAgent& InRequest)
    -> FCk_Handle_AntSquad
{
    InAntSquadHandle.AddOrGet<ck::FFragment_AntSquad_Requests>()._Requests.Emplace(InRequest);
    return InAntSquadHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AntAgent_Renderer_UE, FCk_Handle_AntAgent_Renderer, ck::FFragment_AntAgent_Renderer_Current, ck::FFragment_AntAgent_Renderer_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntAgent_Renderer_UE::
    Request_RenderInstance(
        FCk_Handle_AntAgent_Renderer& InHandle,
        const FCk_Request_InstancedStaticMeshRenderer_NewInstance& InRequest)
    -> FCk_Handle_AntAgent_Renderer
{
        InHandle.AddOrGet<ck::FFragment_InstancedStaticMeshRenderer_Requests>()._Requests.Emplace(InRequest);
        return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntAgent_Renderer_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AntAgent_Renderer_ParamsData& InParams)
    -> FCk_Handle_AntAgent_Renderer
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Handle [{}] does NOT have an OwningActor. An Actor is needed by an AntAgent Renderer"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_AntAgent_Renderer_Params>(InParams);
    InHandle.Add<ck::FFragment_AntAgent_Renderer_Current>();
    InHandle.Add<ck::FTag_AntAgent_Renderer_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

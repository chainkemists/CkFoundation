#include "CkIsmProxy_Utils.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_IsmProxy_UE, FCk_Handle_IsmProxy, ck::FFragment_IsmProxy_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmProxy_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams)
    -> FCk_Handle_IsmProxy
{
    InHandle.Add<ck::FFragment_IsmProxy_Params>(InParams);
    InHandle.Add<ck::FFragment_IsmProxy_Current>();
    InHandle.Add<ck::FTag_IsmProxy_NeedsSetup>();

    return Cast(InHandle);
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_SetCustomData(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomData& InRequest)
    -> FCk_Handle_IsmProxy
{
    InHandle.AddOrGet<ck::FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_SetCustomDataValue(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomDataValue& InRequest)
    -> FCk_Handle_IsmProxy
{
    InHandle.AddOrGet<ck::FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_EnableDisable(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_EnableDisable& InRequest)
    -> FCk_Handle_IsmProxy
{
    InHandle.AddOrGet<ck::FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_LocalLocationOffset(
        const FCk_Handle_IsmProxy& InHandle)
    -> FVector
{
    return InHandle.Get<ck::FFragment_IsmProxy_Params>().Get_LocalLocationOffset();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_LocalRotationOffset(
        const FCk_Handle_IsmProxy& InHandle)
    -> FRotator
{
    return InHandle.Get<ck::FFragment_IsmProxy_Params>().Get_LocalRotationOffset();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_ScaleMultiplier(
        const FCk_Handle_IsmProxy& InHandle)
    -> FVector
{
    return InHandle.Get<ck::FFragment_IsmProxy_Params>().Get_ScaleMultiplier();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_Mobility(
        const FCk_Handle_IsmProxy& InHandle)
    -> ECk_Mobility
{
    const auto IsmRenderer = InHandle.Get<ck::FFragment_IsmProxy_Params>().Get_IsmRenderer();

    CK_ENSURE_IF_NOT(ck::IsValid(IsmRenderer), TEXT("The Ism Renderer [{}] is INVALID for Proxy Handle [{}]"),
        IsmRenderer, InHandle)
    { return {}; }

    return InHandle.Get<ck::FFragment_IsmProxy_Params>().Get_IsmRenderer()->Get_Mobility();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_MeshBounds(
        const FCk_Handle_IsmProxy& InHandle,
        ECk_ScaledUnscaled InScaling)
    -> FBoxSphereBounds
{
    const auto& Params = InHandle.Get<ck::FFragment_IsmProxy_Params>();
    const auto& ScaleMultiplier = Params.Get_ScaleMultiplier();
    const auto& IsmRenderer = Params.Get_IsmRenderer().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(IsmRenderer), TEXT("The Ism Renderer [{}] is INVALID for Proxy Handle [{}]"),
        IsmRenderer, InHandle)
    { return {}; }

    const auto& Mesh = IsmRenderer->Get_Mesh();

    CK_ENSURE_IF_NOT(ck::IsValid(Mesh), TEXT("The Ism Mesh [{}] is INVALID for Proxy Handle [{}]"),
        Mesh, InHandle)
    { return {}; }

    const auto& UnscaledMeshBounds = Mesh->GetBounds();

    if (ScaleMultiplier.Equals(FVector::OneVector))
    { return UnscaledMeshBounds; }

    switch (InScaling)
    {
        case ECk_ScaledUnscaled::Scaled:
        {
            auto ScaledBox = UnscaledMeshBounds.GetBox();
            ScaledBox.Min *= ScaleMultiplier;
            ScaledBox.Max *= ScaleMultiplier;

            const auto ScaledRadius = UnscaledMeshBounds.SphereRadius * ScaleMultiplier.GetMax();

            return FBoxSphereBounds(ScaledBox.GetCenter(), ScaledBox.GetExtent(), ScaledRadius);
        }
        case ECk_ScaledUnscaled::Unscaled:
        {
            return UnscaledMeshBounds;
        }
        default:
        {
            CK_INVALID_ENUM(InScaling);
            return UnscaledMeshBounds;
        }
    }
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_NeedsInstanceAdded(
        FCk_Handle_IsmProxy& InHandle)
    -> void
{
    InHandle.AddOrGet<ck::FTag_IsmProxy_NeedsInstanceAdded>();
}

// --------------------------------------------------------------------------------------------------------------------

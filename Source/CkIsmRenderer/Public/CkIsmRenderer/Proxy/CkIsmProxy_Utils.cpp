#include "CkIsmProxy_Utils.h"

#include "CkCore/Mesh/CkMesh_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_IsmProxy_UE, FCk_Handle_IsmProxy, ck::FFragment_IsmProxy_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmProxy_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams)
    -> FCk_Handle_IsmProxy
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_IsmRenderer()),
        TEXT("IsmRenderer is INVALID. Unable to add IsmProxy to Handle [{}]"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_IsmProxy_Params>(InParams);
    InHandle.Add<ck::FFragment_IsmProxy_Current>();
    InHandle.Add<ck::FTag_IsmProxy_NeedsSetup>();

    if (InParams.Get_StartingState() == ECk_EnableDisable::Disable)
    {
        InHandle.AddOrGet<ck::FTag_IsmProxy_Disabled>();
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_IsmProxy_UE::
    Create(
        FCk_Handle& InOwner,
        const FTransform& InInitialTransform,
        const FCk_Fragment_IsmProxy_ParamsData& InParams)
    -> FCk_Handle_IsmProxy
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    auto ChildTransform = UCk_Utils_Transform_UE::Add(NewEntity, InInitialTransform, ECk_Replication::DoesNotReplicate);
    return Add(ChildTransform, InParams);
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_SetCustomInstanceData(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomInstanceData& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IsmProxy
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_SetCustomInstanceDataValue_Late(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomInstanceDataValue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IsmProxy
{
    const auto IsHandleValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(IsHandleValid,
        TEXT("Request_SetCustomInstanceDataValue_Late received an invalid IsmProxy handle [{}]"),
        InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_IsmProxy_LateCustomDataRequests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_SetCustomInstanceDataValue(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomInstanceDataValue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IsmProxy
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_SetCustomPrimitiveData(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_SetCustomPrimitiveData& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IsmProxy
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_IsmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IsmProxy_UE::
    Request_EnableDisable(
        FCk_Handle_IsmProxy& InHandle,
        const FCk_Request_IsmProxy_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IsmProxy
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

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
    Get_IsOutlineApplied(
        const FCk_Handle_IsmProxy& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_IsmProxy_OutlineApplied>();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_OutlineShadowInstanceCount(
        const FCk_Handle_IsmProxy& InHandle)
    -> int32
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_IsmProxy_OutlineApplied>())
    { return INDEX_NONE; }

    const auto& ShadowIsm = InHandle.Get<ck::FFragment_IsmProxy_OutlineApplied>().Get_ShadowIsm();

    if (ck::Is_NOT_Valid(ShadowIsm))
    { return INDEX_NONE; }

    return ShadowIsm->GetInstanceCount();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_IsCelPatternApplied(
        const FCk_Handle_IsmProxy& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_IsmProxy_CelPatternApplied>();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_CelPatternShadowInstanceCount(
        const FCk_Handle_IsmProxy& InHandle)
    -> int32
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_IsmProxy_CelPatternApplied>())
    { return INDEX_NONE; }

    const auto& ShadowIsm = InHandle.Get<ck::FFragment_IsmProxy_CelPatternApplied>().Get_ShadowIsm();

    if (ck::Is_NOT_Valid(ShadowIsm))
    { return INDEX_NONE; }

    return ShadowIsm->GetInstanceCount();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_CelPatternStencilValue(
        const FCk_Handle_IsmProxy& InHandle)
    -> int32
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_IsmProxy_CelPatternApplied>())
    { return 0; }

    return InHandle.Get<ck::FFragment_IsmProxy_CelPatternApplied>().Get_StencilValue();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_CustomInstanceData(
        const FCk_Handle_IsmProxy& InHandle)
    -> TArray<float>
{
    if (ck::Is_NOT_Valid(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_IsmProxy_Current>().Get_CustomInstanceDataValues();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_OutlineShadowCustomData(
        const FCk_Handle_IsmProxy& InHandle)
    -> TArray<float>
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_IsmProxy_OutlineApplied>())
    { return {}; }

    const auto& Applied = InHandle.Get<ck::FFragment_IsmProxy_OutlineApplied>();
    const auto& ShadowIsm = Applied.Get_ShadowIsm();

    if (ck::Is_NOT_Valid(ShadowIsm) || NOT ShadowIsm->IsValidId(Applied.Get_ShadowInstanceId()))
    { return {}; }

    // PerInstanceSMCustomData is a flat NumCustomDataFloats-strided array in INSTANCE order, so the
    // id has to be resolved to its current index (removals compact the array; ids do not).
    const auto& NumFloats = ShadowIsm->NumCustomDataFloats;
    const auto& InstanceIndex = ShadowIsm->GetInstanceIndexForId(Applied.Get_ShadowInstanceId());

    if (NumFloats <= 0 || InstanceIndex == INDEX_NONE)
    { return {}; }

    const auto& Offset = InstanceIndex * NumFloats;

    if (NOT ShadowIsm->PerInstanceSMCustomData.IsValidIndex(Offset + NumFloats - 1))
    { return {}; }

    auto OutCustomData = TArray<float>{};
    OutCustomData.Append(&ShadowIsm->PerInstanceSMCustomData[Offset], NumFloats);

    return OutCustomData;
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_OutlineShadowMaterial(
        const FCk_Handle_IsmProxy& InHandle,
        int32 InSlot)
    -> UMaterialInterface*
{
    if (ck::Is_NOT_Valid(InHandle) || NOT InHandle.Has<ck::FFragment_IsmProxy_OutlineApplied>())
    { return nullptr; }

    const auto& ShadowIsm = InHandle.Get<ck::FFragment_IsmProxy_OutlineApplied>().Get_ShadowIsm();

    if (ck::Is_NOT_Valid(ShadowIsm) || InSlot >= ShadowIsm->GetNumMaterials())
    { return nullptr; }

    return ShadowIsm->GetMaterial(InSlot);
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
    Get_Mesh(
        const FCk_Handle_IsmProxy& InHandle)
    -> UStaticMesh*
{
    const auto& Params = InHandle.Get<ck::FFragment_IsmProxy_Params>();
    const auto& IsmRenderer = Params.Get_IsmRenderer().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(IsmRenderer), TEXT("The Ism Renderer [{}] is INVALID for Proxy Handle [{}]"),
        IsmRenderer, InHandle)
    { return {}; }

    return IsmRenderer->Get_Mesh();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_RelativeSocketTransform(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName)
    -> FTransform
{
    const auto& LocationOffset = Get_LocalLocationOffset(InHandle);
    const auto& RotationOffset = Get_LocalRotationOffset(InHandle);
    const auto& Scale = Get_ScaleMultiplier(InHandle);

    const auto& Transform = FTransform{RotationOffset, LocationOffset, Scale };
    const auto& SocketTransform = UCk_Utils_StaticMesh_UE::Get_RelativeSocketTransform(Get_Mesh(InHandle), InSocketName);

    return SocketTransform * Transform;
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_RelativeSocketLocation(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName)
    -> FVector
{
    const auto& LocationOffset = Get_LocalLocationOffset(InHandle);
    const auto& SocketLocation = UCk_Utils_StaticMesh_UE::Get_RelativeSocketLocation(Get_Mesh(InHandle), InSocketName);

    return SocketLocation + LocationOffset;
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_RelativeSocketRotation(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName)
    -> FRotator
{
    const auto& RotationOffset = Get_LocalRotationOffset(InHandle);
    const auto& SocketRotation = UCk_Utils_StaticMesh_UE::Get_RelativeSocketRotation(Get_Mesh(InHandle), InSocketName);

    return (SocketRotation.Quaternion() * RotationOffset.Quaternion()).Rotator();
}

auto
    UCk_Utils_IsmProxy_UE::
    Get_RelativeSocketScale(
        const FCk_Handle_IsmProxy& InHandle,
        FName InSocketName)
    -> FVector
{
    const auto& Scale = Get_ScaleMultiplier(InHandle);
    const auto& SocketScale = UCk_Utils_StaticMesh_UE::Get_RelativeSocketScale(Get_Mesh(InHandle), InSocketName);

    return SocketScale * Scale;
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

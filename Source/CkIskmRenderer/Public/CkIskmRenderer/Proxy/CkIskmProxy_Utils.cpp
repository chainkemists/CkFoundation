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

auto
    UCk_Utils_IskmProxy_UE::
    BindTo_OnAnimationNotify(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationNotify& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IskmProxy
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_IskmProxy_OnAnimationNotify,
        InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    UnbindFrom_OnAnimationNotify(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationNotify& InDelegate)
    -> FCk_Handle_IskmProxy
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_IskmProxy_OnAnimationNotify, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    BindTo_OnAnimationFinished(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IskmProxy
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_IskmProxy_OnAnimationFinished,
        InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    UnbindFrom_OnAnimationFinished(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnAnimationFinished& InDelegate)
    -> FCk_Handle_IskmProxy
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_IskmProxy_OnAnimationFinished, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_PlayAnimation(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_PlayAnimation& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_StopAnimation(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_StopAnimation& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetPlayRate(
        FCk_Handle_IskmProxy& InHandle,
        float InRate)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetPlayRate{InRate});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PlayingAnimation(const FCk_Handle_IskmProxy& InHandle)
    -> UAnimSequenceBase*
{
    if (ck::Is_NOT_Valid(InHandle)) { return nullptr; }
    return InHandle.Get<ck::FFragment_IskmProxy_AnimState>().Get_CurrentSequence().Get();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PlayTime(const FCk_Handle_IskmProxy& InHandle)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle)) { return 0.0f; }
    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC)) { return 0.0f; }
    return SKMC->GetPosition();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PlayLength(const FCk_Handle_IskmProxy& InHandle)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle)) { return 0.0f; }
    auto* Seq = Get_PlayingAnimation(InHandle);
    return ck::IsValid(Seq) ? Seq->GetPlayLength() : 0.0f;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetCustomDataFloat(
        FCk_Handle_IskmProxy& InHandle,
        int32 InOffset,
        float InValue)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetCustomDataFloat{InOffset, InValue});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_CustomDataFloat(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InOffset)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle)) { return 0.0f; }
    const auto& Cd = InHandle.Get<ck::FFragment_IskmProxy_CustomData>();
    return Cd.Get_Values().IsValidIndex(InOffset) ? Cd.Get_Values()[InOffset] : 0.0f;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_AttachSubmesh(FCk_Handle_IskmProxy& InHandle, FName InSubmeshName)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_AttachSubmesh{InSubmeshName});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_DetachSubmesh(FCk_Handle_IskmProxy& InHandle, FName InSubmeshName)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_DetachSubmesh{InSubmeshName});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_DetachAllSubmeshes(FCk_Handle_IskmProxy& InHandle)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_DetachAllSubmeshes{});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_NumAttachedSubmeshes(const FCk_Handle_IskmProxy& InHandle)
    -> int32
{
    if (ck::Is_NOT_Valid(InHandle)) { return 0; }
    return InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_AttachedSubmeshIndices().Num();
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetAnimInstanceClass(FCk_Handle_IskmProxy& InHandle, TSubclassOf<UAnimInstance> InClass)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle)) { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetAnimInstanceClass{InClass});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PoseSource(const FCk_Handle_IskmProxy& InHandle)
    -> ECk_IskmProxy_PoseSource
{
    if (ck::Is_NOT_Valid(InHandle)) { return ECk_IskmProxy_PoseSource::Sequence; }
    return InHandle.Get<ck::FFragment_IskmProxy_PoseSource>().Get_PoseSource();
}

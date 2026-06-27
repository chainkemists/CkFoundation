#include "CkIskmRenderer/Proxy/CkIskmProxy_Utils.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"
#include "CkIskmRenderer/CkIskmRenderer_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Iskm::GetSocketTransform"), STAT_CkIskm_GetSocketTransform, STATGROUP_CkIskmRenderer);

// --------------------------------------------------------------------------------------------------------------------
// ---- Add / Has ----

auto
    UCk_Utils_IskmProxy_UE::
    Add(
        FCk_Handle_Transform& InHandle,
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
    InHandle.Add<ck::FFragment_IskmProxy_MaterialOverrides>();
    InHandle.Add<ck::FFragment_IskmProxy_MorphTargets>();
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

// --------------------------------------------------------------------------------------------------------------------
// ---- Getters ----

auto
    UCk_Utils_IskmProxy_UE::
    Get_PlayingAnimation(const FCk_Handle_IskmProxy& InHandle)
    -> UAnimSequenceBase*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }
    return InHandle.Get<ck::FFragment_IskmProxy_AnimState>().Get_CurrentSequence().Get();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PlayTime(const FCk_Handle_IskmProxy& InHandle)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0.0f; }
    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC))
    { return 0.0f; }
    return SKMC->GetPosition();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PlayLength(const FCk_Handle_IskmProxy& InHandle)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0.0f; }
    auto* Seq = Get_PlayingAnimation(InHandle);
    return ck::IsValid(Seq) ? Seq->GetPlayLength() : 0.0f;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_CustomDataFloat(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InOffset)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0.0f; }
    const auto& Cd = InHandle.Get<ck::FFragment_IskmProxy_CustomData>();
    return Cd.Get_Values().IsValidIndex(InOffset) ? Cd.Get_Values()[InOffset] : 0.0f;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_MaterialOverride(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InSlotIndex)
    -> UMaterialInterface*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }

    const auto& Overrides = InHandle.Get<ck::FFragment_IskmProxy_MaterialOverrides>();
    if (const auto* Found = Overrides.Get_SlotToMaterial().Find(InSlotIndex))
    {
        return Found->Get();
    }
    return nullptr;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_Material(
        const FCk_Handle_IskmProxy& InHandle,
        int32 InSlotIndex)
    -> UMaterialInterface*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }

    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC))
    { return nullptr; }

    // GetMaterial resolves override-then-mesh-default and returns nullptr for
    // out-of-range slots — no extra range guard needed.
    return SKMC->GetMaterial(InSlotIndex);
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_MorphTarget(
        const FCk_Handle_IskmProxy& InHandle,
        FName InMorphName)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0.0f; }

    const auto& Morphs = InHandle.Get<ck::FFragment_IskmProxy_MorphTargets>();
    if (const auto* Found = Morphs.Get_Values().Find(InMorphName))
    {
        return *Found;
    }
    return 0.0f;
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_MorphTargetWeight(
        const FCk_Handle_IskmProxy& InHandle,
        FName InMorphName)
    -> float
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0.0f; }

    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC))
    { return 0.0f; }

    return SKMC->GetMorphTarget(InMorphName);
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_NumAttachedSubmeshes(const FCk_Handle_IskmProxy& InHandle)
    -> int32
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0; }
    return InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_AttachedSubmeshIndices().Num();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_ActiveMontage(const FCk_Handle_IskmProxy& InHandle)
    -> UAnimMontage*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }
    return InHandle.Get<ck::FFragment_IskmProxy_AnimState>().Get_CurrentMontage().Get();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_IsRagdolling(const FCk_Handle_IskmProxy& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }
    return InHandle.Has<ck::FTag_IskmProxy_Ragdolling>();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_PoseSource(const FCk_Handle_IskmProxy& InHandle)
    -> ECk_IskmProxy_PoseSource
{
    if (ck::Is_NOT_Valid(InHandle))
    { return ECk_IskmProxy_PoseSource::Sequence; }
    return InHandle.Get<ck::FFragment_IskmProxy_PoseSource>().Get_PoseSource();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_AnimInstance(const FCk_Handle_IskmProxy& InHandle)
    -> UAnimInstance*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }

    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    // Returning null here is legitimate: the SKMC may be in single-node mode
    // with no AnimInstance class set (PlayAnimation's transient state between
    // SetAnimInstanceClass(nullptr) and PlayAnimation completing).
    return SKMC->GetAnimInstance();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_SocketTransform(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace)
    -> FTransform
{
    SCOPE_CYCLE_COUNTER(STAT_CkIskm_GetSocketTransform);

    if (ck::Is_NOT_Valid(InHandle))
    { return FTransform::Identity; }
    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC))
    { return FTransform::Identity; }
    const auto Space = (InSpace == ECk_IskmProxy_TransformSpace::Component)
        ? RTS_Component : RTS_World;
    return SKMC->GetSocketTransform(InSocketName, Space);
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_SocketLocation(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace)
    -> FVector
{
    return Get_SocketTransform(InHandle, InSocketName, InSpace).GetLocation();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_SocketRotation(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace)
    -> FRotator
{
    return Get_SocketTransform(InHandle, InSocketName, InSpace).Rotator();
}

auto
    UCk_Utils_IskmProxy_UE::
    Get_SocketScale(
        const FCk_Handle_IskmProxy& InHandle,
        FName InSocketName,
        ECk_IskmProxy_TransformSpace InSpace)
    -> FVector
{
    return Get_SocketTransform(InHandle, InSocketName, InSpace).GetScale3D();
}

auto
    UCk_Utils_IskmProxy_UE::
    LineTrace_Instance(
        const FCk_Handle_IskmProxy& InHandle,
        const FCk_IskmProxy_LineTraceParams& InParams,
        FCk_IskmProxy_LineTraceResult& OutResult)
    -> bool
{
    OutResult = FCk_IskmProxy_LineTraceResult{};
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }
    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    if (ck::Is_NOT_Valid(SKMC))
    { return false; }

    auto Hit = FHitResult{};
    constexpr auto bTraceComplex = false;
    const auto Hit_Bool = SKMC->LineTraceComponent(
        Hit, InParams.Get_Start(), InParams.Get_End(),
        FCollisionQueryParams{NAME_None, bTraceComplex});
    OutResult.Set_bHit(Hit_Bool);
    if (Hit_Bool)
    {
        OutResult.Set_Position(Hit.ImpactPoint);
        OutResult.Set_Normal(Hit.ImpactNormal);
        OutResult.Set_BoneName(Hit.BoneName);
    }
    return Hit_Bool;
}

// --------------------------------------------------------------------------------------------------------------------
// ---- Requests ----

auto
    UCk_Utils_IskmProxy_UE::
    Request_PlayAnimation(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_PlayAnimation& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
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
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
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
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetPlayRate{InRate});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetVisibility(
        FCk_Handle_IskmProxy& InHandle,
        bool InIsVisible)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetVisibility{InIsVisible});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetCustomDataFloat(
        FCk_Handle_IskmProxy& InHandle,
        int32 InOffset,
        float InValue)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetCustomDataFloat{InOffset, InValue});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetMaterialOverride(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Request_IskmProxy_SetMaterialOverride& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_ClearMaterialOverrides(FCk_Handle_IskmProxy& InHandle)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_ClearMaterialOverrides{});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetMorphTarget(
        FCk_Handle_IskmProxy& InHandle,
        FName InMorphName,
        float InValue)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetMorphTarget{InMorphName, InValue});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_ClearMorphTargets(FCk_Handle_IskmProxy& InHandle)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_ClearMorphTargets{});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetSkeletalMesh(
        FCk_Handle_IskmProxy& InHandle,
        USkeletalMesh* InMesh)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetSkeletalMesh{InMesh});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_AttachSubmesh(FCk_Handle_IskmProxy& InHandle, FName InSubmeshName)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_AttachSubmesh{InSubmeshName});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_DetachSubmesh(FCk_Handle_IskmProxy& InHandle, FName InSubmeshName)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_DetachSubmesh{InSubmeshName});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_DetachAllSubmeshes(FCk_Handle_IskmProxy& InHandle)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_DetachAllSubmeshes{});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_SetAnimInstanceClass(FCk_Handle_IskmProxy& InHandle, TSubclassOf<UAnimInstance> InClass)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(
        FCk_Request_IskmProxy_SetAnimInstanceClass{InClass});
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_PlayMontage(FCk_Handle_IskmProxy& InHandle, const FCk_Request_IskmProxy_PlayMontage& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_StopMontage(FCk_Handle_IskmProxy& InHandle, const FCk_Request_IskmProxy_StopMontage& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_BeginRagdoll(FCk_Handle_IskmProxy& InHandle, const FCk_Request_IskmProxy_BeginRagdoll& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Request_EndRagdoll(FCk_Handle_IskmProxy& InHandle, const FCk_Request_IskmProxy_EndRagdoll& InRequest)
    -> FCk_Handle_IskmProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_IskmProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    Add_SocketFollower(
        FCk_Handle_IskmProxy& InLeader,
        FCk_Handle_Transform& InFollower,
        FName InSocketName,
        const FTransform& InOffset)
    -> FCk_Handle_Transform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InLeader),
        TEXT("Add_SocketFollower: leader proxy is invalid for follower [{}]"), InFollower)
    { return InFollower; }

    InFollower.Add<ck::FFragment_IskmProxy_SocketFollower>(InLeader, InSocketName, InOffset);
    return InFollower;
}

// --------------------------------------------------------------------------------------------------------------------
// ---- Binds ----

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
    BindTo_OnMontageFinished(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnMontageFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IskmProxy
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_IskmProxy_OnMontageFinished,
        InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_IskmProxy_UE::
    UnbindFrom_OnMontageFinished(
        FCk_Handle_IskmProxy& InHandle,
        const FCk_Delegate_IskmProxy_OnMontageFinished& InDelegate)
    -> FCk_Handle_IskmProxy
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_IskmProxy_OnMontageFinished, InHandle, InDelegate);
    return InHandle;
}

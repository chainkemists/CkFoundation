#include "CkResolverTarget_Utils.h"

#include "ResolverTarget/CkResolverTarget_Fragment.h"

#include "TargetPoint/CkTargetPoint_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverTarget_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams)
    -> FCk_Handle_ResolverTarget
{
    InHandle.Add<ck::FFragment_ResolverTarget_Params>(InParams);
    return Cast(InHandle);
}

auto
    UCk_Utils_ResolverTarget_UE::
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_ResolverTarget
{
    auto ResolverTarget = UCk_Utils_TargetPoint_UE::Create(InOwner, InTransform, InLifetime);
    Add(ResolverTarget, InParams);
    return Cast(ResolverTarget);
}

auto
    UCk_Utils_ResolverTarget_UE::
    Create_Transient(
        const FTransform& InTransform,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_ResolverTarget
{
    auto ResolverTarget = UCk_Utils_TargetPoint_UE::Create_Transient(InTransform, InWorldContextObject, InLifetime);
    Add(ResolverTarget, InParams);
    return Cast(ResolverTarget);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ResolverTarget, UCk_Utils_ResolverTarget_UE, FCk_Handle_ResolverTarget,
    ck::FFragment_ResolverTarget_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverTarget_UE::
    ForEach_ResolverDataBundle(
        FCk_Handle_ResolverTarget& InTarget,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_ResolverDataBundle>
{
    auto ResolverDataBundles = TArray<FCk_Handle_ResolverDataBundle>{};

    ForEach_ResolverDataBundle(InTarget, [&](FCk_Handle_ResolverDataBundle InDataBundle)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InDataBundle, InOptionalPayload); }
        else
        { ResolverDataBundles.Emplace(InDataBundle); }
    });

    return ResolverDataBundles;
}

auto
    UCk_Utils_ResolverTarget_UE::
    ForEach_ResolverDataBundle(
        FCk_Handle_ResolverTarget& InTarget,
        const TFunction<void(FCk_Handle_ResolverDataBundle)>& InFunc)
    -> void
{
    using RecordOfAbilities_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfDataBundles>;
    RecordOfAbilities_Utils::ForEach_ValidEntry(InTarget, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverTarget_UE::
    Request_InitiateNewResolution(
        FCk_Handle_ResolverTarget& InResolverTarget,
        const FCk_Request_ResolverTarget_InitiateNewResolution& InRequest,
        FCk_Delegate_ResolverTarget_OnNewResolverDataBundle InDelegate)
    -> FCk_Handle_ResolverTarget
{
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_ResolverTarget_OnNewResolverDataBundle,
        InRequest.PopulateRequestHandle(InResolverTarget), InDelegate);

    InResolverTarget.AddOrGet<ck::FFragment_ResolverTarget_Requests>()._ResolverRequests.Emplace(InRequest);
    return InResolverTarget;
}

auto
    UCk_Utils_ResolverTarget_UE::
    BindTo_OnNewResolverDataBundle(
        FCk_Handle_ResolverTarget& InResolverTarget,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverTarget_OnNewResolverDataBundle& InDelegate)
    -> FCk_Handle_ResolverTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_ResolverTarget_OnNewResolverDataBundle, InResolverTarget, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InResolverTarget;
}

auto
    UCk_Utils_ResolverTarget_UE::
    UnbindFrom_OnNewResolverDataBundle(
        FCk_Handle_ResolverTarget& InResolverTarget,
        const FCk_Delegate_ResolverTarget_OnNewResolverDataBundle& InDelegate)
    -> FCk_Handle_ResolverTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_ResolverTarget_OnNewResolverDataBundle, InResolverTarget, InDelegate);
    return InResolverTarget;
}

// --------------------------------------------------------------------------------------------------------------------

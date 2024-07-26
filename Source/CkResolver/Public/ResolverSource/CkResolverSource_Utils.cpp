#include "CkResolverSource_Utils.h"

#include "CkTargetPoint_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"

#include "ResolverSource/CkResolverSource_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverSource_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ResolverSource_ParamsData& InParams)
    -> FCk_Handle_ResolverSource
{
    InHandle.Add<ck::FFragment_ResolverSource_Params>(InParams);
    RecordOfDataBundles_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    return Cast(InHandle);
}

auto
    UCk_Utils_ResolverSource_UE::
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform,
        const FCk_Fragment_ResolverSource_ParamsData& InParams,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_ResolverSource
{
    auto ResolverSource = UCk_Utils_TargetPoint_UE::Create(InOwner, InTransform, InLifetime);
    Add(ResolverSource, InParams);
    return Cast(ResolverSource);
}

auto
    UCk_Utils_ResolverSource_UE::
    Create_Transient(
        const FTransform& InTransform,
        const FCk_Fragment_ResolverSource_ParamsData& InParams,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime)
    -> FCk_Handle_ResolverSource
{
    auto ResolverSource = UCk_Utils_TargetPoint_UE::Create_Transient(InTransform, InWorldContextObject, InLifetime);
    Add(ResolverSource, InParams);
    return Cast(ResolverSource);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ResolverSource, UCk_Utils_ResolverSource_UE, FCk_Handle_ResolverSource,
    ck::FFragment_ResolverSource_Params, ck::FFragment_RecordOfDataBundles)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverSource_UE::
    ForEach_ResolverDataBundle(
        FCk_Handle_ResolverSource& InSource,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_ResolverDataBundle>
{
    auto ResolverDataBundles = TArray<FCk_Handle_ResolverDataBundle>{};

    ForEach_ResolverDataBundle(InSource, [&](FCk_Handle_ResolverDataBundle InDataBundle)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InDataBundle, InOptionalPayload); }
        else
        { ResolverDataBundles.Emplace(InDataBundle); }
    });

    return ResolverDataBundles;
}

auto
    UCk_Utils_ResolverSource_UE::
    ForEach_ResolverDataBundle(
        FCk_Handle_ResolverSource& InSource,
        const TFunction<void(FCk_Handle_ResolverDataBundle)>& InFunc)
    -> void
{
    using RecordOfAbilities_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfDataBundles>;
    RecordOfAbilities_Utils::ForEach_ValidEntry(InSource, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverSource_UE::
    Request_InitiateNewResolution(
        FCk_Handle_ResolverSource& InResolverSource,
        const FCk_Request_ResolverSource_InitiateNewResolution& InRequest,
        FCk_Delegate_ResolverSource_OnNewResolverDataBundle InDelegate)
    -> FCk_Handle_ResolverSource
{
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_ResolverSource_OnNewResolverDataBundle,
        InRequest.PopulateRequestHandle(InResolverSource), InDelegate);

    InResolverSource.AddOrGet<ck::FFragment_ResolverSource_Requests>()._ResolverRequests.Emplace(InRequest);
    return InResolverSource;
}

auto
    UCk_Utils_ResolverSource_UE::
    BindTo_OnNewResolverDataBundle(
        FCk_Handle_ResolverSource& InResolverSource,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ResolverSource_OnNewResolverDataBundle& InDelegate)
    -> FCk_Handle_ResolverSource
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_ResolverSource_OnNewResolverDataBundle, InResolverSource, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InResolverSource;
}

auto
    UCk_Utils_ResolverSource_UE::
    UnbindFrom_OnNewResolverDataBundle(
        FCk_Handle_ResolverSource& InResolverSource,
        const FCk_Delegate_ResolverSource_OnNewResolverDataBundle& InDelegate)
    -> FCk_Handle_ResolverSource
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_ResolverSource_OnNewResolverDataBundle, InResolverSource, InDelegate);
    return InResolverSource;
}

// --------------------------------------------------------------------------------------------------------------------

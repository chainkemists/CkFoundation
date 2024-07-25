#include "CkResolverSource_Utils.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment.h"

#include "ResolverSource/CkResolverSource_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverSource_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ResolverSource_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_ResolverSource_Params>(InParams);
    InHandle.Add<ck::FFragment_RecordOfDataBundles>();
    RecordOfDataBundles_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ResolverSource, UCk_Utils_ResolverSource_UE, FCk_Handle_ResolverSource,
    ck::FFragment_ResolverSource_Params, ck::FFragment_RecordOfDataBundles)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverSource_UE::
    Request_InitiateNewResolution(
        FCk_Handle_ResolverSource& InResolverSource,
        const FCk_Request_ResolverSource_InitiateNewResolution& InRequest,
        const FCk_Delegate_ResolverSource_OnNewResolverDataBundle& InDelegate)
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

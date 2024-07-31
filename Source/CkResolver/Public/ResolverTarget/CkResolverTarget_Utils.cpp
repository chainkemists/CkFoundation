#include "CkResolverTarget_Utils.h"

#include "ResolverTarget/CkResolverTarget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResolverTarget_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ResolverTarget_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_ResolverTarget_Params>(InParams);
    RecordOfDataBundles_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ResolverTarget, UCk_Utils_ResolverTarget_UE, FCk_Handle_ResolverTarget,
    ck::FFragment_ResolverTarget_Params, ck::FFragment_RecordOfDataBundles)

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

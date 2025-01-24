#include "CkProbe_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams,
        const FCk_Probe_DebugInfo& InDebugInfo)
        -> FCk_Handle_Probe
{
    InHandle.Add<ck::FFragment_Probe_Params>(InParams);
    InHandle.Add<ck::FFragment_Probe_DebugInfo>(InDebugInfo);
    InHandle.Add<ck::FFragment_Probe_Current>();

    InHandle.Add<ck::FTag_Probe_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Probe_UE, FCk_Handle_Probe,
    ck::FFragment_Probe_Params, ck::FFragment_Probe_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Get_IsOverlapping(
        const FCk_Handle_Probe& InProbe)
        -> bool
{
    return InProbe.Has<ck::FTag_Probe_Overlapping>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Request_BeginOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_BeginOverlap& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FTag_Probe_Overlapping>();
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_OverlapPersisted(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_OverlapPersisted& InRequest)
        -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(InProbe.Has<ck::FTag_Probe_Overlapping>(),
        TEXT("Cannot Request Overlap Persisted on Probe [{}] because it is NOT overlapping"), InProbe)
    {
        return InProbe;
    }

    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EndOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EndOverlap& InRequest)
        -> FCk_Handle_Probe
{
    InProbe.Remove<ck::FTag_Probe_Overlapping>();
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeBeginOverlap, InProbeEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeBeginOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnOverlapPersisted(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnOverlapPersisted& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeOverlapPersisted, InProbeEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnOverlapPersisted(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnOverlapPersisted& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeOverlapPersisted, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnEndOverlap(
        FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate)
        -> FCk_Handle_Probe
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

// --------------------------------------------------------------------------------------------------------------------

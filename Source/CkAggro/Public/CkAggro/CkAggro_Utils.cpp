#include "CkAggro_Utils.h"

#include "CkAggro/CkAggroTarget_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_aggro_utils
{
    // Stamp the lowest-threat tracked target PendingForget and drop it from the map to make room for a newcomer.
    // The Forget processor completes the eviction (record disconnect + destroy + OnAggroTargetForgotten/Evicted).
    auto
    Evict_LowestThreat(
        TMap<FCk_Handle, FCk_Handle_AggroTarget>& InTargetMap)
        -> void
    {
        auto LowestThreat = TNumericLimits<float>::Max();
        auto Victim       = FCk_Handle_AggroTarget{};
        auto VictimKey    = FCk_Handle{};

        for (const auto& Pair : InTargetMap)
        {
            if (ck::Is_NOT_Valid(Pair.Value))
            { continue; }

            const auto Threat = Pair.Value.Get<ck::FFragment_AggroTarget_Threat>().Get_Threat();
            if (Threat < LowestThreat)
            {
                LowestThreat = Threat;
                Victim       = Pair.Value;
                VictimKey    = Pair.Key;
            }
        }

        if (ck::Is_NOT_Valid(Victim))
        { return; }

        Victim.AddOrGet<ck::FTag_AggroTarget_Evicted>();
        Victim.AddOrGet<ck::FTag_AggroTarget_PendingForget>();
        InTargetMap.Remove(VictimKey);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Aggro_ParamsData& InParams)
    -> FCk_Handle_Aggro
{
    const auto HasTransform = UCk_Utils_Transform_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(HasTransform,
        TEXT("Cannot Add Aggro to Handle [{}] — it has no Transform feature (required for spatial scoring)"), InHandle)
    {}
    if (NOT HasTransform)
    { return {}; }

    // Clamp the default-target template's retention >= acquisition (AggroTarget::Add re-clamps per target too).
    auto DefaultTargetParams = InParams.Get_DefaultTargetParams();
    {
        auto SpatialParams = DefaultTargetParams.Get_SpatialParams();
        if (SpatialParams.Get_RetentionDistance() < SpatialParams.Get_AcquisitionDistance())
        { SpatialParams.Set_RetentionDistance(SpatialParams.Get_AcquisitionDistance()); }
        DefaultTargetParams.Set_SpatialParams(SpatialParams);
    }

    RecordOfAggroTargets_Utils::AddIfMissing(InHandle);

    InHandle.Add<ck::FFragment_Aggro_DefaultTargetParams>(DefaultTargetParams);
    InHandle.Add<ck::FFragment_Aggro_SelectionParams>(InParams.Get_SelectionParams());
    InHandle.Add<ck::FFragment_Aggro_CapParams>(InParams.Get_CapParams());
    InHandle.Add<ck::FFragment_Aggro_EvaluationParams>(InParams.Get_EvaluationParams());

    InHandle.Add<ck::FFragment_Aggro_Current>();
    InHandle.Add<ck::FFragment_Aggro_EvaluationClock>();
    InHandle.Add<ck::FFragment_Aggro_TargetMap>();

    InHandle.Add<ck::FTag_Aggro>();
    InHandle.Add<ck::FTag_Aggro_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Aggro_UE, FCk_Handle_Aggro, ck::FTag_Aggro)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    DoCreateTarget(
        FCk_Handle_Aggro& InOwner,
        const FCk_Fragment_AggroTarget_ParamsData& InParams)
    -> FCk_Handle_AggroTarget
{
    const auto TrackedEntity  = InParams.Get_TrackedEntity();
    const auto TrackedIsValid = ck::IsValid(TrackedEntity);
    CK_ENSURE_IF_NOT(TrackedIsValid,
        TEXT("Cannot CreateTarget on Aggro [{}] — the tracked entity is INVALID"), InOwner)
    {}
    if (NOT TrackedIsValid)
    { return {}; }

    auto& TargetMap = InOwner.Get<ck::FFragment_Aggro_TargetMap>()._TargetsByTrackedEntity;

    if (const auto Existing = TargetMap.Find(TrackedEntity))
    { return *Existing; }

    const auto& CapParams = InOwner.Get<ck::FFragment_Aggro_CapParams>();
    if (CapParams.Get_TargetCapMode() == ECk_EnableDisable::Enable &&
        TargetMap.Num() >= CapParams.Get_MaximumTrackedTargets())
    {
        if (CapParams.Get_EvictionPolicy() == ECk_Aggro_EvictionPolicy::RejectNew)
        { return {}; }

        ck_aggro_utils::Evict_LowestThreat(TargetMap);
    }

    // AggroTarget::Create takes a generic owner (it knows nothing of the Aggro type) — it builds the child, Adds the
    // feature, and connects it to the owner's record. We only own the O(1) dedupe map here.
    auto OwnerHandle = FCk_Handle{InOwner};
    auto NewTarget   = UCk_Utils_AggroTarget_UE::Create(OwnerHandle, InParams);
    if (ck::Is_NOT_Valid(NewTarget))
    { return {}; }

    TargetMap.Add(TrackedEntity, NewTarget);

    return NewTarget;
}

auto
    UCk_Utils_Aggro_UE::
    CreateTarget(
        FCk_Handle_Aggro& InOwner,
        FCk_Handle InTracked)
    -> FCk_Handle_AggroTarget
{
    const auto OwnerIsValid = ck::IsValid(InOwner) && Has(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Cannot CreateTarget — Owner [{}] is invalid or is not an Aggro entity"), InOwner)
    {}
    if (NOT OwnerIsValid)
    { return {}; }

    auto Params = InOwner.Get<ck::FFragment_Aggro_DefaultTargetParams>();
    Params.Set_TrackedEntity(InTracked);

    return DoCreateTarget(InOwner, Params);
}

auto
    UCk_Utils_Aggro_UE::
    CreateTarget_WithParams(
        FCk_Handle_Aggro& InOwner,
        FCk_Handle InTracked,
        const FCk_AggroTarget_ParamOverrides& InOverrides)
    -> FCk_Handle_AggroTarget
{
    const auto OwnerIsValid = ck::IsValid(InOwner) && Has(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Cannot CreateTarget_WithParams — Owner [{}] is invalid or is not an Aggro entity"), InOwner)
    {}
    if (NOT OwnerIsValid)
    { return {}; }

    auto Params = InOwner.Get<ck::FFragment_Aggro_DefaultTargetParams>();
    Params.Set_TrackedEntity(InTracked);

    if (InOverrides.Get_OverrideThreat())   { Params.Set_ThreatParams(InOverrides.Get_ThreatParams()); }
    if (InOverrides.Get_OverrideSpatial())  { Params.Set_SpatialParams(InOverrides.Get_SpatialParams()); }
    if (InOverrides.Get_OverrideForget())   { Params.Set_ForgetParams(InOverrides.Get_ForgetParams()); }
    if (InOverrides.Get_OverrideScore())    { Params.Set_ScoreParams(InOverrides.Get_ScoreParams()); }
    if (InOverrides.Get_OverrideLifetime()) { Params.Set_LifetimeParams(InOverrides.Get_LifetimeParams()); }

    return DoCreateTarget(InOwner, Params);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    Get_IsEnabled(
        const FCk_Handle_Aggro& InAggro)
    -> bool
{
    return NOT InAggro.Has<ck::FTag_Aggro_Disabled>();
}

auto
    UCk_Utils_Aggro_UE::
    Get_ActiveTarget(
        const FCk_Handle_Aggro& InAggro)
    -> FCk_Handle_AggroTarget
{
    return InAggro.Get<ck::FFragment_Aggro_Current>().Get_ActiveTarget();
}

auto
    UCk_Utils_Aggro_UE::
    TryGet_ActiveTrackedEntity(
        const FCk_Handle_Aggro& InAggro)
    -> FCk_Handle
{
    const auto ActiveTarget = Get_ActiveTarget(InAggro);
    if (ck::Is_NOT_Valid(ActiveTarget))
    { return {}; }

    return UCk_Utils_AggroTarget_UE::Get_TrackedEntity(ActiveTarget);
}

auto
    UCk_Utils_Aggro_UE::
    TryGet_Target_ByTrackedEntity(
        const FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity)
    -> FCk_Handle_AggroTarget
{
    const auto& TargetMap = InAggro.Get<ck::FFragment_Aggro_TargetMap>().Get_TargetsByTrackedEntity();
    if (const auto Found = TargetMap.Find(InTrackedEntity))
    { return *Found; }

    return {};
}

auto
    UCk_Utils_Aggro_UE::
    Get_NumTrackedTargets(
        const FCk_Handle_Aggro& InAggro)
    -> int32
{
    return InAggro.Get<ck::FFragment_Aggro_TargetMap>().Get_TargetsByTrackedEntity().Num();
}

auto
    UCk_Utils_Aggro_UE::
    Get_Debug_EvaluationCount(
        const FCk_Handle_Aggro& InAggro)
    -> int64
{
    return InAggro.Get<ck::FFragment_Aggro_EvaluationClock>().Get_DebugEvaluationCount();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    ForEach_Target(
        const FCk_Handle_Aggro& InAggro,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_AggroTarget>
{
    auto Targets = TArray<FCk_Handle_AggroTarget>{};

    ForEach_Target(InAggro, [&](FCk_Handle_AggroTarget InTarget)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InTarget, InOptionalPayload); }
        else
        { Targets.Emplace(InTarget); }
    });

    return Targets;
}

auto
    UCk_Utils_Aggro_UE::
    ForEach_Target(
        const FCk_Handle_Aggro& InAggro,
        const TFunction<void(FCk_Handle_AggroTarget)>& InFunc)
    -> void
{
    RecordOfAggroTargets_Utils::ForEach_ValidEntry(InAggro, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    Request_AddThreat(
        FCk_Handle_Aggro& InAggro,
        FCk_Request_Aggro_AddThreat InRequest)
    -> FCk_Handle_Aggro
{
    CK_CALLSTACK_RECORD(ck::FFragment_Aggro_Requests, InAggro);

    InAggro.AddOrGet<ck::FFragment_Aggro_Requests>()._Requests.Emplace(InRequest);

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_RemoveTarget(
        FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity)
    -> FCk_Handle_Aggro
{
    CK_CALLSTACK_RECORD(ck::FFragment_Aggro_Requests, InAggro);

    InAggro.AddOrGet<ck::FFragment_Aggro_Requests>()._Requests.Emplace(
        FCk_Request_Aggro_RemoveTarget{InTrackedEntity});

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_ClearAllTargets(
        FCk_Handle_Aggro& InAggro)
    -> FCk_Handle_Aggro
{
    CK_CALLSTACK_RECORD(ck::FFragment_Aggro_Requests, InAggro);

    InAggro.AddOrGet<ck::FFragment_Aggro_Requests>()._Requests.Emplace(
        FCk_Request_Aggro_ClearAllTargets{});

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_SetActiveTarget(
        FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity)
    -> FCk_Handle_Aggro
{
    CK_CALLSTACK_RECORD(ck::FFragment_Aggro_Requests, InAggro);

    InAggro.AddOrGet<ck::FFragment_Aggro_Requests>()._Requests.Emplace(
        FCk_Request_Aggro_SetActiveTarget{InTrackedEntity});

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_ClearActiveTarget(
        FCk_Handle_Aggro& InAggro)
    -> FCk_Handle_Aggro
{
    CK_CALLSTACK_RECORD(ck::FFragment_Aggro_Requests, InAggro);

    InAggro.AddOrGet<ck::FFragment_Aggro_Requests>()._Requests.Emplace(
        FCk_Request_Aggro_ClearActiveTarget{});

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_EnableDisable(
        FCk_Handle_Aggro& InAggro,
        ECk_EnableDisable InEnableDisable)
    -> FCk_Handle_Aggro
{
    switch (InEnableDisable)
    {
        case ECk_EnableDisable::Enable:
        {
            // Counted decrement — one Enable cancels one Disable. A no-op when not disabled.
            if (InAggro.Has<ck::FTag_Aggro_Disabled>())
            { InAggro.Remove<ck::FTag_Aggro_Disabled>(); }
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InAggro.Add<ck::FTag_Aggro_Disabled>();
            break;
        }
    }

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Request_MarkPerceived_ByTrackedEntity(
        FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity,
        FCk_Request_AggroTarget_MarkPerceived InRequest)
    -> FCk_Handle_Aggro
{
    auto Target = TryGet_Target_ByTrackedEntity(InAggro, InTrackedEntity);
    if (ck::IsValid(Target))
    { UCk_Utils_AggroTarget_UE::Request_MarkPerceived(Target, InRequest); }

    return InAggro;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    BindTo_OnTargetAcquired(
        FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetAcquired& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Aggro
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAggroTargetAcquired, InAggro, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    UnbindFrom_OnTargetAcquired(
        FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetAcquired& InDelegate)
    -> FCk_Handle_Aggro
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAggroTargetAcquired, InAggro, InDelegate);

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    BindTo_OnActiveTargetChanged(
        FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnActiveTargetChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Aggro
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAggroActiveTargetChanged, InAggro, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    UnbindFrom_OnActiveTargetChanged(
        FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnActiveTargetChanged& InDelegate)
    -> FCk_Handle_Aggro
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAggroActiveTargetChanged, InAggro, InDelegate);

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    BindTo_OnTargetForgotten(
        FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetForgotten& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Aggro
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAggroTargetForgotten, InAggro, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InAggro;
}

auto
    UCk_Utils_Aggro_UE::
    UnbindFrom_OnTargetForgotten(
        FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetForgotten& InDelegate)
    -> FCk_Handle_Aggro
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAggroTargetForgotten, InAggro, InDelegate);

    return InAggro;
}

// --------------------------------------------------------------------------------------------------------------------

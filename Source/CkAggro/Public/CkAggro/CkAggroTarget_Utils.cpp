#include "CkAggroTarget_Utils.h"

#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Utils.h"

#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_aggro_target_utils
{
    auto
    Get_Now(
        const FCk_Handle& InHandle)
        -> FCk_Time
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        return UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AggroTarget_UE::
    Create(
        FCk_Handle_Aggro& InOwner,
        const FCk_Fragment_AggroTarget_ParamsData& InParams)
    -> FCk_Handle_AggroTarget
{
    const auto OwnerIsValid = ck::IsValid(InOwner) && InOwner.Has<ck::FTag_Aggro>();
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Cannot Create an AggroTarget — Owner [{}] is invalid or is not an Aggro entity"), InOwner)
    {}
    if (NOT OwnerIsValid)
    { return {}; }

    const auto TrackedEntityIsValid = ck::IsValid(InParams.Get_TrackedEntity());
    CK_ENSURE_IF_NOT(TrackedEntityIsValid,
        TEXT("Cannot Create an AggroTarget on Owner [{}] — the supplied TrackedEntity is INVALID"), InOwner)
    {}
    if (NOT TrackedEntityIsValid)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner, [&](FCk_Handle InNewEntity)
    {
        InNewEntity.Add<ck::FTag_AggroTarget>();
        ck::UAggroTarget_TrackedEntity_Utils::AddOrReplace(InNewEntity, InParams.Get_TrackedEntity());

        InNewEntity.Add<ck::FFragment_AggroTarget_TargetInfo>(InOwner, InParams.Get_Instigator(), InParams.Get_Source());
        InNewEntity.Add<ck::FFragment_AggroTarget_ThreatParams>(InParams.Get_ThreatParams());
        InNewEntity.Add<ck::FFragment_AggroTarget_ScoreParams>(InParams.Get_ScoreParams());
        InNewEntity.Add<ck::FFragment_AggroTarget_LifetimeParams>(InParams.Get_LifetimeParams());
        InNewEntity.Add<ck::FFragment_AggroTarget_Threat>();
        InNewEntity.Add<ck::FFragment_AggroTarget_Perception>();
        InNewEntity.Add<ck::FFragment_AggroTarget_Score>();
    });

    return UCk_Utils_AggroTarget_UE::CastChecked(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AggroTarget_UE, FCk_Handle_AggroTarget, ck::FTag_AggroTarget)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AggroTarget_UE::
    Get_TrackedEntity(
        const FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle
{
    return ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InTarget);
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Instigator(
        const FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle
{
    return InTarget.Get<ck::FFragment_AggroTarget_TargetInfo>().Get_Instigator();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Source(
        const FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle
{
    return InTarget.Get<ck::FFragment_AggroTarget_TargetInfo>().Get_Source();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_AggroOwner(
        const FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle_Aggro
{
    return UCk_Utils_Aggro_UE::Cast(InTarget.Get<ck::FFragment_AggroTarget_TargetInfo>().Get_AggroOwner());
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Threat(
        const FCk_Handle_AggroTarget& InTarget)
    -> float
{
    // Analytic decay applied as-of-now, WITHOUT advancing the stored anchor (a pure read).
    const auto& Threat = InTarget.Get<ck::FFragment_AggroTarget_Threat>();
    const auto  Owner  = Get_AggroOwner(InTarget);

    const auto OwnerReady = ck::IsValid(Owner)
        && Owner.Has<ck::FFragment_Aggro_ThreatParams>()
        && Owner.Has<ck::FFragment_Aggro_SpatialParams>()
        && Owner.Has<ck::FFragment_Aggro_ForgetParams>();

    if (NOT OwnerReady)
    { return Threat.Get_Threat(); }

    const auto& OwnerThreat  = Owner.Get<ck::FFragment_Aggro_ThreatParams>();
    const auto& OwnerSpatial = Owner.Get<ck::FFragment_Aggro_SpatialParams>();
    const auto& OwnerForget  = Owner.Get<ck::FFragment_Aggro_ForgetParams>();
    const auto& Perception   = InTarget.Get<ck::FFragment_AggroTarget_Perception>();

    const auto Now          = ck_aggro_target_utils::Get_Now(InTarget);
    const auto Elapsed      = (Now - Threat.Get_LastDecayTime()).Get_Seconds();
    const auto SecsSincePer = (Now - Perception.Get_LastPerceivedTime()).Get_Seconds();
    const auto PerceptK     = UCk_Utils_Aggro_UE::Compute_PerceptionDecayMultiplier(
        InTarget.Has<ck::FTag_AggroTarget_Perceived>(), SecsSincePer,
        OwnerForget.Get_LostSightGraceDuration().Get_Seconds(), OwnerThreat.Get_UnperceivedThreatDecayMultiplier());
    const auto RangeK       = UCk_Utils_Aggro_UE::Compute_RangeDecayMultiplier(
        InTarget.Has<ck::FTag_AggroTarget_WithinRetention>(), OwnerSpatial.Get_OutOfRangeDecayMultiplier());

    return UCk_Utils_Aggro_UE::Compute_DecayedThreat(
        Threat.Get_Threat(), Elapsed, OwnerThreat.Get_ThreatDecayRate(), PerceptK, RangeK,
        OwnerThreat.Get_ThreatClampRange().Get_Min(), OwnerThreat.Get_ThreatClampRange().Get_Max());
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Score(
        const FCk_Handle_AggroTarget& InTarget)
    -> float
{
    return InTarget.Get<ck::FFragment_AggroTarget_Score>().Get_Score();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Distance(
        const FCk_Handle_AggroTarget& InTarget)
    -> float
{
    return InTarget.Get<ck::FFragment_AggroTarget_Score>().Get_Distance();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_LastKnownLocation(
        const FCk_Handle_AggroTarget& InTarget)
    -> FVector
{
    return InTarget.Get<ck::FFragment_AggroTarget_Perception>().Get_LastKnownLocation();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_IsPerceived(
        const FCk_Handle_AggroTarget& InTarget)
    -> bool
{
    return InTarget.Has<ck::FTag_AggroTarget_Perceived>();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_IsActiveTarget(
        const FCk_Handle_AggroTarget& InTarget)
    -> bool
{
    return InTarget.Has<ck::FTag_AggroTarget_IsActive>();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_IsWithinRetention(
        const FCk_Handle_AggroTarget& InTarget)
    -> bool
{
    return InTarget.Has<ck::FTag_AggroTarget_WithinRetention>();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Age(
        const FCk_Handle_AggroTarget& InTarget)
    -> FCk_Time
{
    const auto CreationTime = InTarget.Get<ck::FFragment_AggroTarget_TargetInfo>().Get_CreationTime();
    return ck_aggro_target_utils::Get_Now(InTarget) - CreationTime;
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_TimeSinceLastThreat(
        const FCk_Handle_AggroTarget& InTarget)
    -> FCk_Time
{
    const auto LastThreatTime = InTarget.Get<ck::FFragment_AggroTarget_Threat>().Get_LastThreatTime();
    return ck_aggro_target_utils::Get_Now(InTarget) - LastThreatTime;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AggroTarget_UE::
    Request_AddThreat(
        FCk_Handle_AggroTarget& InTarget,
        float InThreatDelta)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(
        FCk_Request_AggroTarget_AddThreat{InThreatDelta});

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_SetThreat(
        FCk_Handle_AggroTarget& InTarget,
        float InThreat)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(
        FCk_Request_AggroTarget_SetThreat{InThreat});

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_MarkPerceived(
        FCk_Handle_AggroTarget& InTarget,
        FCk_Request_AggroTarget_MarkPerceived InRequest)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(InRequest);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_MarkUnperceived(
        FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(
        FCk_Request_AggroTarget_MarkUnperceived{});

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_ResetPerception(
        FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(
        FCk_Request_AggroTarget_ResetPerception{});

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_Forget(
        FCk_Handle_AggroTarget& InTarget)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(
        FCk_Request_AggroTarget_Forget{});

    return InTarget;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AggroTarget_UE::
    BindTo_OnThreatChanged(
        FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_AggroTarget_OnThreatChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_AggroTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnAggroThreatChanged, InTarget, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    UnbindFrom_OnThreatChanged(
        FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_AggroTarget_OnThreatChanged& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnAggroThreatChanged, InTarget, InDelegate);

    return InTarget;
}

// --------------------------------------------------------------------------------------------------------------------

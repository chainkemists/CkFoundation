#include "CkAggroTarget_Utils.h"

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
    DoAdd_Fragments(
        FCk_Handle& InHandle,
        const FCk_Fragment_AggroTarget_ParamsData& InParams,
        const FCk_Handle& InOwner)
    -> void
{
    // Retention must never be tighter than acquisition, else a target could be acquirable yet never retained.
    auto SpatialParams = InParams.Get_SpatialParams();
    if (SpatialParams.Get_RetentionDistance() < SpatialParams.Get_AcquisitionDistance())
    { SpatialParams.Set_RetentionDistance(SpatialParams.Get_AcquisitionDistance()); }

    const auto Tracked = ck::IsValid(InParams.Get_TrackedEntity()) ? InParams.Get_TrackedEntity() : InHandle;

    InHandle.Add<ck::FTag_AggroTarget>();
    ck::UAggroTarget_TrackedEntity_Utils::AddOrReplace(InHandle, Tracked);

    InHandle.Add<ck::FFragment_AggroTarget_TargetInfo>(InOwner, InParams.Get_Instigator(), InParams.Get_Source());
    InHandle.Add<ck::FFragment_AggroTarget_ThreatParams>(InParams.Get_ThreatParams());
    InHandle.Add<ck::FFragment_AggroTarget_SpatialParams>(SpatialParams);
    InHandle.Add<ck::FFragment_AggroTarget_ForgetParams>(InParams.Get_ForgetParams());
    InHandle.Add<ck::FFragment_AggroTarget_ScoreParams>(InParams.Get_ScoreParams());
    InHandle.Add<ck::FFragment_AggroTarget_LifetimeParams>(InParams.Get_LifetimeParams());
    InHandle.Add<ck::FFragment_AggroTarget_Threat>();
    InHandle.Add<ck::FFragment_AggroTarget_Perception>();
    InHandle.Add<ck::FFragment_AggroTarget_Score>();

    InHandle.Add<ck::FTag_AggroTarget_NeedsSetup>();
}

auto
    UCk_Utils_AggroTarget_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AggroTarget_ParamsData& InParams)
    -> FCk_Handle_AggroTarget
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Cannot Add AggroTarget — the Handle is INVALID"))
    {}
    if (NOT HandleIsValid)
    { return {}; }

    DoAdd_Fragments(InHandle, InParams, FCk_Handle{});

    return CastChecked(InHandle);
}

auto
    UCk_Utils_AggroTarget_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_AggroTarget_ParamsData& InParams)
    -> FCk_Handle_AggroTarget
{
    const auto OwnerIsValid = ck::IsValid(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Cannot Create an AggroTarget — Owner [{}] is INVALID"), InOwner)
    {}
    if (NOT OwnerIsValid)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner, [&](FCk_Handle InNewEntity)
    {
        DoAdd_Fragments(InNewEntity, InParams, InOwner);
    });

    auto NewTarget = CastChecked(NewEntity);

    ck::FUtils_RecordOfAggroTargets::AddIfMissing(InOwner);
    ck::FUtils_RecordOfAggroTargets::Request_Connect(InOwner, NewTarget, ECk_Record_LabelRequirementPolicy::Optional);

    return NewTarget;
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
    -> FCk_Handle
{
    return InTarget.Get<ck::FFragment_AggroTarget_TargetInfo>().Get_AggroOwner();
}

auto
    UCk_Utils_AggroTarget_UE::
    Get_Threat(
        const FCk_Handle_AggroTarget& InTarget)
    -> float
{
    // A pure read: decay is applied as-of-now WITHOUT advancing the stored anchor.
    const auto& Threat     = InTarget.Get<ck::FFragment_AggroTarget_Threat>();
    const auto& ThreatP    = InTarget.Get<ck::FFragment_AggroTarget_ThreatParams>();
    const auto& SpatialP   = InTarget.Get<ck::FFragment_AggroTarget_SpatialParams>();
    const auto& ForgetP    = InTarget.Get<ck::FFragment_AggroTarget_ForgetParams>();
    const auto& Perception = InTarget.Get<ck::FFragment_AggroTarget_Perception>();

    const auto Now          = ck_aggro_target_utils::Get_Now(InTarget);
    const auto Elapsed      = (Now - Threat.Get_LastDecayTime()).Get_Seconds();
    const auto SecsSincePer = (Now - Perception.Get_LastPerceivedTime()).Get_Seconds();
    const auto PerceptK     = UCk_Utils_Aggro_UE::Compute_PerceptionDecayMultiplier(
        InTarget.Has<ck::FTag_AggroTarget_Perceived>(), SecsSincePer,
        ForgetP.Get_LostSightGraceDuration().Get_Seconds(), ThreatP.Get_UnperceivedThreatDecayMultiplier());
    const auto RangeK       = UCk_Utils_Aggro_UE::Compute_RangeDecayMultiplier(
        InTarget.Has<ck::FTag_AggroTarget_WithinRetention>(), SpatialP.Get_OutOfRangeDecayMultiplier());

    return UCk_Utils_Aggro_UE::Compute_DecayedThreat(
        Threat.Get_Threat(), Elapsed, ThreatP.Get_ThreatDecayRate(), PerceptK, RangeK,
        ThreatP.Get_ThreatClampRange().Get_Min(), ThreatP.Get_ThreatClampRange().Get_Max());
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
        float InThreatDelta,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    const auto Request = FCk_Request_AggroTarget_AddThreat{InThreatDelta};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(Request);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_SetThreat(
        FCk_Handle_AggroTarget& InTarget,
        float InThreat,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    const auto Request = FCk_Request_AggroTarget_SetThreat{InThreat};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(Request);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_MarkPerceived(
        FCk_Handle_AggroTarget& InTarget,
        FCk_Request_AggroTarget_MarkPerceived InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(InRequest);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_MarkUnperceived(
        FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    const auto Request = FCk_Request_AggroTarget_MarkUnperceived{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(Request);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_ResetPerception(
        FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    const auto Request = FCk_Request_AggroTarget_ResetPerception{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(Request);

    return InTarget;
}

auto
    UCk_Utils_AggroTarget_UE::
    Request_Forget(
        FCk_Handle_AggroTarget& InTarget,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_AggroTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_AggroTarget_Requests, InTarget);

    const auto Request = FCk_Request_AggroTarget_Forget{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTarget.AddOrGet<ck::FFragment_AggroTarget_Requests>()._Requests.Emplace(Request);

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

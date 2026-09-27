#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_gait_utils
{
    // Stance travel per cycle is kept within this share of the shortest chord any enabled leg can stride.
    constexpr auto StanceTravelShareOfStride = 0.85;

    // The chord a leg can stride along body X at its rest pose's lateral offset: from where the target clamp lets it land
    // ahead to where the reach Emergency forces it up behind. Unset when the lateral offset alone reaches the target radius.
    // It ignores the velocity lead, which may land a swing short of the chord's forward end.
    auto
        Get_Stride(
            const FCk_ProceduralLeg_Placement& InPlacement,
            float InReach,
            const FCk_ProceduralGait_Step& InStep)
        -> TOptional<double>
    {
        const auto Drop = InPlacement.Get_HipLocal().Z - InPlacement.Get_RestFootLocal().Z;
        const auto Lateral = FMath::Abs(InPlacement.Get_RestFootLocal().Y - InPlacement.Get_HipLocal().Y);
        const auto TargetRadius = FMath::Sqrt(FMath::Max(0.0,
            FMath::Square(static_cast<double>(InStep.Get_TargetReachFraction()) * InReach) - FMath::Square(Drop)));
        const auto ForceRadius = FMath::Sqrt(FMath::Max(0.0,
            FMath::Square(static_cast<double>(InStep.Get_ForceStepReachFraction()) * InReach) - FMath::Square(Drop)));

        if (Lateral >= TargetRadius)
        { return {}; }

        return FMath::Sqrt(FMath::Square(TargetRadius) - FMath::Square(Lateral))
            + FMath::Sqrt(FMath::Square(ForceRadius) - FMath::Square(Lateral));
    }

    auto
        Get_Reach(
            const FCk_ProceduralLeg_ChainGeometry& InChain)
        -> float
    {
        auto Reach = 0.0f;
        for (const auto Length : InChain.Get_SegmentLengths())
        {
            Reach += Length;
        }
        return Reach;
    }

    auto
        Get_IsRestWithinReach(
            const FCk_ProceduralLeg_Placement& InPlacement,
            const FCk_ProceduralLeg_ChainGeometry& InChain,
            const FCk_ProceduralGait_Step& InStep)
        -> bool
    {
        return FVector::Dist(InPlacement.Get_RestFootLocal(), InPlacement.Get_HipLocal())
            <= static_cast<double>(InStep.Get_TargetReachFraction()) * Get_Reach(InChain);
    }

    auto
        Get_SupportWeight(
            const FCk_ProceduralLeg_Foot& InFoot)
        -> float
    {
        if (InFoot.Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted)
        { return InFoot.Get_Contact() == ECk_ProceduralLeg_FootContact::Trusted ? 1.0f : 0.0f; }

        const auto SwingAlpha = FMath::Clamp(InFoot.Get_SwingAlpha(), 0.0f, 1.0f);
        return FMath::Max(0.0f, 1.0f - 3.0f * SwingAlpha) + FMath::Max(0.0f, 3.0f * SwingAlpha - 2.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProceduralGait_UE, FCk_Handle_ProceduralGait, ck::FFragment_ProceduralGait);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralGait_UE::
    Add(
        FCk_Handle_Transform& InBody,
        const UCk_ProceduralGait_Data* InData)
    -> FCk_Handle_ProceduralGait
{
    const auto BodyValid = ck::IsValid(InBody)
        && NOT InBody.Has<ck::FTag_DestroyEntity_Initiate>()
        && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InBody)
        && NOT Has(InBody);
    CK_ENSURE_IF_NOT(BodyValid,
        TEXT("Procedural gait Add rejected body [{}]: it must be a live transform entity with no existing gait."), InBody)
    { return {}; }

    const auto DataValid = ck::IsValid(InData)
        && ck::IsValid(InData->Get_Timing())
        && ck::IsValid(InData->Get_Step())
        && ck::IsValid(InData->Get_Probe())
        && ck::IsValid(InData->Get_Foothold());
    CK_ENSURE_IF_NOT(DataValid,
        TEXT("Procedural gait Add rejected body [{}]: the gait data asset is missing or its timing, step, probe or foothold settings are malformed."),
        InBody)
    { return {}; }

    const auto Legs = UCk_Utils_ProceduralLeg_UE::Get_Legs(InBody);
    const auto LegsValid = Legs.Num() >= 2 && Legs.Num() <= 64
        && InData->Get_Timing().Get_MaxSimultaneousSwings() <= Legs.Num();
    CK_ENSURE_IF_NOT(LegsValid,
        TEXT("Procedural gait Add rejected body [{}] with [{}] legs: create 2..64 legs before the gait and keep MaxSimultaneousSwings within the leg count."),
        InBody, Legs.Num())
    { return {}; }

    const auto LegBeyondReach = DoFind_LegBeyondReach(Legs, InData->Get_Step());
    const auto RestsWithinReach = ck::Is_NOT_Valid(LegBeyondReach);
    CK_ENSURE_IF_NOT(RestsWithinReach,
        TEXT("Procedural gait Add rejected body [{}]: leg [{}]'s rest foot lies farther from its hip than TargetReachFraction of its chain length."),
        InBody, LegBeyondReach)
    { return {}; }

    // Admission is atomic: nothing attaches until every authored field has been checked.
    auto Tunables = ck::FFragment_ProceduralGait_Tunables{InData->Get_Timing(), InData->Get_Step(), InData->Get_Probe(),
        InData->Get_Foothold()};
    auto GaitComp = ck::FFragment_ProceduralGait{};
    GaitComp._Legs = Legs;
    const auto Built = DoBuild_SolverSettings(Tunables, GaitComp._Legs, GaitComp._EnabledMask);
    GaitComp._Solver.Set_Settings(Built.Get_Settings());
    GaitComp._Probes.SetNum(Legs.Num());
    GaitComp._Footholds.SetNum(Legs.Num());

    auto DebugComp = ck::FFragment_ProceduralGait_Debug{};
    DebugComp._ReachCadenceFloor = Built.Get_ReachCadenceFloor();
    DebugComp._ReachSkippedLegs = Built.Get_ReachSkippedLegs();
    DebugComp._ScratchLegs.SetNum(Legs.Num());
    DebugComp._Snapshot.Get_Legs().Reserve(Legs.Num());

    InBody.Add<ck::FFragment_ProceduralGait_Tunables>(MoveTemp(Tunables));
    InBody.Add<ck::FFragment_ProceduralGait>(MoveTemp(GaitComp));
    InBody.Add<ck::FFragment_ProceduralGait_Debug>(MoveTemp(DebugComp));
    InBody.Add<ck::FFragment_ProceduralGait_FeetPlane>();
    InBody.Add<ck::FTag_ProceduralGait_NeedsSetup>();

    return CastChecked(InBody);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralGait_UE::
    Get_Status(
        const FCk_Handle_ProceduralGait& InGait)
    -> ECk_ProceduralAnimation_Status
{
    if (ck::Is_NOT_Valid(InGait) || NOT Has(InGait) || InGait.Has<ck::FFragment_ProceduralGait_Failure>())
    { return ECk_ProceduralAnimation_Status::Failed; }

    if (InGait.Has<ck::FTag_ProceduralGait_NeedsSetup>())
    { return ECk_ProceduralAnimation_Status::PendingSetup; }

    return ECk_ProceduralAnimation_Status::Ready;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_Failure(
        const FCk_Handle_ProceduralGait& InGait)
    -> ECk_ProceduralGait_Failure
{
    return ck::IsValid(InGait) && Has(InGait) && InGait.Has<ck::FFragment_ProceduralGait_Failure>()
        ? InGait.Get<ck::FFragment_ProceduralGait_Failure>().Get_Reason()
        : ECk_ProceduralGait_Failure::None;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_GaitClock(
        const FCk_Handle_ProceduralGait& InGait)
    -> float
{
    return Get_Status(InGait) == ECk_ProceduralAnimation_Status::Ready
        ? InGait.Get<ck::FFragment_ProceduralGait>()._Solver.GetGaitClock()
        : 0.0f;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_TrustedContactCount(
        const FCk_Handle_ProceduralGait& InGait)
    -> int32
{
    if (Get_Status(InGait) != ECk_ProceduralAnimation_Status::Ready)
    { return 0; }

    return ck::algo::CountIf(InGait.Get<ck::FFragment_ProceduralGait>()._Legs,
    [](const FCk_Handle_ProceduralLeg& InLeg) -> bool
    {
        return ck::IsValid(InLeg) && UCk_Utils_ProceduralLeg_UE::Get_Foot(InLeg).Get_Contact() == ECk_ProceduralLeg_FootContact::Trusted;
    });
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_PlantedCount(
        const FCk_Handle_ProceduralGait& InGait)
    -> int32
{
    if (Get_Status(InGait) != ECk_ProceduralAnimation_Status::Ready)
    { return 0; }

    return ck::algo::CountIf(InGait.Get<ck::FFragment_ProceduralGait>()._Legs,
    [](const FCk_Handle_ProceduralLeg& InLeg) -> bool
    {
        return ck::IsValid(InLeg) && UCk_Utils_ProceduralLeg_UE::Get_Foot(InLeg).Get_Phase() == ECk_ProceduralLeg_FootPhase::Planted;
    });
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_Legs(
        const FCk_Handle_ProceduralGait& InGait)
    -> TArray<FCk_Handle_ProceduralLeg>
{
    return UCk_Utils_ProceduralLeg_UE::Get_Legs(InGait);
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_EnabledLegCount(
        const FCk_Handle_ProceduralGait& InGait)
    -> int32
{
    if (ck::Is_NOT_Valid(InGait) || NOT Has(InGait))
    { return 0; }

    const auto& GaitComp = InGait.Get<ck::FFragment_ProceduralGait>();
    auto EnabledCount = 0;
    for (auto Index = 0; Index < GaitComp._Legs.Num(); ++Index)
    {
        const auto WasEnabled = (GaitComp._EnabledMask & (uint64{1} << Index)) != 0;
        if (WasEnabled && ck::IsValid(GaitComp._Legs[Index]))
        { ++EnabledCount; }
    }
    return EnabledCount;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_FeetPlane(
        const FCk_Handle_ProceduralGait& InGait)
    -> FCk_ProceduralGait_FeetPlane
{
    if (Get_Status(InGait) != ECk_ProceduralAnimation_Status::Ready || NOT InGait.Has<ck::FFragment_ProceduralGait_FeetPlane>())
    { return {}; }

    const auto& FeetPlane = InGait.Get<ck::FFragment_ProceduralGait_FeetPlane>();
    if (FeetPlane.Get_State() == ck::EProceduralGaitFeetPlane::None)
    { return {}; }

    return FCk_ProceduralGait_FeetPlane{}
        .Set_Point(FeetPlane.Get_Support().Get_Point())
        .Set_Normal(FeetPlane.Get_Support().Get_Normal())
        .Set_State(FeetPlane.Get_State() == ck::EProceduralGaitFeetPlane::Fitted
            ? ECk_ProceduralGait_FeetPlane::Fitted
            : ECk_ProceduralGait_FeetPlane::Held);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralGait_UE::
    Request_ApplyPreset(
        FCk_Handle_ProceduralGait& InGait,
        const FCk_Request_ProceduralGait_ApplyPreset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_ProceduralGait
{
    auto Request = InRequest;
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    const auto RequestValid = ck::IsValid(InGait)
        && Has(InGait)
        && NOT InGait.Has<ck::FTag_DestroyEntity_Initiate>()
        && ck::IsValid(InRequest.Get_Timing())
        && ck::IsValid(InRequest.Get_Step())
        && ck::IsValid(InRequest.Get_Probe())
        && ck::IsValid(InRequest.Get_Foothold())
        && InRequest.Get_Timing().Get_MaxSimultaneousSwings() <= InGait.Get<ck::FFragment_ProceduralGait>()._Legs.Num();
    CK_ENSURE_IF_NOT(RequestValid,
        TEXT("Procedural gait Request_ApplyPreset rejected gait [{}]: the gait must be live and the preset well-formed and within the leg count."),
        InGait)
    {
        Request.TryFireCompletion(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    const auto LegBeyondReach = DoFind_LegBeyondReach(InGait.Get<ck::FFragment_ProceduralGait>()._Legs, InRequest.Get_Step());
    const auto RestsWithinReach = ck::Is_NOT_Valid(LegBeyondReach);
    CK_ENSURE_IF_NOT(RestsWithinReach,
        TEXT("Procedural gait Request_ApplyPreset rejected gait [{}]: leg [{}]'s rest foot lies farther from its hip than TargetReachFraction of its chain length."),
        InGait, LegBeyondReach)
    {
        Request.TryFireCompletion(InGait, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InGait;
    }

    InGait.AddOrGet<ck::FFragment_ProceduralGait_Requests>()._Requests.Emplace(MoveTemp(Request));

    return InGait;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralGait_UE::
    BindTo_OnLegSetChanged(
        FCk_Handle_ProceduralGait& InGait,
        const FCk_Delegate_ProceduralGait_OnLegSetChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProceduralGait
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProceduralGait_LegSetChanged, InGait, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InGait;
}

auto
    UCk_Utils_ProceduralGait_UE::
    UnbindFrom_OnLegSetChanged(
        FCk_Handle_ProceduralGait& InGait,
        const FCk_Delegate_ProceduralGait_OnLegSetChanged& InDelegate)
    -> FCk_Handle_ProceduralGait
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProceduralGait_LegSetChanged, InGait, InDelegate);
    return InGait;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralGait_UE::
    DoBuild_SolverSettings(
        const ck::FFragment_ProceduralGait_Tunables& InTunables,
        const TArray<FCk_Handle_ProceduralLeg>& InLegs,
        uint64 InEnabledMask)
    -> ck::FProceduralGaitBuiltSettings
{
    const auto& Timing = InTunables.Get_Timing();
    const auto& Step = InTunables.Get_Step();

    auto EnabledCount = 0;
    auto ShortestStride = TOptional<double>{};
    auto ReachSkippedLegs = 0;
    for (auto Index = 0; Index < InLegs.Num(); ++Index)
    {
        const auto& Leg = InLegs[Index];
        const auto Enabled = (InEnabledMask & (uint64{1} << Index)) != 0 && ck::IsValid(Leg);
        if (NOT Enabled)
        { continue; }

        ++EnabledCount;
        const auto& Params = Leg.Get<ck::FFragment_ProceduralLeg_Params>();
        const auto Stride = ck_procedural_gait_utils::Get_Stride(Params.Get_Placement(),
            ck_procedural_gait_utils::Get_Reach(Params.Get_Chain()), Step);
        if (NOT Stride.IsSet())
        {
            ++ReachSkippedLegs;
            continue;
        }

        ShortestStride = ShortestStride.IsSet() ? FMath::Min(ShortestStride.GetValue(), Stride.GetValue()) : Stride.GetValue();
    }

    const auto StanceTime = Timing.Get_CycleDuration() - Timing.Get_StepDuration();
    const auto ReachCadenceFloor = ShortestStride.IsSet() && StanceTime > FCk_Time{}
        ? static_cast<float>(ck_procedural_gait_utils::StanceTravelShareOfStride * ShortestStride.GetValue() / StanceTime.Get_Seconds())
        : 0.0f;
    const auto CadenceSpeedRef = ReachCadenceFloor > 0.0f
        ? FMath::Min(Timing.Get_CadenceSpeedRef(), ReachCadenceFloor)
        : Timing.Get_CadenceSpeedRef();

    const auto MaxSimultaneousSwings = Timing.Get_MaxSimultaneousSwings() == 0
        ? FMath::Max(1, EnabledCount / 2)
        : Timing.Get_MaxSimultaneousSwings();

    const auto LegLossPolicy = Timing.Get_LegLossPolicy() == ECk_ProceduralGait_LegLossPolicy::RedistributeOffsets
        ? ck::EProceduralGaitLegLossPolicy::RedistributeOffsets
        : ck::EProceduralGaitLegLossPolicy::KeepAuthoredOffsets;

    auto Settings = ck::FProceduralGaitSettings{};
    Settings.Get_Cadence().Set_CycleDuration(Timing.Get_CycleDuration())
        .Set_MaxSimultaneousSwings(MaxSimultaneousSwings)
        .Set_CadenceSpeedRef(CadenceSpeedRef)
        .Set_MaxCadenceScale(Timing.Get_MaxCadenceScale());
    Settings.Get_Step().Set_Duration(Timing.Get_StepDuration())
        .Set_Threshold(Step.Get_Threshold());
    Settings.Get_Swing().Set_Height(Step.Get_Height())
        .Set_ObstacleClearance(Step.Get_ObstacleClearance());
    Settings.Get_Pattern().Set_LegLossPolicy(LegLossPolicy);
    Settings.Get_Reach().Set_TargetFraction(Step.Get_TargetReachFraction())
        .Set_ForceStepFraction(Step.Get_ForceStepReachFraction())
        .Set_HardOverstretchFraction(Step.Get_HardOverstretchReachFraction());

    return ck::FProceduralGaitBuiltSettings{Settings, ReachCadenceFloor, ReachSkippedLegs};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralGait_UE::
    DoFind_LegBeyondReach(
        const TArray<FCk_Handle_ProceduralLeg>& InLegs,
        const FCk_ProceduralGait_Step& InStep)
    -> FCk_Handle_ProceduralLeg
{
    for (const auto& Leg : InLegs)
    {
        if (ck::Is_NOT_Valid(Leg))
        { continue; }

        const auto& Params = Leg.Get<ck::FFragment_ProceduralLeg_Params>();
        if (NOT ck_procedural_gait_utils::Get_IsRestWithinReach(Params.Get_Placement(), Params.Get_Chain(), InStep))
        { return Leg; }
    }
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

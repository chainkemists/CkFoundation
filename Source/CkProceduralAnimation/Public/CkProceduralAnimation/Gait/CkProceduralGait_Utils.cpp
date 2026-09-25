#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

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
        && ck::IsValid(InData->Get_Probe());
    CK_ENSURE_IF_NOT(DataValid,
        TEXT("Procedural gait Add rejected body [{}]: the gait data asset is missing or its timing, step or probe settings are malformed."),
        InBody)
    { return {}; }

    const auto Legs = UCk_Utils_ProceduralLeg_UE::Get_Legs(InBody);
    const auto LegsValid = Legs.Num() >= 2 && Legs.Num() <= 64
        && InData->Get_Timing().Get_MaxSimultaneousSwings() <= Legs.Num();
    CK_ENSURE_IF_NOT(LegsValid,
        TEXT("Procedural gait Add rejected body [{}] with [{}] legs: create 2..64 legs before the gait and keep MaxSimultaneousSwings within the leg count."),
        InBody, Legs.Num())
    { return {}; }

    // Admission is atomic: nothing attaches until every authored field has been checked.
    auto Tunables = ck::FFragment_ProceduralGait_Tunables{InData->Get_Timing(), InData->Get_Step(), InData->Get_Probe()};
    auto GaitComp = ck::FFragment_ProceduralGait{};
    GaitComp._Solver.Set_Settings(DoBuild_SolverSettings(Tunables, Legs.Num()));
    GaitComp._Legs = Legs;
    GaitComp._Probes.SetNum(Legs.Num());

    auto DebugComp = ck::FFragment_ProceduralGait_Debug{};
    DebugComp._ScratchLegs.SetNum(Legs.Num());
    DebugComp._Snapshot.Get_Legs().Reserve(Legs.Num());

    InBody.Add<ck::FFragment_ProceduralGait_Tunables>(MoveTemp(Tunables));
    InBody.Add<ck::FFragment_ProceduralGait>(MoveTemp(GaitComp));
    InBody.Add<ck::FFragment_ProceduralGait_Debug>(MoveTemp(DebugComp));
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
        && InRequest.Get_Timing().Get_MaxSimultaneousSwings() <= InGait.Get<ck::FFragment_ProceduralGait>()._Legs.Num();
    CK_ENSURE_IF_NOT(RequestValid,
        TEXT("Procedural gait Request_ApplyPreset rejected gait [{}]: the gait must be live and the preset well-formed and within the leg count."),
        InGait)
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
        int32 InEnabledCount)
    -> ck::FProceduralGaitSettings
{
    const auto& Timing = InTunables.Get_Timing();
    const auto& Step = InTunables.Get_Step();

    const auto MaxSimultaneousSwings = Timing.Get_MaxSimultaneousSwings() == 0
        ? FMath::Max(1, InEnabledCount / 2)
        : Timing.Get_MaxSimultaneousSwings();

    const auto LegLossPolicy = Timing.Get_LegLossPolicy() == ECk_ProceduralGait_LegLossPolicy::RedistributeOffsets
        ? ck::EProceduralGaitLegLossPolicy::RedistributeOffsets
        : ck::EProceduralGaitLegLossPolicy::KeepAuthoredOffsets;

    auto Settings = ck::FProceduralGaitSettings{};
    Settings.Get_Cadence().Set_CycleDuration(Timing.Get_CycleDuration())
        .Set_MaxSimultaneousSwings(MaxSimultaneousSwings)
        .Set_CadenceSpeedRef(Timing.Get_CadenceSpeedRef())
        .Set_MaxCadenceScale(Timing.Get_MaxCadenceScale());
    Settings.Get_Step().Set_Duration(Timing.Get_StepDuration())
        .Set_Threshold(Step.Get_Threshold());
    Settings.Get_Swing().Set_Height(Step.Get_Height())
        .Set_ObstacleClearance(Step.Get_ObstacleClearance());
    Settings.Get_Pattern().Set_LegLossPolicy(LegLossPolicy);

    return Settings;
}

// --------------------------------------------------------------------------------------------------------------------

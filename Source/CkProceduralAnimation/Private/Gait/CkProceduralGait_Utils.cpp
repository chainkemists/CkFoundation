#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProceduralGait_UE, FCk_Handle_ProceduralGait,
    ck::FFragment_ProceduralGait_Params, ck::FFragment_ProceduralGait_Current);

auto
    UCk_Utils_ProceduralGait_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ProceduralGait_ParamsData& InParams)
    -> FCk_Handle_ProceduralGait
{
    const auto CompositionValid = ck::IsValid(InHandle) && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InHandle)
        && UCk_Utils_Transform_UE::Has(InHandle) && NOT Has(InHandle);
    CK_ENSURE_IF_NOT(CompositionValid, TEXT("Procedural gait needs a live transform entity with no existing gait feature."))
    { return {}; }
    if (NOT CompositionValid)
    { return {}; }

    auto Valid = InParams.Get_Legs().Num() >= 2 && InParams.Get_Legs().Num() <= 64
        && FMath::IsFinite(InParams.Get_ProbeUp()) && InParams.Get_ProbeUp() > 0.0f
        && FMath::IsFinite(InParams.Get_ProbeDown()) && InParams.Get_ProbeDown() > 0.0f
        && FMath::IsFinite(InParams.Get_ContactGrace().Get_Seconds()) && InParams.Get_ContactGrace() >= FCk_Time{}
        && FMath::IsFinite(InParams.Get_OutwardProbeLean()) && InParams.Get_OutwardProbeLean() >= 0.0f
        && InParams.Get_OutwardProbeLean() <= 1.0f
        && FMath::IsFinite(InParams.Get_MaxVelocityLead()) && InParams.Get_MaxVelocityLead() >= 0.0f
        && InParams.Get_MaxSimultaneousSwings() >= 0 && InParams.Get_MaxSimultaneousSwings() <= InParams.Get_Legs().Num();
    auto Ids = TSet<FName>{};
    for (const auto& Leg : InParams.Get_Legs())
    {
        Valid &= NOT Leg.Get_Id().IsNone() && NOT Ids.Contains(Leg.Get_Id())
            && NOT Leg.Get_HipLocal().ContainsNaN() && NOT Leg.Get_RestFootLocal().ContainsNaN()
            && FMath::IsFinite(Leg.Get_PhaseOffset()) && Leg.Get_PhaseOffset() >= 0.0f && Leg.Get_PhaseOffset() < 1.0f
            && FMath::IsFinite(Leg.Get_StepThresholdScale()) && Leg.Get_StepThresholdScale() > 0.0f;
        Ids.Add(Leg.Get_Id());
    }
    auto Settings = ck::FProceduralGaitSettings{};
    Settings.Set_CycleDuration(InParams.Get_CycleDuration()).Set_StepDuration(InParams.Get_StepDuration())
        .Set_StepHeight(InParams.Get_StepHeight()).Set_StepThreshold(InParams.Get_StepThreshold())
        .Set_MaxSimultaneousSwings(InParams.Get_MaxSimultaneousSwings() == 0
            ? FMath::Max(1, InParams.Get_Legs().Num() / 2) : InParams.Get_MaxSimultaneousSwings())
        .Set_CadenceSpeedRef(InParams.Get_CadenceSpeedRef()).Set_MaxCadenceScale(InParams.Get_MaxCadenceScale())
        .Set_ObstacleClearance(InParams.Get_ObstacleClearance());
    Valid &= ck::FProceduralGaitSolver::ValidateSettings(Settings);
    CK_ENSURE_IF_NOT(Valid, TEXT("Procedural gait admission rejected malformed legs, timing or probe configuration."))
    { return {}; }
    if (NOT Valid)
    { return {}; }

    // Admission is atomic: nothing attaches until every authored field has been checked.
    auto Current = ck::FFragment_ProceduralGait_Current{};
    Current._Solver.Set_Settings(Settings);
    Current._Probes.SetNum(InParams.Get_Legs().Num());
    Current._Inputs.SetNum(InParams.Get_Legs().Num());
    Current._Outputs.SetNum(InParams.Get_Legs().Num());
    Current._Feet.SetNum(InParams.Get_Legs().Num());
    Current._DebugScratchLegs.SetNum(InParams.Get_Legs().Num());
    Current._DebugSnapshot.Get_Legs().Reserve(InParams.Get_Legs().Num());
    InHandle.Add<ck::FFragment_ProceduralGait_Params>(InParams);
    InHandle.Add<ck::FFragment_ProceduralGait_Current>(MoveTemp(Current));
    return CastChecked(InHandle);
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_IsReady(
        const FCk_Handle_ProceduralGait& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && Has(InHandle) && InHandle.Get<ck::FFragment_ProceduralGait_Current>()._Ready;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_HasFailed(
        const FCk_Handle_ProceduralGait& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && Has(InHandle) && InHandle.Get<ck::FFragment_ProceduralGait_Current>()._Failed;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_TrustedContactCount(
        const FCk_Handle_ProceduralGait& InHandle)
    -> int32
{
    if (NOT Get_IsReady(InHandle))
    { return 0; }
    auto Count = 0;
    for (const auto& Foot : InHandle.Get<ck::FFragment_ProceduralGait_Current>()._Feet)
    { Count += Foot.Get_ContactTrusted() ? 1 : 0; }
    return Count;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_PlantedCount(
        const FCk_Handle_ProceduralGait& InHandle)
    -> int32
{
    if (NOT Get_IsReady(InHandle))
    { return 0; }
    auto Count = 0;
    for (const auto& Foot : InHandle.Get<ck::FFragment_ProceduralGait_Current>()._Feet)
    { Count += Foot.Get_Planted() ? 1 : 0; }
    return Count;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_Feet(
        const FCk_Handle_ProceduralGait& InHandle)
    -> TArray<FCk_ProceduralGait_Foot>
{
    return Get_IsReady(InHandle) ? InHandle.Get<ck::FFragment_ProceduralGait_Current>()._Feet : TArray<FCk_ProceduralGait_Foot>{};
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_GaitClock(
        const FCk_Handle_ProceduralGait& InHandle)
    -> float
{
    return Get_IsReady(InHandle) ? InHandle.Get<ck::FFragment_ProceduralGait_Current>()._Solver.GetGaitClock() : 0.0f;
}

auto
    UCk_Utils_ProceduralGait_UE::
    Get_Legs(
        const FCk_Handle_ProceduralGait& InHandle)
    -> TArray<FCk_ProceduralGait_Leg>
{
    return ck::IsValid(InHandle) && Has(InHandle) ? InHandle.Get<ck::FFragment_ProceduralGait_Params>().Get_Legs() : TArray<FCk_ProceduralGait_Leg>{};
}

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h"

#include "CkCore/Validation/CkIsValid.h"

auto UCk_Utils_CrowdAvoidanceVolume_UE::Add(
    FCk_Handle_Transform& InOwner,
    const FCk_Fragment_CrowdAvoidanceVolume_ParamsData& InParams)
    -> FCk_Handle_CrowdAvoidanceVolume
{
    const auto& HalfExtents = InParams.Get_HalfExtents();
    const auto InfluenceRange = InParams.Get_InfluenceRange();
    const auto PathPlanningClearance = InParams.Get_PathPlanningClearance();
    const auto HasFiniteProbeExtents =
        FMath::IsFinite(HalfExtents.X + InfluenceRange) &&
        FMath::IsFinite(HalfExtents.Y + InfluenceRange);
    const auto IsValidInput = ck::IsValid(InOwner) &&
        NOT InOwner.Has<ck::FFragment_CrowdAvoidanceVolume_Params>() &&
        FMath::IsFinite(HalfExtents.X) && FMath::IsFinite(HalfExtents.Y) && FMath::IsFinite(HalfExtents.Z) &&
        HalfExtents.X > 0.0 && HalfExtents.Y > 0.0 && HalfExtents.Z > 0.0 &&
        FMath::IsFinite(InfluenceRange) && InfluenceRange >= 0.0f &&
        FMath::IsFinite(PathPlanningClearance) && PathPlanningClearance >= 0.0f &&
        HasFiniteProbeExtents;
    CK_ENSURE_IF_NOT(IsValidInput,
        TEXT("CrowdAvoidanceVolume Add requires a valid Transform owner, unique feature, positive half extents, and non-negative influence and path clearances."))
    { }
    if (NOT IsValidInput)
    { return {}; }

    InOwner.Add<ck::FFragment_CrowdAvoidanceVolume_Params>(InParams);
    InOwner.Add<ck::FFragment_CrowdAvoidanceVolume_ProbeRef>();
    InOwner.Add<ck::FTag_CrowdAvoidanceVolume_NeedsSetup>();
    return Cast(InOwner);
}

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CrowdAvoidanceVolume_UE, FCk_Handle_CrowdAvoidanceVolume,
    ck::FFragment_CrowdAvoidanceVolume_Params)

auto UCk_Utils_CrowdAvoidanceVolume_UE::Get_IsNavigationConfirmed(
    const FCk_Handle_CrowdAvoidanceVolume& InVolume) -> bool
{
    const auto VolumeIsValid = ck::IsValid(InVolume);
    CK_ENSURE_IF_NOT(VolumeIsValid,
        TEXT("Invalid CrowdAvoidanceVolume handle [{}] passed to Get_IsNavigationConfirmed"),
        InVolume)
    { }
    if (NOT VolumeIsValid ||
        NOT InVolume.Has<ck::FFragment_CrowdAvoidanceVolume_ProbeRef>())
    { return false; }

    const auto& Runtime = InVolume.Get<ck::FFragment_CrowdAvoidanceVolume_ProbeRef>();
    return Runtime.Get_Markup().IsValid() && Runtime.Get_ConfirmedOnMesh();
}

auto UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay() -> FCk_Nav_QueryFilterOverlay
{
    auto Result = FCk_Nav_QueryFilterOverlay{};
    auto ExcludedAreas = TArray<TSubclassOf<UNavArea>>{};
    ExcludedAreas.Add(UCk_NavArea_CrowdAvoidanceVolume::StaticClass());
    Result.Set_ExcludedAreaClasses(MoveTemp(ExcludedAreas));
    return Result;
}

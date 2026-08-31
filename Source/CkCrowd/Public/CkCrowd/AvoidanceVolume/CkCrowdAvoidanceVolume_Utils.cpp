#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h"
#include "CkCrowd/CkCrowd_NavGameplayTags.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

namespace ck_crowd_avoidance_volume
{
    auto Get_DebugState(
        const FCk_Handle& InVolume,
        const ck::FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
        -> ECk_CrowdAvoidanceVolume_DebugState
    {
        if (InVolume.Has<ck::FTag_CrowdAvoidanceVolume_Invalid>())
        { return ECk_CrowdAvoidanceVolume_DebugState::Invalid; }
        if (InVolume.Has<ck::FTag_CrowdAvoidanceVolume_NeedsSetup>())
        { return ECk_CrowdAvoidanceVolume_DebugState::PendingSetup; }
        if (InRuntime.Get_Markup().IsValid() && InRuntime.Get_ConfirmedOnMesh())
        { return ECk_CrowdAvoidanceVolume_DebugState::Confirmed; }
        return ECk_CrowdAvoidanceVolume_DebugState::PendingNavigationConfirmation;
    }

    auto Get_HasFiniteNonNegativeExtents(const FVector& InHalfExtents) -> bool
    {
        return FMath::IsFinite(InHalfExtents.X) && FMath::IsFinite(InHalfExtents.Y) &&
            FMath::IsFinite(InHalfExtents.Z) &&
            InHalfExtents.X >= 0.0f && InHalfExtents.Y >= 0.0f && InHalfExtents.Z >= 0.0f;
    }

    auto Get_IsValidTraversalPolicy(ECk_CrowdAvoidanceVolume_TraversalPolicy InTraversalPolicy) -> bool
    {
        switch (InTraversalPolicy)
        {
            case ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible:
            case ECk_CrowdAvoidanceVolume_TraversalPolicy::HardExclude:
            case ECk_CrowdAvoidanceVolume_TraversalPolicy::CostOnly:
                return true;
            default:
                return false;
        }
    }
}

auto UCk_Utils_CrowdAvoidanceVolume_UE::Add(
    FCk_Handle_Transform& InOwner,
    const FCk_Fragment_CrowdAvoidanceVolume_ParamsData& InParams)
    -> FCk_Handle_CrowdAvoidanceVolume
{
    const auto& HalfExtents = InParams.Get_HalfExtents();
    const auto InfluenceRange = InParams.Get_InfluenceRange();
    const auto PathPlanningClearance = InParams.Get_PathPlanningClearance();
    const auto TraversalPolicy = InParams.Get_TraversalPolicy();
    const auto HasFiniteProbeExtents =
        FMath::IsFinite(HalfExtents.X + InfluenceRange) &&
        FMath::IsFinite(HalfExtents.Y + InfluenceRange);
    const auto IsValidInput = ck::IsValid(InOwner) &&
        NOT InOwner.Has<ck::FFragment_CrowdAvoidanceVolume_Params>() &&
        FMath::IsFinite(HalfExtents.X) && FMath::IsFinite(HalfExtents.Y) && FMath::IsFinite(HalfExtents.Z) &&
        HalfExtents.X > 0.0 && HalfExtents.Y > 0.0 && HalfExtents.Z > 0.0 &&
        FMath::IsFinite(InfluenceRange) && InfluenceRange >= 0.0f &&
        FMath::IsFinite(PathPlanningClearance) && PathPlanningClearance >= 0.0f &&
        HasFiniteProbeExtents &&
        ck_crowd_avoidance_volume::Get_IsValidTraversalPolicy(TraversalPolicy);
    CK_ENSURE_IF_NOT(IsValidInput,
        TEXT("CrowdAvoidanceVolume Add requires a valid Transform owner, unique feature, positive half extents, non-negative influence and path clearances, and a valid traversal policy."))
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

auto UCk_Utils_CrowdAvoidanceVolume_UE::Get_DebugSnapshots(
    const FCk_Handle& InAnyEntityInWorld) -> TArray<FCk_CrowdAvoidanceVolume_DebugSnapshot>
{
    // Debug consumers race normal PIE teardown. An invalid selector means the world is gone, not a malformed
    // gameplay request, so fail closed without turning expected teardown into an ensure.
    if (ck::Is_NOT_Valid(InAnyEntityInWorld))
    { return {}; }

    const auto Registry = InAnyEntityInWorld.Get_RegistryView();
    auto Result = TArray<FCk_CrowdAvoidanceVolume_DebugSnapshot>{};
    Registry.View<
        ck::FFragment_CrowdAvoidanceVolume_Params,
        ck::FFragment_CrowdAvoidanceVolume_ProbeRef,
        CK_IGNORE_PENDING_KILL>().ForEach(
        [&Result, Registry](
            FCk_Entity InVolumeEntity,
            const ck::FFragment_CrowdAvoidanceVolume_Params& InParams,
            const ck::FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
        {
            const auto Volume = FCk_Handle{InVolumeEntity, Registry.Get_RegistryHandle()};
            const auto VolumeTransform = UCk_Utils_Transform_UE::Cast(Volume);
            const auto HasTransform = ck::IsValid(VolumeTransform);
            const auto FallbackPhysical = HasTransform
                ? ck::crowd_avoidance_volume::MakeObb(
                    UCk_Utils_Transform_UE::Get_EntityCurrentTransform(VolumeTransform),
                    InParams.Get_HalfExtents())
                : ck::crowd_avoidance_volume::FCk_Obb{};
            const auto& RuntimePhysical = InRuntime.Get_AuthoredObb();
            const auto& RuntimePainted = InRuntime.Get_PaintedObb();
            const auto Physical = RuntimePhysical.IsFiniteAndPositive() ? RuntimePhysical : FallbackPhysical;
            const auto Painted = RuntimePainted.IsFiniteAndPositive()
                ? RuntimePainted
                : Physical.ExpandedXY(InParams.Get_PathPlanningClearance());
            const auto HasValidGeometry = Physical.IsFiniteAndPositive() && Painted.IsFiniteAndPositive();
            const auto InfluenceRange = InParams.Get_InfluenceRange();
            const auto HasValidInfluence = FMath::IsFinite(InfluenceRange) && InfluenceRange >= 0.0f &&
                FMath::IsFinite(Physical._WorldHalfExtents.X + InfluenceRange) &&
                FMath::IsFinite(Physical._WorldHalfExtents.Y + InfluenceRange);
            const auto InfluenceHalfExtents = HasValidGeometry && HasValidInfluence
                ? FVector{
                    Physical._WorldHalfExtents.X + InfluenceRange,
                    Physical._WorldHalfExtents.Y + InfluenceRange,
                    Physical._WorldHalfExtents.Z}
                : FVector::ZeroVector;
            const auto GeometryIsSafeToPublish = HasValidGeometry &&
                ck_crowd_avoidance_volume::Get_HasFiniteNonNegativeExtents(InfluenceHalfExtents);

            Result.Emplace(
                static_cast<int64>(InVolumeEntity.Get_ID()),
                Volume.Get_DebugName(),
                GeometryIsSafeToPublish ? Physical._YawTransform : FTransform::Identity,
                GeometryIsSafeToPublish ? Physical._WorldHalfExtents : FVector::ZeroVector,
                InfluenceHalfExtents,
                GeometryIsSafeToPublish ? Painted._WorldHalfExtents : FVector::ZeroVector,
                InRuntime.Get_SecondsSincePaint(),
                static_cast<int64>(InRuntime.Get_ConfirmationSerial()),
                0,
                ck_crowd_avoidance_volume::Get_DebugState(Volume, InRuntime),
                GeometryIsSafeToPublish,
                InParams.Get_TraversalPolicy());
        });

    Registry.View<ck::FFragment_CrowdAvoidanceVolume_Retirements>().ForEach(
        [&Result](FCk_Entity, const ck::FFragment_CrowdAvoidanceVolume_Retirements& InRetirements)
        {
            for (const auto& Retirement : InRetirements.Get_Records())
            {
                const auto HasValidGeometry = Retirement._PhysicalObb.IsFiniteAndPositive() &&
                    Retirement._PaintedObb.IsFiniteAndPositive();
                Result.Emplace(
                    Retirement._VolumeIdentity,
                    Retirement._VolumeDebugName,
                    HasValidGeometry ? Retirement._YawWorldTransform : FTransform::Identity,
                    HasValidGeometry ? Retirement._PhysicalObb._WorldHalfExtents : FVector::ZeroVector,
                    FVector::ZeroVector,
                    HasValidGeometry ? Retirement._PaintedObb._WorldHalfExtents : FVector::ZeroVector,
                    0.0f,
                    static_cast<int64>(Retirement._ConfirmationSerial),
                    static_cast<int64>(Retirement._NavigationRevisionAtUnregister),
                    ECk_CrowdAvoidanceVolume_DebugState::Retiring,
                    HasValidGeometry,
                    Retirement._TraversalPolicy);
            }
        });

    Result.Sort([](
        const FCk_CrowdAvoidanceVolume_DebugSnapshot& InLeft,
        const FCk_CrowdAvoidanceVolume_DebugSnapshot& InRight)
    {
        if (InLeft.Get_VolumeIdentity() != InRight.Get_VolumeIdentity())
        { return InLeft.Get_VolumeIdentity() < InRight.Get_VolumeIdentity(); }
        return static_cast<uint8>(InLeft.Get_State()) < static_cast<uint8>(InRight.Get_State());
    });
    return Result;
}

auto UCk_Utils_CrowdAvoidanceVolume_UE::Get_HasAvoidIfPossibleVolumes(
    const FCk_Handle& InAnyEntityInWorld) -> bool
{
    if (ck::Is_NOT_Valid(InAnyEntityInWorld))
    { return false; }

    const auto Registry = InAnyEntityInWorld.Get_RegistryView();
    auto HasAvoidIfPossibleVolume = false;
    Registry.View<
        ck::FFragment_CrowdAvoidanceVolume_ProbeRef,
        CK_IGNORE_PENDING_KILL>().ForEach(
        [&HasAvoidIfPossibleVolume](
            FCk_Entity,
            const ck::FFragment_CrowdAvoidanceVolume_ProbeRef& InRuntime)
        {
            HasAvoidIfPossibleVolume = HasAvoidIfPossibleVolume ||
                (InRuntime.Get_TraversalPolicy() ==
                    ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible &&
                InRuntime.Get_Markup().IsValid());
        });
    return HasAvoidIfPossibleVolume;
}

auto UCk_Utils_CrowdAvoidanceVolume_UE::Get_NavQueryFilterOverlay(
    ECk_CrowdAvoidanceVolume_QueryPhase InPhase) -> FCk_Nav_QueryFilterOverlay
{
    auto Result = FCk_Nav_QueryFilterOverlay{};
    auto ExcludedAreas = TArray<FGameplayTag>{};
    if (Get_IsTraversalPolicyExcluded(
        ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible, InPhase))
    {
        ExcludedAreas.Add(TAG_Nav_Area_Crowd_AvoidanceVolume);
    }
    if (Get_IsTraversalPolicyExcluded(
        ECk_CrowdAvoidanceVolume_TraversalPolicy::HardExclude, InPhase))
    {
        ExcludedAreas.Add(TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude);
    }
    if (Get_IsTraversalPolicyExcluded(
        ECk_CrowdAvoidanceVolume_TraversalPolicy::CostOnly, InPhase))
    {
        ExcludedAreas.Add(TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly);
    }
    Result.Set_ExcludedAreaTags(MoveTemp(ExcludedAreas));
    return Result;
}

auto UCk_Utils_CrowdAvoidanceVolume_UE::Get_IsTraversalPolicyExcluded(
    ECk_CrowdAvoidanceVolume_TraversalPolicy InTraversalPolicy,
    ECk_CrowdAvoidanceVolume_QueryPhase InPhase) -> bool
{
    switch (InTraversalPolicy)
    {
        case ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible:
            return InPhase != ECk_CrowdAvoidanceVolume_QueryPhase::Permissive;
        case ECk_CrowdAvoidanceVolume_TraversalPolicy::HardExclude:
            return true;
        case ECk_CrowdAvoidanceVolume_TraversalPolicy::CostOnly:
            return false;
        default:
            return true;
    }
}

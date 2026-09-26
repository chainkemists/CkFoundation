#include "CkProceduralAnimation/Core/CkProceduralBodySupport.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_body_support
{
    auto
        Get_IsLegValid(
            const ck::FProceduralBodySupportLeg& InLeg)
        -> bool
    {
        return NOT InLeg.Get_HipLocal().ContainsNaN()
            && FMath::IsFinite(InLeg.Get_Weight())
            && InLeg.Get_Weight() >= 0.0f
            && InLeg.Get_Weight() <= 1.0f;
    }

    auto
        Get_AreSettingsValid(
            const ck::FProceduralBodySupportSettings& InSettings)
        -> bool
    {
        return FMath::IsFinite(InSettings.Get_CollapseDrop())
            && InSettings.Get_CollapseDrop() >= 0.0f
            && FMath::IsFinite(InSettings.Get_MaxTiltDegrees())
            && InSettings.Get_MaxTiltDegrees() >= 0.0f
            && InSettings.Get_MaxTiltDegrees() <= 89.0f;
    }

    auto
        Get_Planar(
            const FVector& InHip)
        -> FVector
    {
        return FVector{InHip.X, InHip.Y, 0.0};
    }

    // Weighted feet whose planar spread has eigenvalues l1 and l2 lie along one line when l1 l2 / (l1 + l2)^2 falls to this.
    constexpr auto CollinearSpread = 0.02;
    constexpr auto MinWeightedFeet = 3;

    auto
        Get_IsFootValid(
            const ck::FProceduralBodyConformFoot& InFoot)
        -> bool
    {
        return NOT InFoot.Get_PositionLocal().ContainsNaN()
            && NOT InFoot.Get_RestLocal().ContainsNaN()
            && FMath::IsFinite(InFoot.Get_Weight())
            && InFoot.Get_Weight() >= 0.0f;
    }

    auto
        Get_AreConformSettingsValid(
            const ck::FProceduralBodyConformSettings& InSettings)
        -> bool
    {
        return FMath::IsFinite(InSettings.Get_MaxTiltDegrees())
            && InSettings.Get_MaxTiltDegrees() >= 0.0f
            && InSettings.Get_MaxTiltDegrees() <= 89.0f
            && FMath::IsFinite(InSettings.Get_HeightWeight())
            && InSettings.Get_HeightWeight() >= 0.0f
            && InSettings.Get_HeightWeight() <= 1.0f
            && FMath::IsFinite(InSettings.Get_MaxHeight())
            && InSettings.Get_MaxHeight() >= 0.0f;
    }

    auto
        Get_AreSlewSettingsValid(
            const ck::FProceduralBodyConformSlewSettings& InSettings)
        -> bool
    {
        return FMath::IsFinite(InSettings.Get_MaxTiltRateDegrees())
            && InSettings.Get_MaxTiltRateDegrees() > 0.0f
            && FMath::IsFinite(InSettings.Get_MaxHeightRate())
            && InSettings.Get_MaxHeightRate() > 0.0f;
    }

    auto
        Get_FitPoint(
            const ck::FProceduralBodyConformFoot& InFoot)
        -> FVector
    {
        return FVector{InFoot.Get_PositionLocal().X, InFoot.Get_PositionLocal().Y, InFoot.Get_PositionLocal().Z - InFoot.Get_RestLocal().Z};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        ComputeProceduralBodySupportPose(
            TArrayView<const FProceduralBodySupportLeg> InLegs,
            const FProceduralBodySupportSettings& InSettings)
        -> TOptional<FTransform>
    {
        if (InLegs.IsEmpty()
            || NOT ck_procedural_body_support::Get_AreSettingsValid(InSettings)
            || NOT algo::AllOf(InLegs, &ck_procedural_body_support::Get_IsLegValid))
        { return {}; }

        const auto LegCount = static_cast<double>(InLegs.Num());
        auto WeightSum = 0.0;
        auto HipSum = FVector::ZeroVector;
        auto WeightedHipSum = FVector::ZeroVector;
        for (const auto& Leg : InLegs)
        {
            const auto Hip = ck_procedural_body_support::Get_Planar(Leg.Get_HipLocal());
            WeightSum += Leg.Get_Weight();
            HipSum += Hip;
            WeightedHipSum += Hip * Leg.Get_Weight();
        }

        const auto SupportFraction = WeightSum / LegCount;
        const auto Drop = FVector{0.0, 0.0, -InSettings.Get_CollapseDrop() * (1.0 - SupportFraction)};

        const auto CentroidAll = HipSum / LegCount;
        const auto CentroidSupport = WeightSum > 0.0 ? WeightedHipSum / WeightSum : CentroidAll;
        const auto Gap = CentroidAll - CentroidSupport;

        auto Extent = 0.0;
        for (const auto& Leg : InLegs)
        { Extent = FMath::Max(Extent, (ck_procedural_body_support::Get_Planar(Leg.Get_HipLocal()) - CentroidAll).Size()); }

        if (Extent <= KINDA_SMALL_NUMBER || Gap.IsNearlyZero())
        { return FTransform{FQuat::Identity, Drop}; }

        // Up x gap turns a positive angle into a dip on the side the gap points at: the unsupported side.
        const auto Angle = FMath::DegreesToRadians(static_cast<double>(InSettings.Get_MaxTiltDegrees()))
            * FMath::Clamp(Gap.Size() / Extent, 0.0, 1.0);
        const auto Axis = FVector::CrossProduct(FVector::UpVector, Gap.GetSafeNormal()).GetSafeNormal();

        return FTransform{FQuat{Axis, Angle}, Drop};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        ComputeProceduralBodyConformPose(
            TArrayView<const FProceduralBodyConformFoot> InFeet,
            const FProceduralBodyConformSettings& InSettings,
            FTransform& OutTarget)
        -> EProceduralBodyConformResult
    {
        if (NOT ck_procedural_body_support::Get_AreConformSettingsValid(InSettings)
            || NOT algo::AllOf(InFeet, &ck_procedural_body_support::Get_IsFootValid))
        { return EProceduralBodyConformResult::Malformed; }

        const auto WeightedFeet = algo::CountIf(InFeet, [](const FProceduralBodyConformFoot& InFoot) -> bool
        {
            return InFoot.Get_Weight() > 0.0f;
        });
        if (WeightedFeet < ck_procedural_body_support::MinWeightedFeet)
        { return EProceduralBodyConformResult::Underdetermined; }

        auto WeightSum = 0.0;
        auto Mean = FVector::ZeroVector;
        for (const auto& Foot : InFeet)
        {
            WeightSum += Foot.Get_Weight();
            Mean += ck_procedural_body_support::Get_FitPoint(Foot) * Foot.Get_Weight();
        }
        Mean /= WeightSum;

        auto Sxx = 0.0;
        auto Sxy = 0.0;
        auto Syy = 0.0;
        auto Sxz = 0.0;
        auto Syz = 0.0;
        for (const auto& Foot : InFeet)
        {
            const auto Weight = static_cast<double>(Foot.Get_Weight());
            const auto Delta = ck_procedural_body_support::Get_FitPoint(Foot) - Mean;
            Sxx += Weight * Delta.X * Delta.X;
            Sxy += Weight * Delta.X * Delta.Y;
            Syy += Weight * Delta.Y * Delta.Y;
            Sxz += Weight * Delta.X * Delta.Z;
            Syz += Weight * Delta.Y * Delta.Z;
        }

        const auto Determinant = Sxx * Syy - Sxy * Sxy;
        const auto Spread = Sxx + Syy;
        if (Spread <= UE_DOUBLE_SMALL_NUMBER || Determinant <= ck_procedural_body_support::CollinearSpread * Spread * Spread)
        { return EProceduralBodyConformResult::Underdetermined; }

        const auto SlopeX = (Syy * Sxz - Sxy * Syz) / Determinant;
        const auto SlopeY = (Sxx * Syz - Sxy * Sxz) / Determinant;
        const auto HeightAtOrigin = Mean.Z - SlopeX * Mean.X - SlopeY * Mean.Y;

        const auto Normal = FVector{-SlopeX, -SlopeY, 1.0}.GetSafeNormal();
        const auto Axis = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
        const auto Tilt = FMath::Min(FMath::Acos(FMath::Clamp(Normal.Z, -1.0, 1.0)),
            FMath::DegreesToRadians(static_cast<double>(InSettings.Get_MaxTiltDegrees())));
        const auto Rotation = Axis.IsNearlyZero() ? FQuat::Identity : FQuat{Axis, Tilt};

        const auto MaxHeight = static_cast<double>(InSettings.Get_MaxHeight());
        const auto Height = FMath::Clamp(HeightAtOrigin * InSettings.Get_HeightWeight(), -MaxHeight, MaxHeight);

        OutTarget = FTransform{Rotation, FVector{0.0, 0.0, Height}};
        return EProceduralBodyConformResult::Fitted;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        SlewProceduralBodyConformPose(
            const FTransform& InApplied,
            const FTransform& InTarget,
            const FProceduralBodyConformSlewSettings& InSettings,
            FCk_Time InDeltaTime)
        -> TOptional<FTransform>
    {
        if (NOT ck_procedural_body_support::Get_AreSlewSettingsValid(InSettings)
            || InApplied.ContainsNaN() || InTarget.ContainsNaN()
            || NOT FMath::IsFinite(InDeltaTime.Get_Seconds()) || InDeltaTime < FCk_Time{})
        { return {}; }

        const auto DeltaSeconds = InDeltaTime.Get_Seconds();
        const auto MaxTurn = FMath::DegreesToRadians(static_cast<double>(InSettings.Get_MaxTiltRateDegrees())) * DeltaSeconds;
        const auto Turn = InApplied.GetRotation().AngularDistance(InTarget.GetRotation());
        const auto Rotation = Turn <= MaxTurn
            ? InTarget.GetRotation()
            : FQuat::Slerp(InApplied.GetRotation(), InTarget.GetRotation(), MaxTurn / Turn).GetNormalized();

        const auto MaxMove = InSettings.Get_MaxHeightRate() * DeltaSeconds;
        const auto Move = InTarget.GetLocation() - InApplied.GetLocation();
        const auto Location = Move.Size() <= MaxMove
            ? InTarget.GetLocation()
            : InApplied.GetLocation() + Move.GetSafeNormal() * MaxMove;

        return FTransform{Rotation, Location};
    }
}

// --------------------------------------------------------------------------------------------------------------------

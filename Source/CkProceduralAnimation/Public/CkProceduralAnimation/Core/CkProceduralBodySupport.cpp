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
}

// --------------------------------------------------------------------------------------------------------------------

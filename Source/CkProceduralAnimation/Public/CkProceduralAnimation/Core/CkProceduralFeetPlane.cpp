#include "CkProceduralAnimation/Core/CkProceduralFeetPlane.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_feet_plane
{
    // Weighted feet whose planar spread has eigenvalues l1 and l2 lie along one line when l1 l2 / (l1 + l2)^2 falls to this.
    constexpr auto CollinearSpread = 0.02;
    constexpr auto MinWeightedFeet = 3;

    auto
        Get_IsFootValid(
            const ck::FProceduralFeetPlaneFoot& InFoot)
        -> bool
    {
        return NOT InFoot.Get_Position().ContainsNaN()
            && FMath::IsFinite(InFoot.Get_Weight())
            && InFoot.Get_Weight() >= 0.0f;
    }

    auto
        Get_IsMaxAngleValid(
            float InMaxAngleDegrees)
        -> bool
    {
        return FMath::IsFinite(InMaxAngleDegrees) && InMaxAngleDegrees >= 0.0f && InMaxAngleDegrees <= 90.0f;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FitProceduralFeetPlane(
            TArrayView<const FProceduralFeetPlaneFoot> InFeet,
            float InMaxAngleDegrees,
            FProceduralFeetPlane& OutPlane)
        -> EProceduralFeetPlaneResult
    {
        if (NOT ck_procedural_feet_plane::Get_IsMaxAngleValid(InMaxAngleDegrees)
            || NOT algo::AllOf(InFeet, &ck_procedural_feet_plane::Get_IsFootValid))
        { return EProceduralFeetPlaneResult::Malformed; }

        const auto WeightedFeet = algo::CountIf(InFeet, [](const FProceduralFeetPlaneFoot& InFoot) -> bool
        {
            return InFoot.Get_Weight() > 0.0f;
        });
        if (WeightedFeet < ck_procedural_feet_plane::MinWeightedFeet)
        { return EProceduralFeetPlaneResult::Underdetermined; }

        auto WeightSum = 0.0;
        auto Mean = FVector::ZeroVector;
        for (const auto& Foot : InFeet)
        {
            WeightSum += Foot.Get_Weight();
            Mean += Foot.Get_Position() * Foot.Get_Weight();
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
            const auto Delta = Foot.Get_Position() - Mean;
            Sxx += Weight * Delta.X * Delta.X;
            Sxy += Weight * Delta.X * Delta.Y;
            Syy += Weight * Delta.Y * Delta.Y;
            Sxz += Weight * Delta.X * Delta.Z;
            Syz += Weight * Delta.Y * Delta.Z;
        }

        const auto Determinant = Sxx * Syy - Sxy * Sxy;
        const auto Spread = Sxx + Syy;
        if (Spread <= UE_DOUBLE_SMALL_NUMBER || Determinant <= ck_procedural_feet_plane::CollinearSpread * Spread * Spread)
        { return EProceduralFeetPlaneResult::Underdetermined; }

        const auto SlopeX = (Syy * Sxz - Sxy * Syz) / Determinant;
        const auto SlopeY = (Sxx * Syz - Sxy * Sxz) / Determinant;
        const auto HeightAtOrigin = Mean.Z - SlopeX * Mean.X - SlopeY * Mean.Y;

        const auto Normal = FVector{-SlopeX, -SlopeY, 1.0}.GetSafeNormal();
        const auto AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Normal.Z, -1.0, 1.0)));
        if (AngleDegrees > InMaxAngleDegrees)
        { return EProceduralFeetPlaneResult::TooSteep; }

        OutPlane = FProceduralFeetPlane{}.Set_Point(FVector{0.0, 0.0, HeightAtOrigin}).Set_Normal(Normal);
        return EProceduralFeetPlaneResult::Fitted;
    }
}

// --------------------------------------------------------------------------------------------------------------------

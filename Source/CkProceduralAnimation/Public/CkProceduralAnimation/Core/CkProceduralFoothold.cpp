#include "CkProceduralAnimation/Core/CkProceduralFoothold.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_foothold
{
    // A foothold shallower than this share of the leg's rest drop below its hip stands beside the body, not under it.
    constexpr auto UnderBodyDepthShareOfRestDrop = 0.25;

    // A face exactly perpendicular to the support up (a vertical wall under a 90 degree limit) must pass the level gate
    // whatever the rounding of its normal.
    constexpr auto LevelCosineTolerance = 1.0e-4;

    auto
        Get_IsCandidateFinite(
            const ck::FProceduralFootholdCandidate& InCandidate)
        -> bool
    {
        return NOT InCandidate.Get_Position().ContainsNaN() && NOT InCandidate.Get_Normal().ContainsNaN();
    }

    auto
        Get_AreSettingsFinite(
            const ck::FProceduralFootholdSettings& InSettings)
        -> bool
    {
        return FMath::IsFinite(InSettings.Get_SlopeWeight())
            && FMath::IsFinite(InSettings.Get_ContinuityWeight())
            && FMath::IsFinite(InSettings.Get_MaxAngleDegrees());
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        Get_IsFootholdLevelEnough(
            const FVector& InNormal,
            float InMaxAngleDegrees)
        -> bool
    {
        if (InNormal.ContainsNaN() || NOT FMath::IsFinite(InMaxAngleDegrees))
        { return false; }

        const auto Normal = InNormal.GetSafeNormal();
        if (Normal.IsNearlyZero())
        { return false; }

        const auto MinCosine = FMath::Cos(FMath::DegreesToRadians(static_cast<double>(InMaxAngleDegrees)));
        return Normal.Z >= MinCosine - ck_procedural_foothold::LevelCosineTolerance;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Get_IsFootholdOnOwnSide(
            const FVector& InHipToPoint,
            const FVector& InOutboard,
            float InTolerance)
        -> bool
    {
        if (InHipToPoint.ContainsNaN() || NOT FMath::IsFinite(InTolerance))
        { return false; }

        const auto Outboard = InOutboard.GetSafeNormal();
        if (Outboard.IsNearlyZero())
        { return true; }

        return FVector::DotProduct(InHipToPoint, Outboard) >= -static_cast<double>(InTolerance);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Get_IsFootholdUnderBody(
            float InLateralOffset,
            float InDepthBelowHip,
            float InMaxHipLateral,
            float InRestDrop,
            float InMargin)
        -> bool
    {
        const auto InputIsFinite = FMath::IsFinite(InLateralOffset) && FMath::IsFinite(InDepthBelowHip)
            && FMath::IsFinite(InMaxHipLateral) && FMath::IsFinite(InRestDrop) && FMath::IsFinite(InMargin);
        if (NOT InputIsFinite || InMaxHipLateral <= 0.0f)
        { return false; }

        return FMath::Abs(InLateralOffset) < InMaxHipLateral - InMargin
            && InDepthBelowHip > ck_procedural_foothold::UnderBodyDepthShareOfRestDrop * InRestDrop;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        ComputeProceduralFootholdCost(
            const FProceduralFootholdCandidate& InCandidate,
            const FVector& InIdeal,
            const FVector& InPlant,
            bool InPlanted,
            float InReach,
            const FProceduralFootholdSettings& InSettings)
        -> double
    {
        if (NOT FMath::IsFinite(InReach) || InReach <= 0.0f)
        { return TNumericLimits<double>::Max(); }

        const auto Reach = static_cast<double>(InReach);
        const auto Distance = FVector::Dist(InCandidate.Get_Position(), InIdeal) / Reach;
        const auto Slope = InSettings.Get_SlopeWeight() * (1.0 - InCandidate.Get_Normal().GetSafeNormal().Z);
        const auto Continuity = InPlanted
            ? InSettings.Get_ContinuityWeight() * FMath::Abs(InCandidate.Get_Position().Z - InPlant.Z) / Reach
            : 0.0;

        return Distance + Slope + Continuity;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        SelectProceduralFoothold(
            TArrayView<const FProceduralFootholdCandidate> InCandidates,
            const FVector& InIdeal,
            const FVector& InPlant,
            bool InPlanted,
            float InReach,
            const FProceduralFootholdSettings& InSettings)
        -> int32
    {
        const auto InputIsWellFormed = NOT InCandidates.IsEmpty()
            && FMath::IsFinite(InReach) && InReach > 0.0f
            && NOT InIdeal.ContainsNaN() && NOT InPlant.ContainsNaN()
            && ck_procedural_foothold::Get_AreSettingsFinite(InSettings)
            && algo::AllOf(InCandidates, &ck_procedural_foothold::Get_IsCandidateFinite);
        if (NOT InputIsWellFormed)
        { return INDEX_NONE; }

        auto Chosen = int32{INDEX_NONE};
        auto ChosenCost = TNumericLimits<double>::Max();
        for (auto Index = 0; Index < InCandidates.Num(); ++Index)
        {
            const auto& Candidate = InCandidates[Index];
            if (Candidate.Get_Verdict() != EProceduralFootholdVerdict::Usable)
            { continue; }

            const auto Cost = ComputeProceduralFootholdCost(Candidate, InIdeal, InPlant, InPlanted, InReach, InSettings);
            if (Chosen == INDEX_NONE || Cost < ChosenCost)
            {
                Chosen = Index;
                ChosenCost = Cost;
            }
        }
        return Chosen;
    }
}

// --------------------------------------------------------------------------------------------------------------------

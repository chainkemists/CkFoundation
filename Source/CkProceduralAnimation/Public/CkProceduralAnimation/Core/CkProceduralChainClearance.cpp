#include "CkProceduralAnimation/Core/CkProceduralChainClearance.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_chain_avoidance
{
    constexpr auto MaxLegs = 64;
    constexpr auto MaxCandidates = 24;
    constexpr auto MaxLinks = 8;
    constexpr auto MaxPasses = 3;

    struct FPairPenalty
    {
        bool Valid = true;
        double Penalty = 0.0;
        double MaxPenetration = 0.0;
        uint32 LeftLinks = 0;
        uint32 RightLinks = 0;
    };

    auto
        Get_Bounds(
            const ck::FProceduralChainPoseCandidate& InCandidate,
            TArrayView<const float> InRadii)
        -> FBox
    {
        auto Bounds = FBox{ForceInit};
        for (const auto& Joint : InCandidate.Get_Joints())
        { Bounds += Joint; }
        auto Radius = 0.0;
        for (const auto Value : InRadii)
        { Radius = FMath::Max(Radius, static_cast<double>(Value)); }
        return Bounds.ExpandBy(Radius);
    }

    auto
        Get_PairPenalty(
            const ck::FProceduralChainPoseCandidate& InLeft,
            TArrayView<const float> InLeftRadii,
            const FBox& InLeftBounds,
            const ck::FProceduralChainPoseCandidate& InRight,
            TArrayView<const float> InRightRadii,
            const FBox& InRightBounds)
        -> FPairPenalty
    {
        auto Result = FPairPenalty{};
        if (NOT InLeftBounds.Intersect(InRightBounds))
        { return Result; }
        const auto Left = InLeft.Get_Joints();
        const auto Right = InRight.Get_Joints();
        for (auto LeftLink = 0; LeftLink < InLeftRadii.Num(); ++LeftLink)
        {
            for (auto RightLink = 0; RightLink < InRightRadii.Num(); ++RightLink)
            {
                const auto Radius = static_cast<double>(InLeftRadii[LeftLink]) + InRightRadii[RightLink];
                if (Radius <= 0.0)
                { continue; }
                auto LeftPoint = FVector{};
                auto RightPoint = FVector{};
                FMath::SegmentDistToSegmentSafe(Left[LeftLink], Left[LeftLink + 1], Right[RightLink], Right[RightLink + 1],
                    LeftPoint, RightPoint);
                if (LeftPoint.ContainsNaN() || RightPoint.ContainsNaN())
                {
                    Result.Valid = false;
                    return Result;
                }
                const auto DistanceSquared = FVector::DistSquared(LeftPoint, RightPoint);
                if (NOT FMath::IsFinite(DistanceSquared))
                {
                    Result.Valid = false;
                    return Result;
                }
                if (DistanceSquared >= Radius * Radius)
                { continue; }
                const auto Penetration = Radius - FMath::Sqrt(DistanceSquared);
                Result.Penalty += Penetration * Penetration;
                Result.MaxPenetration = FMath::Max(Result.MaxPenetration, Penetration);
                Result.LeftLinks |= uint32{1} << LeftLink;
                Result.RightLinks |= uint32{1} << RightLink;
            }
        }
        return Result;
    }

    auto
        Get_AreInputsValid(
            TArrayView<const ck::FProceduralChainAvoidanceLeg> InLegs,
            TArrayView<int32> OutChoices,
            TArrayView<int32> OutCrossingLinks)
        -> bool
    {
        if (InLegs.Num() > MaxLegs || OutChoices.Num() != InLegs.Num() || OutCrossingLinks.Num() != InLegs.Num())
        { return false; }
        if (NOT InLegs.IsEmpty())
        {
            const auto ChoicesStart = reinterpret_cast<UPTRINT>(OutChoices.GetData());
            const auto CrossingsStart = reinterpret_cast<UPTRINT>(OutCrossingLinks.GetData());
            const auto Bytes = static_cast<UPTRINT>(InLegs.Num()) * sizeof(int32);
            if (ChoicesStart < CrossingsStart + Bytes && CrossingsStart < ChoicesStart + Bytes)
            { return false; }
        }
        for (auto LegIndex = 0; LegIndex < InLegs.Num(); ++LegIndex)
        {
            const auto& Leg = InLegs[LegIndex];
            const auto Candidates = Leg.Get_Candidates();
            const auto Radii = Leg.Get_Radii();
            if (Candidates.IsEmpty() || Candidates.Num() > MaxCandidates || Radii.IsEmpty() || Radii.Num() > MaxLinks
                || Leg.Get_InitialCandidate() < 0 || Leg.Get_InitialCandidate() >= Candidates.Num())
            { return false; }
            for (auto Previous = 0; Previous < LegIndex; ++Previous)
            {
                if (Leg.Get_StableId() == InLegs[Previous].Get_StableId())
                { return false; }
            }
            for (const auto Radius : Radii)
            {
                if (NOT FMath::IsFinite(Radius) || Radius < 0.0f)
                { return false; }
            }
            for (const auto& Candidate : Candidates)
            {
                const auto Joints = Candidate.Get_Joints();
                if (Joints.Num() != Radii.Num() + 1)
                { return false; }
                for (const auto& Joint : Joints)
                {
                    if (Joint.ContainsNaN())
                    { return false; }
                }
            }
        }
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProceduralChainAvoidanceSolver
    {
    public:
        static auto
            Select(
                TArrayView<const FProceduralChainAvoidanceLeg> InLegs,
                FProceduralChainAvoidanceScratch& InOutScratch,
                TArrayView<int32> OutChoices,
                TArrayView<int32> OutCrossingLinks,
                FProceduralChainAvoidanceOutcome& OutOutcome)
            -> bool
        {
            using namespace ck_procedural_chain_avoidance;
            if (NOT Get_AreInputsValid(InLegs, OutChoices, OutCrossingLinks))
            { return false; }

            const auto Count = InLegs.Num();
            auto& Bounds = InOutScratch._Bounds;
            auto& Offsets = InOutScratch._CandidateOffsets;
            auto& Choices = InOutScratch._Choices;
            auto& Order = InOutScratch._Order;
            auto& Matrix = InOutScratch._PairPenalties;
            auto& CandidateRow = InOutScratch._CandidateRow;
            auto& BestRow = InOutScratch._BestRow;
            auto& Masks = InOutScratch._CrossingMasks;
            Bounds.Reset();
            Offsets.SetNumUninitialized(Count);
            Choices.SetNumUninitialized(Count);
            Order.SetNumUninitialized(Count);
            Matrix.Init(0.0, Count * Count);
            CandidateRow.SetNumZeroed(Count);
            BestRow.SetNumZeroed(Count);
            Masks.Init(0, Count);
            for (auto LegIndex = 0; LegIndex < Count; ++LegIndex)
            {
                Offsets[LegIndex] = Bounds.Num();
                Choices[LegIndex] = InLegs[LegIndex].Get_InitialCandidate();
                Order[LegIndex] = LegIndex;
                for (const auto& Candidate : InLegs[LegIndex].Get_Candidates())
                { Bounds.Add(Get_Bounds(Candidate, InLegs[LegIndex].Get_Radii())); }
            }
            Order.Sort([&](int32 InLeft, int32 InRight) -> bool
            { return InLegs[InLeft].Get_StableId() < InLegs[InRight].Get_StableId(); });

            const auto Get_Pair = [&](int32 InLeft, int32 InLeftChoice, int32 InRight, int32 InRightChoice) -> FPairPenalty
            {
                return Get_PairPenalty(InLegs[InLeft].Get_Candidates()[InLeftChoice], InLegs[InLeft].Get_Radii(),
                    Bounds[Offsets[InLeft] + InLeftChoice], InLegs[InRight].Get_Candidates()[InRightChoice], InLegs[InRight].Get_Radii(),
                    Bounds[Offsets[InRight] + InRightChoice]);
            };
            auto Outcome = FProceduralChainAvoidanceOutcome{};
            auto Total = 0.0;
            for (auto Left = 0; Left < Count; ++Left)
            {
                for (auto Right = Left + 1; Right < Count; ++Right)
                {
                    const auto Pair = Get_Pair(Left, Choices[Left], Right, Choices[Right]);
                    if (NOT Pair.Valid)
                    { return false; }
                    Matrix[Left * Count + Right] = Pair.Penalty;
                    Matrix[Right * Count + Left] = Pair.Penalty;
                    Total += Pair.Penalty;
                }
            }
            for (auto Pass = 0; Pass < MaxPasses && Total > 0.0; ++Pass)
            {
                auto Improved = false;
                Outcome.Set_Passes(Pass + 1);
                for (const auto LegIndex : Order)
                {
                    auto CurrentPenalty = 0.0;
                    for (auto Other = 0; Other < Count; ++Other)
                    { CurrentPenalty += Matrix[LegIndex * Count + Other]; }
                    auto BestPenalty = CurrentPenalty;
                    auto BestChoice = Choices[LegIndex];
                    const auto Candidates = InLegs[LegIndex].Get_Candidates();
                    for (auto CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
                    {
                        if (CandidateIndex == Choices[LegIndex])
                        { continue; }
                        Outcome.Set_CandidateEvaluations(Outcome.Get_CandidateEvaluations() + 1);
                        auto Penalty = 0.0;
                        for (auto Other = 0; Other < Count; ++Other)
                        {
                            CandidateRow[Other] = 0.0;
                            if (Other == LegIndex)
                            { continue; }
                            const auto Pair = Get_Pair(LegIndex, CandidateIndex, Other, Choices[Other]);
                            if (NOT Pair.Valid)
                            { return false; }
                            CandidateRow[Other] = Pair.Penalty;
                            Penalty += Pair.Penalty;
                        }
                        if (Penalty < BestPenalty)
                        {
                            BestPenalty = Penalty;
                            BestChoice = CandidateIndex;
                            for (auto Other = 0; Other < Count; ++Other)
                            { BestRow[Other] = CandidateRow[Other]; }
                        }
                    }
                    if (BestChoice == Choices[LegIndex])
                    { continue; }

                    // Check the same body-wide sum before committing. Rounded row improvements must not worsen it.
                    auto NewTotal = 0.0;
                    for (auto Left = 0; Left < Count; ++Left)
                    {
                        for (auto Right = Left + 1; Right < Count; ++Right)
                        {
                            NewTotal += Left == LegIndex ? BestRow[Right]
                                : Right == LegIndex ? BestRow[Left] : Matrix[Left * Count + Right];
                        }
                    }
                    if (NOT (NewTotal < Total))
                    { continue; }
                    Choices[LegIndex] = BestChoice;
                    for (auto Other = 0; Other < Count; ++Other)
                    {
                        Matrix[LegIndex * Count + Other] = BestRow[Other];
                        Matrix[Other * Count + LegIndex] = BestRow[Other];
                    }
                    Total = NewTotal;
                    Improved = true;
                }
                if (NOT Improved)
                { break; }
            }
            for (auto Left = 0; Left < Count; ++Left)
            {
                for (auto Right = Left + 1; Right < Count; ++Right)
                {
                    const auto Pair = Get_Pair(Left, Choices[Left], Right, Choices[Right]);
                    if (NOT Pair.Valid)
                    { return false; }
                    Masks[Left] |= Pair.LeftLinks;
                    Masks[Right] |= Pair.RightLinks;
                    Outcome.Set_MaxPenetration(FMath::Max(Outcome.Get_MaxPenetration(), Pair.MaxPenetration));
                }
            }
            Outcome.Set_TotalPenalty(Total);
            for (auto LegIndex = 0; LegIndex < Count; ++LegIndex)
            {
                OutChoices[LegIndex] = Choices[LegIndex];
                OutCrossingLinks[LegIndex] = FMath::CountBits(Masks[LegIndex]);
            }
            OutOutcome = Outcome;
            return true;
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    auto
        SelectProceduralChainPoses(
            TArrayView<const FProceduralChainAvoidanceLeg> InLegs,
            FProceduralChainAvoidanceScratch& InOutScratch,
            TArrayView<int32> OutChoices,
            TArrayView<int32> OutCrossingLinks,
            FProceduralChainAvoidanceOutcome& OutOutcome)
        -> bool
    {
        return FProceduralChainAvoidanceSolver::Select(InLegs, InOutScratch, OutChoices, OutCrossingLinks, OutOutcome);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        ComputeProceduralPoleSwivel(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InPole,
            float InDegrees)
        -> FVector
    {
        const auto InputIsFinite = NOT InHip.ContainsNaN() && NOT InFoot.ContainsNaN() && NOT InPole.ContainsNaN()
            && FMath::IsFinite(InDegrees);
        if (NOT InputIsFinite || InDegrees == 0.0f)
        { return InPole; }

        const auto Axis = (InFoot - InHip).GetSafeNormal();
        if (Axis.IsNearlyZero())
        { return InPole; }

        const auto Swivel = FQuat{Axis, FMath::DegreesToRadians(static_cast<double>(InDegrees))};
        return InHip + Swivel.RotateVector(InPole - InHip);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Get_IsProceduralBendSidePreserved(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InAuthoredPole,
            const FVector& InBodyUp,
            const FVector& InAuthoredKnee,
            const FVector& InCandidatePole,
            const FVector& InCandidateKnee)
        -> bool
    {
        if (InHip.ContainsNaN() || InFoot.ContainsNaN() || InAuthoredPole.ContainsNaN()
            || InBodyUp.ContainsNaN() || InAuthoredKnee.ContainsNaN() || InCandidatePole.ContainsNaN()
            || InCandidateKnee.ContainsNaN() || InBodyUp.IsNearlyZero())
        { return false; }

        constexpr auto BoundaryTolerance = 1.0e-3;
        const auto Up = InBodyUp.GetSafeNormal();
        const auto Axis = (InFoot - InHip).GetSafeNormal();
        const auto AuthoredPole = InAuthoredPole - InHip;
        const auto AuthoredKnee = InAuthoredKnee - InHip;
        const auto CandidateKnee = InCandidateKnee - InHip;
        auto Bend = FVector::VectorPlaneProject(AuthoredPole, Axis).GetSafeNormal();
        if (Bend.IsNearlyZero())
        { Bend = FVector::VectorPlaneProject(AuthoredKnee, Axis).GetSafeNormal(); }

        const auto Preserves = [&](const FVector& InDirection, const FVector& InBaseline, const FVector& InCandidate) -> bool
        {
            if (InDirection.IsNearlyZero())
            { return true; }

            const auto BaselineDistance = FVector::DotProduct(InBaseline, InDirection);
            const auto CandidateDistance = FVector::DotProduct(InCandidate, InDirection);
            return FMath::IsFinite(BaselineDistance) && FMath::IsFinite(CandidateDistance)
                && CandidateDistance >= FMath::Min(0.0, BaselineDistance) - BoundaryTolerance;
        };

        const auto Tangent = FVector::VectorPlaneProject(AuthoredPole, Up).GetSafeNormal();
        return Preserves(Bend, AuthoredKnee, CandidateKnee)
            && Preserves(Tangent, AuthoredKnee, CandidateKnee)
            && Preserves(Tangent, AuthoredPole, InCandidatePole - InHip);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Get_ProceduralPoleSwivelOrder(
            float InLastClearDegrees,
            TArrayView<float> OutOrder)
        -> int32
    {
        const auto Fan = MakeArrayView(ProceduralPoleSwivelFanDegrees);
        const auto LastClearLeadsTheOrder = InLastClearDegrees != 0.0f && algo::AnyOf(Fan, [&](float InDegrees)
        {
            return InDegrees == InLastClearDegrees;
        });

        auto Count = 0;
        const auto Append = [&](float InDegrees)
        {
            if (Count < OutOrder.Num())
            { OutOrder[Count++] = InDegrees; }
        };

        if (LastClearLeadsTheOrder)
        { Append(InLastClearDegrees); }

        for (const auto Degrees : Fan)
        {
            if (LastClearLeadsTheOrder && Degrees == InLastClearDegrees)
            { continue; }

            Append(Degrees);
        }
        return Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------

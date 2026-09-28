#include "CkProceduralAnimation/Core/CkProceduralLegCurve.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_leg_curve
{
    constexpr auto MaxLinks = 8;
    constexpr auto SampleSegments = 48;
    constexpr auto BisectionIterations = 20;
    constexpr auto StraightReachFraction = 1.0 - 1.0e-4;
    constexpr auto FootHandleFraction = 0.5;
    constexpr auto MinBendSine = 1.0e-3;
    constexpr auto PastEnd = 2.0;
    constexpr auto ClosureTolerance = 0.01;
    constexpr auto ClosureIterations = 64;

    using FPolyline = TArray<FVector, TInlineAllocator<SampleSegments + 1>>;

    auto
        Get_AreInputsValid(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InBendDirection,
            TArrayView<const float> InLengths,
            TArrayView<FVector> InJoints)
        -> bool
    {
        return InLengths.Num() >= 1
            && InLengths.Num() <= MaxLinks
            && InJoints.Num() == InLengths.Num() + 1
            && NOT InHip.ContainsNaN()
            && NOT InFoot.ContainsNaN()
            && NOT InBendDirection.ContainsNaN()
            && ck::algo::AllOf(InLengths, [](float InLength) -> bool
            {
                return FMath::IsFinite(InLength) && InLength > 0.0f;
            });
    }

    auto
        Get_BezierPoint(
            const FVector& InP0,
            const FVector& InP1,
            const FVector& InP2,
            const FVector& InP3,
            double InU)
        -> FVector
    {
        const auto V = 1.0 - InU;
        return InP0 * (V * V * V) + InP1 * (3.0 * V * V * InU) + InP2 * (3.0 * V * InU * InU) + InP3 * (InU * InU * InU);
    }

    auto
        DoBuild_Polyline(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InBend,
            double InArch,
            FPolyline& OutPoints)
        -> void
    {
        const auto HipHandle = InHip + InBend * InArch;
        const auto FootHandle = InFoot + InBend * (FootHandleFraction * InArch);
        OutPoints.Reset();
        for (auto Sample = 0; Sample <= SampleSegments; ++Sample)
        {
            OutPoints.Add(Get_BezierPoint(InHip, HipHandle, FootHandle, InFoot, static_cast<double>(Sample) / SampleSegments));
        }
    }

    // Where, as a fraction of InStart->InEnd, the segment leaves the sphere of InRadius around InCenter. InStart lies inside
    // the sphere, so the larger root is the exit; a value above 1 means the segment ends inside.
    auto
        Get_ExitFraction(
            const FVector& InCenter,
            double InRadius,
            const FVector& InStart,
            const FVector& InEnd)
        -> double
    {
        const auto Direction = InEnd - InStart;
        const auto FromCenter = InStart - InCenter;
        const auto A = Direction.SizeSquared();
        if (A <= UE_DOUBLE_SMALL_NUMBER)
        { return PastEnd; }

        const auto B = 2.0 * FVector::DotProduct(FromCenter, Direction);
        const auto C = FromCenter.SizeSquared() - InRadius * InRadius;
        const auto Discriminant = FMath::Max(0.0, B * B - 4.0 * A * C);
        return (-B + FMath::Sqrt(Discriminant)) / (2.0 * A);
    }

    // Walks the links hip-first along the polyline, each joint the first point at its link's length from the previous one.
    // Past the polyline's end the walk continues along its last direction, so every joint is always placed. Returns the
    // polyline length left after the last joint, or the negative distance the last joint lies past the end.
    auto
        DoWalk(
            const FPolyline& InPoints,
            TArrayView<const float> InLengths,
            TArrayView<FVector> OutJoints)
        -> double
    {
        const auto LastPoint = InPoints.Last();
        const auto EndDirection = (LastPoint - InPoints[SampleSegments - 1]).GetSafeNormal();

        auto Joint = InPoints[0];
        auto SegmentStart = InPoints[0];
        auto Segment = 0;
        for (auto Link = 0; Link < InLengths.Num(); ++Link)
        {
            const auto Length = static_cast<double>(InLengths[Link]);
            auto Placed = false;
            while (Segment < SampleSegments)
            {
                const auto& SegmentEnd = InPoints[Segment + 1];
                const auto Exit = Get_ExitFraction(Joint, Length, SegmentStart, SegmentEnd);
                if (Exit <= 1.0)
                {
                    Joint = SegmentStart + (SegmentEnd - SegmentStart) * Exit;
                    SegmentStart = Joint;
                    Placed = true;
                    break;
                }
                ++Segment;
                SegmentStart = SegmentEnd;
            }

            if (NOT Placed)
            {
                Joint = SegmentStart + EndDirection * Get_ExitFraction(Joint, Length, SegmentStart, SegmentStart + EndDirection);
                SegmentStart = Joint;
            }

            if (OutJoints.Num() > Link + 1)
            { OutJoints[Link + 1] = Joint; }
        }

        if (Segment >= SampleSegments)
        { return -FMath::Max(FVector::Dist(Joint, LastPoint), UE_DOUBLE_SMALL_NUMBER); }

        auto Remaining = FVector::Dist(Joint, InPoints[Segment + 1]);
        for (auto Point = Segment + 2; Point <= SampleSegments; ++Point)
        { Remaining += FVector::Dist(InPoints[Point - 1], InPoints[Point]); }
        return Remaining;
    }

    // The polyline's first sphere exit can switch branches while folded, leaving no arch whose walk ends on the foot.
    // Keep the curve's proximal link when its remaining links can close the target; otherwise retry the whole chain.
    auto
        DoClose_FoldedChain(
            const FVector& InHip,
            const FVector& InFoot,
            TArrayView<const float> InLengths,
            TArrayView<FVector> InOutJoints)
        -> void
    {
        const auto InitialError = FVector::Dist(InOutJoints.Last(), InFoot);
        if (InitialError <= ClosureTolerance || InHip.Equals(InFoot, UE_DOUBLE_SMALL_NUMBER))
        { return; }

        auto TotalLength = 0.0;
        auto LongestLink = 0.0;
        for (const auto Link : InLengths)
        {
            TotalLength += Link;
            LongestLink = FMath::Max(LongestLink, static_cast<double>(Link));
        }
        const auto InnerReach = FMath::Max(0.0, 2.0 * LongestLink - TotalLength);
        if (FVector::Dist(InHip, InFoot) + ClosureTolerance < InnerReach)
        { return; }

        FVector SeedDirections[MaxLinks];
        FVector Joints[MaxLinks + 1];
        FVector Best[MaxLinks + 1];
        for (auto Index = 0; Index < InLengths.Num(); ++Index)
        {
            const auto Link = InOutJoints[Index + 1] - InOutJoints[Index];
            const auto LengthSquared = Link.SizeSquared();
            if (LengthSquared <= UE_DOUBLE_SMALL_NUMBER)
            { return; }
            SeedDirections[Index] = Link / FMath::Sqrt(LengthSquared);
        }
        for (auto Index = 0; Index < InOutJoints.Num(); ++Index)
        { Joints[Index] = InOutJoints[Index]; }

        auto BestError = InitialError;
        const auto DoAttempt = [&](int32 InFirstLink)
        {
            for (auto Index = 0; Index < InOutJoints.Num(); ++Index)
            { Joints[Index] = InOutJoints[Index]; }

            for (auto Iteration = 0; Iteration < ClosureIterations; ++Iteration)
            {
                Joints[InLengths.Num()] = InFoot;
                for (auto Index = InLengths.Num() - 1; Index >= InFirstLink; --Index)
                {
                    const auto Delta = Joints[Index] - Joints[Index + 1];
                    const auto LengthSquared = Delta.SizeSquared();
                    const auto Direction = LengthSquared > UE_DOUBLE_SMALL_NUMBER
                        ? Delta / FMath::Sqrt(LengthSquared) : -SeedDirections[Index];
                    Joints[Index] = Joints[Index + 1] + Direction * InLengths[Index];
                }
                Joints[InFirstLink] = InOutJoints[InFirstLink];
                for (auto Index = InFirstLink; Index < InLengths.Num(); ++Index)
                {
                    const auto Delta = Joints[Index + 1] - Joints[Index];
                    const auto LengthSquared = Delta.SizeSquared();
                    const auto Direction = LengthSquared > UE_DOUBLE_SMALL_NUMBER
                        ? Delta / FMath::Sqrt(LengthSquared) : SeedDirections[Index];
                    Joints[Index + 1] = Joints[Index] + Direction * InLengths[Index];
                }

                const auto Error = FVector::Dist(Joints[InLengths.Num()], InFoot);
                if (Error < BestError)
                {
                    BestError = Error;
                    for (auto Index = 0; Index < InOutJoints.Num(); ++Index)
                    { Best[Index] = Joints[Index]; }
                }
                if (Error <= ClosureTolerance)
                { break; }
            }
        };

        if (InLengths.Num() > 1)
        {
            auto SuffixLength = 0.0;
            auto SuffixLongest = 0.0;
            for (auto Index = 1; Index < InLengths.Num(); ++Index)
            {
                SuffixLength += InLengths[Index];
                SuffixLongest = FMath::Max(SuffixLongest, static_cast<double>(InLengths[Index]));
            }
            const auto SuffixInnerReach = FMath::Max(0.0, 2.0 * SuffixLongest - SuffixLength);
            const auto SuffixDistance = FVector::Dist(InOutJoints[1], InFoot);
            if (SuffixDistance + ClosureTolerance >= SuffixInnerReach && SuffixDistance <= SuffixLength + ClosureTolerance)
            { DoAttempt(1); }
        }

        if (BestError > ClosureTolerance)
        { DoAttempt(0); }

        if (BestError < InitialError)
        {
            for (auto Index = 0; Index < InOutJoints.Num(); ++Index)
            { InOutJoints[Index] = Best[Index]; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        SolveProceduralLegCurve(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InBendDirection,
            TArrayView<const float> InLengths,
            TArrayView<FVector> OutJoints)
        -> bool
    {
        using namespace ck_procedural_leg_curve;

        if (NOT Get_AreInputsValid(InHip, InFoot, InBendDirection, InLengths, OutJoints))
        { return false; }

        const auto HipToFoot = InFoot - InHip;
        const auto Along = HipToFoot.GetSafeNormal();
        const auto Bend = FVector::VectorPlaneProject(InBendDirection, Along);
        if (Bend.SizeSquared() <= FMath::Square(MinBendSine) * InBendDirection.SizeSquared())
        { return false; }

        auto Length = 0.0;
        for (const auto Link : InLengths)
        { Length += Link; }

        OutJoints[0] = InHip;
        if (HipToFoot.Size() >= Length * StraightReachFraction)
        {
            auto Reached = 0.0;
            for (auto Link = 0; Link < InLengths.Num(); ++Link)
            {
                Reached += InLengths[Link];
                OutJoints[Link + 1] = InHip + Along * Reached;
            }
            return true;
        }

        const auto BendNormal = Bend.GetUnsafeNormal();
        const auto NoJoints = TArrayView<FVector>{};
        auto Points = FPolyline{};
        auto LowArch = 0.0;
        auto HighArch = Length;
        for (auto Iteration = 0; Iteration < BisectionIterations; ++Iteration)
        {
            const auto Arch = 0.5 * (LowArch + HighArch);
            DoBuild_Polyline(InHip, InFoot, BendNormal, Arch, Points);
            if (DoWalk(Points, InLengths, NoJoints) < 0.0)
            { LowArch = Arch; }
            else
            { HighArch = Arch; }
        }

        DoBuild_Polyline(InHip, InFoot, BendNormal, HighArch, Points);
        DoWalk(Points, InLengths, OutJoints);
        DoClose_FoldedChain(InHip, InFoot, InLengths, OutJoints);
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

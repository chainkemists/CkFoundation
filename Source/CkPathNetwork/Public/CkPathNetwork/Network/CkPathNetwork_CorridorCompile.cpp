#include "CkPathNetwork_CorridorCompile.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    using namespace ck::pathnetwork;

    constexpr auto PointMergeDistance = 0.1f;
    constexpr auto SideOffsetClampMarginCm = 30.0f;
    constexpr auto CurveSampleSpacingCm = 40.0f;
    constexpr auto MaxCurveHeadingStepRadians = UE_PI / 8.0f;
    constexpr auto PathIntersectionToleranceCm = 0.01f;
    constexpr auto LocalFoldVerticalToleranceCm = 50.0f;
    constexpr auto LocalFoldLookAheadSegmentCount = 16;
    constexpr auto RasterSimplifyMinimumTurnDegrees = 35.0f;
    constexpr auto RasterSimplifyMaximumTurnDegrees = 135.0f;
    constexpr auto RasterSimplifyMinimumForwardDot = 0.05f;
    constexpr auto RasterSimplifyMaximumVerticalDeviationCm = 10.0f;
    constexpr auto RasterSimplifyLookAheadControlCount = 32;
    constexpr auto RasterSimplifyMaximumContainmentAttempts = 4;

    struct FCenterSample
    {
        FVector _Location = FVector::ZeroVector;
        float _HalfWidth = 0.0f;
    };

    struct FPointContainmentMeasurement
    {
        FVector _ClosestRibbonPoint = FVector::ZeroVector;
        int32 _SpanIndex = INDEX_NONE;
        int32 _EdgeId = INDEX_NONE;
        float _Distance3D = TNumericLimits<float>::Max();
        float _Distance2D = TNumericLimits<float>::Max();
        float _VerticalDistance = TNumericLimits<float>::Max();
        float _AllowedDistance = 0.0f;
        float _ExcessDistance = TNumericLimits<float>::Max();
    };

    auto
    Measure_PointAgainstRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InPoint,
        float InTolerance)
        -> FPointContainmentMeasurement
    {
        auto Result = FPointContainmentMeasurement{};

        for (auto SpanIndex = 0; SpanIndex < InSpans.Num(); ++SpanIndex)
        {
            const auto& Span = InSpans[SpanIndex];
            if (Span._IsOffPath || NOT InNetwork._Edges.IsValidIndex(Span._EdgeId))
            { continue; }

            const auto& Edge = InNetwork._Edges[Span._EdgeId];
            if (Edge._Points.Num() < 2 ||
                Edge._Points.Num() != Edge._HalfWidths.Num() ||
                Edge._Points.Num() != Edge._CumulativeLengths.Num())
            { continue; }

            const auto SpanMin = FMath::Clamp(
                FMath::Min(Span._FromDist, Span._ToDist), 0.0f, Edge._Length);
            const auto SpanMax = FMath::Clamp(
                FMath::Max(Span._FromDist, Span._ToDist), 0.0f, Edge._Length);

            for (auto SegmentIndex = 0; SegmentIndex < Edge._Points.Num() - 1; ++SegmentIndex)
            {
                const auto SegmentMin = FMath::Max(SpanMin, Edge._CumulativeLengths[SegmentIndex]);
                const auto SegmentMax = FMath::Min(SpanMax, Edge._CumulativeLengths[SegmentIndex + 1]);
                if (SegmentMax < SegmentMin)
                { continue; }

                const auto A = InNetwork.Sample_Edge(Span._EdgeId, SegmentMin);
                const auto B = InNetwork.Sample_Edge(Span._EdgeId, SegmentMax);
                const auto Closest = FMath::ClosestPointOnSegment(InPoint, A._Location, B._Location);
                const auto SegmentLengthSquared = FVector::DistSquared(A._Location, B._Location);
                const auto Alpha = SegmentLengthSquared > UE_KINDA_SMALL_NUMBER
                    ? FMath::Clamp(
                        static_cast<float>(FVector::DotProduct(
                            Closest - A._Location,
                            B._Location - A._Location) / SegmentLengthSquared),
                        0.0f,
                        1.0f)
                    : 0.0f;
                const auto AllowedDistance = FMath::Lerp(A._HalfWidth, B._HalfWidth, Alpha) + InTolerance;
                const auto Distance3D = static_cast<float>(FVector::Dist(InPoint, Closest));
                const auto ExcessDistance = Distance3D - AllowedDistance;

                if (ExcessDistance < Result._ExcessDistance)
                {
                    Result._ClosestRibbonPoint = Closest;
                    Result._SpanIndex = SpanIndex;
                    Result._EdgeId = Span._EdgeId;
                    Result._Distance3D = Distance3D;
                    Result._Distance2D = static_cast<float>(FVector::Dist2D(InPoint, Closest));
                    Result._VerticalDistance = static_cast<float>(FMath::Abs(InPoint.Z - Closest.Z));
                    Result._AllowedDistance = AllowedDistance;
                    Result._ExcessDistance = ExcessDistance;
                }

            }
        }

        return Result;
    }

    auto
    Append_Unique(TArray<FCenterSample>& InOutPoints, const FCenterSample& InPoint) -> void
    {
        if (InOutPoints.Num() > 0 &&
            FVector::DistSquared(InOutPoints.Last()._Location, InPoint._Location) <=
                FMath::Square(PointMergeDistance))
        {
            InOutPoints.Last()._HalfWidth =
                FMath::Min(InOutPoints.Last()._HalfWidth, InPoint._HalfWidth);
            return;
        }

        InOutPoints.Add(InPoint);
    }

    auto
    Append_Line(
        TArray<FCenterSample>& InOutPoints,
        const FCenterSample& InFrom,
        const FCenterSample& InTo,
        float InMaxSpacing) -> void
    {
        if (InOutPoints.IsEmpty())
        { Append_Unique(InOutPoints, InFrom); }

        const auto Length = static_cast<float>(FVector::Dist(InFrom._Location, InTo._Location));
        const auto StepCount = FMath::Max(1, FMath::CeilToInt32(Length / InMaxSpacing));

        for (auto Index = 1; Index <= StepCount; ++Index)
        {
            const auto Alpha = static_cast<float>(Index) / StepCount;
            Append_Unique(InOutPoints, FCenterSample{
                FMath::Lerp(InFrom._Location, InTo._Location, Alpha),
                FMath::Lerp(InFrom._HalfWidth, InTo._HalfWidth, Alpha)});
        }
    }

    auto
    Append_SpanControls(
        const FBuiltNetwork& InNetwork,
        const FRouteLegSpan& InSpan,
        TArray<FCenterSample>& InOutControls) -> bool
    {
        if (InSpan._IsOffPath || NOT InNetwork._Edges.IsValidIndex(InSpan._EdgeId))
        { return false; }

        const auto& Edge = InNetwork._Edges[InSpan._EdgeId];
        if (Edge._Points.Num() < 2 ||
            Edge._Points.Num() != Edge._HalfWidths.Num() ||
            Edge._Points.Num() != Edge._CumulativeLengths.Num())
        { return false; }

        const auto From = FMath::Clamp(InSpan._FromDist, 0.0f, Edge._Length);
        const auto To = FMath::Clamp(InSpan._ToDist, 0.0f, Edge._Length);
        const auto Direction = To >= From ? 1.0f : -1.0f;

        const auto AppendAtDistance = [&](float InDistance)
        {
            const auto Sample = InNetwork.Sample_Edge(InSpan._EdgeId, InDistance);
            Append_Unique(InOutControls, FCenterSample{Sample._Location, Sample._HalfWidth});
        };

        AppendAtDistance(From);

        if (Direction > 0.0f)
        {
            for (auto Index = 1; Index < Edge._CumulativeLengths.Num() - 1; ++Index)
            {
                const auto Distance = Edge._CumulativeLengths[Index];
                if (Distance > From + PointMergeDistance && Distance < To - PointMergeDistance)
                { AppendAtDistance(Distance); }
            }
        }
        else
        {
            for (auto Index = Edge._CumulativeLengths.Num() - 2; Index > 0; --Index)
            {
                const auto Distance = Edge._CumulativeLengths[Index];
                if (Distance < From - PointMergeDistance && Distance > To + PointMergeDistance)
                { AppendAtDistance(Distance); }
            }
        }

        AppendAtDistance(To);
        return true;
    }

    auto
    Gather_Controls(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans) -> TArray<FCenterSample>
    {
        auto Controls = TArray<FCenterSample>{};

        for (const auto& Span : InSpans)
        {
            if (NOT Append_SpanControls(InNetwork, Span, Controls))
            { return {}; }
        }

        return Controls;
    }

    auto
    Get_AlternatingMicroTurnSign(
        const FCenterSample& InPrevious,
        const FCenterSample& InCorner,
        const FCenterSample& InNext,
        float InMaximumLegLength) -> int32
    {
        const auto IncomingDelta = FVector2D{InCorner._Location - InPrevious._Location};
        const auto OutgoingDelta = FVector2D{InNext._Location - InCorner._Location};
        const auto IncomingLength = static_cast<float>(IncomingDelta.Size());
        const auto OutgoingLength = static_cast<float>(OutgoingDelta.Size());
        if (IncomingLength <= PointMergeDistance ||
            OutgoingLength <= PointMergeDistance ||
            IncomingLength > InMaximumLegLength ||
            OutgoingLength > InMaximumLegLength)
        { return 0; }

        const auto Incoming = IncomingDelta / IncomingLength;
        const auto Outgoing = OutgoingDelta / OutgoingLength;
        const auto Dot = FVector2D::DotProduct(Incoming, Outgoing);
        const auto MinimumDot = FMath::Cos(FMath::DegreesToRadians(
            RasterSimplifyMaximumTurnDegrees));
        const auto MaximumDot = FMath::Cos(FMath::DegreesToRadians(
            RasterSimplifyMinimumTurnDegrees));
        if (Dot < MinimumDot || Dot > MaximumDot)
        { return 0; }

        const auto Cross = Incoming.X * Outgoing.Y - Incoming.Y * Outgoing.X;
        if (FMath::IsNearlyZero(Cross))
        { return 0; }
        return Cross > 0.0f ? 1 : -1;
    }

    auto
    Does_ControlRunAdvanceAlongChord(
        TConstArrayView<FCenterSample> InControls,
        int32 InStartIndex,
        int32 InEndIndex) -> bool
    {
        const auto ChordDelta = FVector2D{
            InControls[InEndIndex]._Location - InControls[InStartIndex]._Location};
        const auto Chord = ChordDelta.GetSafeNormal();
        if (Chord.IsNearlyZero())
        { return false; }

        for (auto Index = InStartIndex; Index < InEndIndex; ++Index)
        {
            const auto Segment = FVector2D{
                InControls[Index + 1]._Location - InControls[Index]._Location}.GetSafeNormal();
            if (Segment.IsNearlyZero() ||
                FVector2D::DotProduct(Segment, Chord) <= RasterSimplifyMinimumForwardDot)
            { return false; }
        }
        return true;
    }

    auto
    Does_ControlRunPreserveVerticalProfile(
        TConstArrayView<FCenterSample> InControls,
        int32 InStartIndex,
        int32 InEndIndex) -> bool
    {
        const auto& Start = InControls[InStartIndex]._Location;
        const auto& End = InControls[InEndIndex]._Location;
        const auto ChordLength2DSquared = FVector::DistSquared2D(Start, End);
        if (ChordLength2DSquared <= UE_KINDA_SMALL_NUMBER)
        { return false; }

        for (auto Index = InStartIndex + 1; Index < InEndIndex; ++Index)
        {
            const auto& Point = InControls[Index]._Location;
            const auto Alpha = FMath::Clamp(
                static_cast<float>(FVector::DotProduct(
                    FVector{Point.X - Start.X, Point.Y - Start.Y, 0.0},
                    FVector{End.X - Start.X, End.Y - Start.Y, 0.0}) /
                    ChordLength2DSquared),
                0.0f,
                1.0f);
            const auto ChordHeight = FMath::Lerp(
                static_cast<float>(Start.Z),
                static_cast<float>(End.Z),
                Alpha);
            if (FMath::Abs(static_cast<float>(Point.Z) - ChordHeight) >
                RasterSimplifyMaximumVerticalDeviationCm)
            { return false; }
        }
        return true;
    }

    auto
    Simplify_AlternatingMicroTurns(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        TConstArrayView<FCenterSample> InControls,
        float InSmoothingDistance) -> TArray<FCenterSample>
    {
        if (InControls.Num() < 4 || InSmoothingDistance <= PointMergeDistance)
        {
            auto Unchanged = TArray<FCenterSample>{};
            Unchanged.Append(InControls.GetData(), InControls.Num());
            return Unchanged;
        }

        auto Result = TArray<FCenterSample>{};
        Result.Reserve(InControls.Num());
        Result.Add(InControls[0]);

        auto StartIndex = 0;
        while (StartIndex < InControls.Num() - 1)
        {
            int32 BestEndIndex = INDEX_NONE;
            int32 LastAlternatingEndIndex = INDEX_NONE;
            auto PreviousTurnSign = StartIndex + 2 < InControls.Num()
                ? Get_AlternatingMicroTurnSign(
                    InControls[StartIndex],
                    InControls[StartIndex + 1],
                    InControls[StartIndex + 2],
                    InSmoothingDistance)
                : 0;
            const auto LastCandidateEndIndex = FMath::Min(
                InControls.Num() - 1,
                StartIndex + RasterSimplifyLookAheadControlCount - 1);

            if (PreviousTurnSign != 0)
            {
                for (auto EndIndex = StartIndex + 3;
                     EndIndex <= LastCandidateEndIndex;
                     ++EndIndex)
                {
                    const auto TurnSign = Get_AlternatingMicroTurnSign(
                        InControls[EndIndex - 2],
                        InControls[EndIndex - 1],
                        InControls[EndIndex],
                        InSmoothingDistance);
                    if (TurnSign == 0 || TurnSign == PreviousTurnSign)
                    { break; }
                    PreviousTurnSign = TurnSign;
                    LastAlternatingEndIndex = EndIndex;
                }
            }

            auto ContainmentAttemptCount = 0;
            for (auto EndIndex = LastAlternatingEndIndex;
                 EndIndex >= StartIndex + 3;
                 --EndIndex)
            {
                if (ContainmentAttemptCount >= RasterSimplifyMaximumContainmentAttempts)
                { break; }
                ++ContainmentAttemptCount;

                if (NOT Does_ControlRunAdvanceAlongChord(
                            InControls,
                            StartIndex,
                            EndIndex) ||
                    NOT Does_ControlRunPreserveVerticalProfile(
                            InControls,
                            StartIndex,
                            EndIndex) ||
                    NOT Is_SegmentInsideRibbonRun(
                            InNetwork,
                            InSpans,
                            InControls[StartIndex]._Location,
                            InControls[EndIndex]._Location,
                            RibbonContainmentSampleSpacingCm,
                            RibbonContainmentToleranceCm))
                { continue; }

                BestEndIndex = EndIndex;
                break;
            }

            if (BestEndIndex != INDEX_NONE)
            {
                Append_Unique(Result, InControls[BestEndIndex]);
                StartIndex = BestEndIndex;
                continue;
            }

            ++StartIndex;
            Append_Unique(Result, InControls[StartIndex]);
        }

        return Result;
    }

    auto
    Quadratic(const FVector& InA, const FVector& InB, const FVector& InC, float InT) -> FVector
    {
        const auto OneMinusT = 1.0f - InT;
        return OneMinusT * OneMinusT * InA +
            2.0f * OneMinusT * InT * InB +
            InT * InT * InC;
    }

    auto
    Quadratic(float InA, float InB, float InC, float InT) -> float
    {
        const auto OneMinusT = 1.0f - InT;
        return OneMinusT * OneMinusT * InA +
            2.0f * OneMinusT * InT * InB +
            InT * InT * InC;
    }

    auto
    Build_Centerline(
        TConstArrayView<FCenterSample> InControls,
        float InWaypointSpacing,
        float InSmoothingDistance) -> TArray<FCenterSample>
    {
        auto Result = TArray<FCenterSample>{};
        if (InControls.Num() < 2)
        { return Result; }

        Append_Unique(Result, InControls[0]);
        auto Cursor = InControls[0];

        for (auto Index = 1; Index < InControls.Num() - 1; ++Index)
        {
            const auto& Previous = InControls[Index - 1];
            const auto& Corner = InControls[Index];
            const auto& Next = InControls[Index + 1];

            const auto IncomingDelta = Corner._Location - Previous._Location;
            const auto OutgoingDelta = Next._Location - Corner._Location;
            const auto IncomingLength = static_cast<float>(IncomingDelta.Size());
            const auto OutgoingLength = static_cast<float>(OutgoingDelta.Size());
            const auto Incoming = IncomingDelta.GetSafeNormal();
            const auto Outgoing = OutgoingDelta.GetSafeNormal();
            const auto Dot = FVector::DotProduct(Incoming, Outgoing);
            const auto TurnRadians = FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f));

            const auto CanSmooth =
                InSmoothingDistance > PointMergeDistance &&
                IncomingLength > PointMergeDistance &&
                OutgoingLength > PointMergeDistance &&
                Dot < FMath::Cos(FMath::DegreesToRadians(5.0f)) &&
                Dot > -0.95f;

            if (NOT CanSmooth)
            {
                Append_Line(Result, Cursor, Corner, InWaypointSpacing);
                Cursor = Corner;
                continue;
            }

            const auto Trim = FMath::Min3(
                InSmoothingDistance,
                IncomingLength * 0.45f,
                OutgoingLength * 0.45f);

            if (Trim <= PointMergeDistance)
            {
                Append_Line(Result, Cursor, Corner, InWaypointSpacing);
                Cursor = Corner;
                continue;
            }

            const auto EntryAlpha = Trim / IncomingLength;
            const auto ExitAlpha = Trim / OutgoingLength;
            const auto Entry = FCenterSample{
                Corner._Location - Incoming * Trim,
                FMath::Lerp(Corner._HalfWidth, Previous._HalfWidth, EntryAlpha)};
            const auto Exit = FCenterSample{
                Corner._Location + Outgoing * Trim,
                FMath::Lerp(Corner._HalfWidth, Next._HalfWidth, ExitAlpha)};

            Append_Line(Result, Cursor, Entry, InWaypointSpacing);

            const auto LengthSteps = FMath::CeilToInt32((2.0f * Trim) / CurveSampleSpacingCm);
            const auto HeadingSteps = FMath::CeilToInt32(TurnRadians / MaxCurveHeadingStepRadians);
            const auto StepCount = FMath::Max(2, FMath::Max(LengthSteps, HeadingSteps));

            for (auto StepIndex = 1; StepIndex <= StepCount; ++StepIndex)
            {
                const auto Alpha = static_cast<float>(StepIndex) / StepCount;
                Append_Unique(Result, FCenterSample{
                    Quadratic(Entry._Location, Corner._Location, Exit._Location, Alpha),
                    Quadratic(Entry._HalfWidth, Corner._HalfWidth, Exit._HalfWidth, Alpha)});
            }

            Cursor = Exit;
        }

        Append_Line(Result, Cursor, InControls.Last(), InWaypointSpacing);
        return Result;
    }

    auto
    Apply_SideOffset(
        TConstArrayView<FCenterSample> InCenterline,
        const FCorridorCompileParams& InParams) -> TArray<FVector>
    {
        auto Result = TArray<FVector>{};
        if (InCenterline.IsEmpty())
        { return Result; }

        auto CumulativeLengths = TArray<float>{};
        CumulativeLengths.Reserve(InCenterline.Num());
        CumulativeLengths.Add(0.0f);

        for (auto Index = 1; Index < InCenterline.Num(); ++Index)
        {
            CumulativeLengths.Add(
                CumulativeLengths.Last() +
                static_cast<float>(FVector::Dist(
                    InCenterline[Index - 1]._Location,
                    InCenterline[Index]._Location)));
        }

        const auto TotalLength = CumulativeLengths.Last();
        const auto RampDistance = FMath::Clamp(InParams._WaypointSpacing, 50.0f, 250.0f);

        Result.Reserve(InCenterline.Num());
        for (auto Index = 0; Index < InCenterline.Num(); ++Index)
        {
            auto Tangent = FVector::ForwardVector;
            if (InCenterline.Num() > 1)
            {
                if (Index == 0)
                {
                    Tangent =
                        (InCenterline[1]._Location - InCenterline[0]._Location).GetSafeNormal();
                }
                else if (Index == InCenterline.Num() - 1)
                {
                    Tangent =
                        (InCenterline[Index]._Location - InCenterline[Index - 1]._Location).GetSafeNormal();
                }
                else
                {
                    Tangent =
                        (InCenterline[Index + 1]._Location - InCenterline[Index - 1]._Location).GetSafeNormal();
                }
            }

            auto RampAlpha = 1.0f;
            if (InParams._RampSideOffsetAtStart)
            { RampAlpha = FMath::Min(RampAlpha, CumulativeLengths[Index] / RampDistance); }
            if (InParams._RampSideOffsetAtEnd)
            { RampAlpha = FMath::Min(RampAlpha, (TotalLength - CumulativeLengths[Index]) / RampDistance); }
            RampAlpha = FMath::Clamp(RampAlpha, 0.0f, 1.0f);

            const auto Right = FVector::CrossProduct(FVector::UpVector, Tangent).GetSafeNormal();
            const auto AvailableHalfWidth =
                FMath::Max(0.0f, InCenterline[Index]._HalfWidth - SideOffsetClampMarginCm);
            const auto Offset = FMath::Min(
                InParams._SideKeepingFraction * InCenterline[Index]._HalfWidth,
                AvailableHalfWidth) * RampAlpha;

            Result.Add(InCenterline[Index]._Location + Right * Offset);
        }

        return Result;
    }

    auto
    Does_PathContainLocalFold2D(
        TConstArrayView<FVector> InWaypoints) -> bool
    {
        const auto Orientation =
            [](const FVector& InA, const FVector& InB, const FVector& InC)
            {
                const auto AB = FVector2D{InB - InA};
                const auto AC = FVector2D{InC - InA};
                return AB.X * AC.Y - AB.Y * AC.X;
            };
        const auto ClosestAlpha2D =
            [](const FVector& InPoint, const FVector& InSegmentStart, const FVector& InSegmentEnd)
            {
                const auto Point = FVector2D{InPoint};
                const auto SegmentStart = FVector2D{InSegmentStart};
                const auto Segment = FVector2D{InSegmentEnd - InSegmentStart};
                const auto SegmentLengthSquared = Segment.SizeSquared();
                if (SegmentLengthSquared <= FMath::Square(PathIntersectionToleranceCm))
                { return 0.0; }

                return FMath::Clamp(
                    FVector2D::DotProduct(Point - SegmentStart, Segment) / SegmentLengthSquared,
                    0.0,
                    1.0);
            };
        const auto IsPointOnSegmentAtSameLevel =
            [&](const FVector& InPoint, const FVector& InSegmentStart, const FVector& InSegmentEnd)
            {
                const auto Alpha = ClosestAlpha2D(InPoint, InSegmentStart, InSegmentEnd);
                const auto ClosestPoint = FMath::Lerp(InSegmentStart, InSegmentEnd, Alpha);
                return FVector2D::DistSquared(FVector2D{InPoint}, FVector2D{ClosestPoint}) <=
                        FMath::Square(PathIntersectionToleranceCm) &&
                    FMath::Abs(InPoint.Z - ClosestPoint.Z) <= LocalFoldVerticalToleranceCm;
            };
        const auto HasOppositeOrientation =
            [](const double InA, const double InB, const double InTolerance)
            {
                return
                    (InA > InTolerance && InB < -InTolerance) ||
                    (InA < -InTolerance && InB > InTolerance);
            };

        const auto SegmentCount = InWaypoints.Num() - 1;
        if (SegmentCount < 3)
        { return false; }

        const auto IsClosed = InWaypoints[0].Equals(
            InWaypoints.Last(),
            PathIntersectionToleranceCm);
        for (auto SegmentA = 0; SegmentA < SegmentCount; ++SegmentA)
        {
            const auto& A0 = InWaypoints[SegmentA];
            const auto& A1 = InWaypoints[SegmentA + 1];
            const auto SegmentA2D = FVector2D{A1 - A0};
            const auto OrientationToleranceA =
                PathIntersectionToleranceCm * FMath::Max(SegmentA2D.Size(), 1.0);
            // Side-offset folds are local to a tight curve. Bounding this look-ahead keeps the
            // safeguard linear while still spanning several sampled fillets around the turn.
            const auto LastSegmentB = FMath::Min(
                SegmentCount,
                SegmentA + LocalFoldLookAheadSegmentCount + 1);
            for (auto SegmentB = SegmentA + 2; SegmentB < LastSegmentB; ++SegmentB)
            {
                if (IsClosed && SegmentA == 0 && SegmentB == SegmentCount - 1)
                { continue; }

                const auto& B0 = InWaypoints[SegmentB];
                const auto& B1 = InWaypoints[SegmentB + 1];
                const auto BoundsOverlap =
                    FMath::Max(FMath::Min(A0.X, A1.X), FMath::Min(B0.X, B1.X)) <=
                        FMath::Min(FMath::Max(A0.X, A1.X), FMath::Max(B0.X, B1.X)) +
                            PathIntersectionToleranceCm &&
                    FMath::Max(FMath::Min(A0.Y, A1.Y), FMath::Min(B0.Y, B1.Y)) <=
                        FMath::Min(FMath::Max(A0.Y, A1.Y), FMath::Max(B0.Y, B1.Y)) +
                            PathIntersectionToleranceCm;
                if (NOT BoundsOverlap)
                { continue; }

                const auto SegmentB2D = FVector2D{B1 - B0};
                const auto OrientationToleranceB =
                    PathIntersectionToleranceCm * FMath::Max(SegmentB2D.Size(), 1.0);
                const auto HasStrictCrossing = HasOppositeOrientation(
                        Orientation(A0, A1, B0),
                        Orientation(A0, A1, B1),
                        OrientationToleranceA) &&
                    HasOppositeOrientation(
                        Orientation(B0, B1, A0),
                        Orientation(B0, B1, A1),
                        OrientationToleranceB);
                if (HasStrictCrossing)
                {
                    const auto AVector = FVector2D{A1 - A0};
                    const auto BVector = FVector2D{B1 - B0};
                    const auto B0FromA0 = FVector2D{B0 - A0};
                    const auto Denominator =
                        AVector.X * BVector.Y - AVector.Y * BVector.X;
                    if (NOT FMath::IsNearlyZero(Denominator))
                    {
                        const auto AlphaA =
                            (B0FromA0.X * BVector.Y - B0FromA0.Y * BVector.X) /
                            Denominator;
                        const auto AlphaB =
                            (B0FromA0.X * AVector.Y - B0FromA0.Y * AVector.X) /
                            Denominator;
                        const auto ZA = FMath::Lerp(A0.Z, A1.Z, AlphaA);
                        const auto ZB = FMath::Lerp(B0.Z, B1.Z, AlphaB);
                        if (FMath::Abs(ZA - ZB) <= LocalFoldVerticalToleranceCm)
                        { return true; }
                    }
                }

                if (IsPointOnSegmentAtSameLevel(A0, B0, B1) ||
                    IsPointOnSegmentAtSameLevel(A1, B0, B1) ||
                    IsPointOnSegmentAtSameLevel(B0, A0, A1) ||
                    IsPointOnSegmentAtSameLevel(B1, A0, A1))
                { return true; }
            }
        }

        return false;
    }

    auto
    Is_CompiledPathContained(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        TConstArrayView<FVector> InWaypoints) -> bool
    {
        if (InWaypoints.Num() < 2)
        { return false; }

        for (auto Index = 0; Index < InWaypoints.Num() - 1; ++Index)
        {
            if (NOT Is_SegmentInsideRibbonRun(
                InNetwork,
                InSpans,
                InWaypoints[Index],
                InWaypoints[Index + 1],
                RibbonContainmentSampleSpacingCm,
                RibbonContainmentToleranceCm))
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    Is_PointInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InPoint,
        float InTolerance)
        -> bool
    {
        if (NOT FMath::IsFinite(InTolerance) || InTolerance < 0.0f)
        { return false; }

        for (const auto& Span : InSpans)
        {
            if (Span._IsOffPath || NOT InNetwork._Edges.IsValidIndex(Span._EdgeId))
            { continue; }

            const auto& Edge = InNetwork._Edges[Span._EdgeId];
            if (Edge._Points.Num() < 2 ||
                Edge._Points.Num() != Edge._HalfWidths.Num() ||
                Edge._Points.Num() != Edge._CumulativeLengths.Num())
            { continue; }

            const auto SpanMin = FMath::Clamp(
                FMath::Min(Span._FromDist, Span._ToDist), 0.0f, Edge._Length);
            const auto SpanMax = FMath::Clamp(
                FMath::Max(Span._FromDist, Span._ToDist), 0.0f, Edge._Length);

            for (auto SegmentIndex = 0; SegmentIndex < Edge._Points.Num() - 1; ++SegmentIndex)
            {
                const auto SegmentMin = FMath::Max(SpanMin, Edge._CumulativeLengths[SegmentIndex]);
                const auto SegmentMax = FMath::Min(SpanMax, Edge._CumulativeLengths[SegmentIndex + 1]);
                if (SegmentMax < SegmentMin)
                { continue; }

                const auto A = InNetwork.Sample_Edge(Span._EdgeId, SegmentMin);
                const auto B = InNetwork.Sample_Edge(Span._EdgeId, SegmentMax);
                const auto Closest = FMath::ClosestPointOnSegment(InPoint, A._Location, B._Location);
                const auto SegmentLengthSquared = FVector::DistSquared(A._Location, B._Location);
                const auto Alpha = SegmentLengthSquared > UE_KINDA_SMALL_NUMBER
                    ? FMath::Clamp(
                        static_cast<float>(FVector::DotProduct(
                            Closest - A._Location,
                            B._Location - A._Location) / SegmentLengthSquared),
                        0.0f,
                        1.0f)
                    : 0.0f;
                const auto HalfWidth = FMath::Lerp(A._HalfWidth, B._HalfWidth, Alpha);

                if (FVector::DistSquared(InPoint, Closest) <= FMath::Square(HalfWidth + InTolerance))
                { return true; }
            }
        }

        return false;
    }

    auto
    Is_SegmentInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InFrom,
        const FVector& InTo,
        float InSampleSpacing,
        float InTolerance,
        FRibbonContainmentFailure* OutFailure)
        -> bool
    {
        if (OutFailure != nullptr)
        { *OutFailure = FRibbonContainmentFailure{}; }

        if (NOT FMath::IsFinite(InSampleSpacing) || InSampleSpacing <= 0.0f)
        { return false; }
        if (NOT FMath::IsFinite(InTolerance) || InTolerance < 0.0f)
        { return false; }

        const auto Length = static_cast<float>(FVector::Dist(InFrom, InTo));
        const auto StepCount = FMath::Max(1, FMath::CeilToInt32(Length / InSampleSpacing));

        for (auto Index = 0; Index <= StepCount; ++Index)
        {
            const auto Alpha = static_cast<float>(Index) / StepCount;
            const auto Sample = FMath::Lerp(InFrom, InTo, Alpha);
            if (Is_PointInsideRibbonRun(InNetwork, InSpans, Sample, InTolerance))
            { continue; }

            if (OutFailure != nullptr)
            {
                const auto Measurement = Measure_PointAgainstRibbonRun(
                    InNetwork,
                    InSpans,
                    Sample,
                    InTolerance);
                OutFailure->_Sample = Sample;
                OutFailure->_ClosestRibbonPoint = Measurement._ClosestRibbonPoint;
                OutFailure->_SampleIndex = Index;
                OutFailure->_SampleCount = StepCount + 1;
                OutFailure->_SpanIndex = Measurement._SpanIndex;
                OutFailure->_EdgeId = Measurement._EdgeId;
                OutFailure->_Distance3D = Measurement._Distance3D;
                OutFailure->_Distance2D = Measurement._Distance2D;
                OutFailure->_VerticalDistance = Measurement._VerticalDistance;
                OutFailure->_AllowedDistance = Measurement._AllowedDistance;
            }
            return false;
        }

        return true;
    }

    auto
    Compile_OnRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FCorridorCompileParams& InParams)
        -> TArray<FVector>
    {
        const auto ParamsAreValid =
            FMath::IsFinite(InParams._SideKeepingFraction) &&
            InParams._SideKeepingFraction >= 0.0f &&
            InParams._SideKeepingFraction <= 0.9f &&
            FMath::IsFinite(InParams._WaypointSpacing) &&
            InParams._WaypointSpacing >= 50.0f &&
            FMath::IsFinite(InParams._CornerSmoothingDistance) &&
            InParams._CornerSmoothingDistance >= 0.0f;

        if (NOT ParamsAreValid || InSpans.IsEmpty())
        { return {}; }

        const auto GatheredControls = Gather_Controls(InNetwork, InSpans);
        if (GatheredControls.Num() < 2)
        { return {}; }
        const auto Controls = Simplify_AlternatingMicroTurns(
            InNetwork,
            InSpans,
            GatheredControls,
            InParams._CornerSmoothingDistance);

        auto SmoothingDistance = InParams._CornerSmoothingDistance;
        for (auto Attempt = 0; Attempt < 6; ++Attempt)
        {
            const auto Centerline = Build_Centerline(
                Controls,
                InParams._WaypointSpacing,
                SmoothingDistance);
            const auto Waypoints = Apply_SideOffset(Centerline, InParams);

            if (Is_CompiledPathContained(InNetwork, InSpans, Waypoints))
            {
                if (InParams._SideKeepingFraction <= 0.0f ||
                    NOT Does_PathContainLocalFold2D(Waypoints))
                { return Waypoints; }

                // A side offset larger than a raster-scale curve radius can fold the offset path
                // across itself even though every point remains inside the ribbon. Preserve the
                // smooth centerline for that curve instead of making the follower walk the fold.
                if (InParams._SideKeepingFraction > 0.0f)
                {
                    auto CenteredParams = InParams;
                    CenteredParams._SideKeepingFraction = 0.0f;
                    const auto CenteredWaypoints = Apply_SideOffset(
                        Centerline,
                        CenteredParams);
                    if (Is_CompiledPathContained(
                            InNetwork,
                            InSpans,
                            CenteredWaypoints))
                    { return CenteredWaypoints; }
                }
            }

            SmoothingDistance = Attempt < 4 ? SmoothingDistance * 0.5f : 0.0f;
        }

        // The selected centerline is the final fail-soft representation: it retains every selected
        // control and cannot create a shortcut merely because the configured spacing is large.
        auto CenterlineParams = InParams;
        CenterlineParams._SideKeepingFraction = 0.0f;
        const auto Fallback = Apply_SideOffset(
            Build_Centerline(Controls, InParams._WaypointSpacing, 0.0f),
            CenterlineParams);
        return Is_CompiledPathContained(InNetwork, InSpans, Fallback)
            ? Fallback
            : TArray<FVector>{};
    }
}

// --------------------------------------------------------------------------------------------------------------------

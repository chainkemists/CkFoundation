#include "CkPathNetwork_CorridorCompile.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    using namespace ck::pathnetwork;

    constexpr auto PointMergeDistance = 0.1f;
    constexpr auto SideOffsetClampMarginCm = 30.0f;
    constexpr auto CurveSampleSpacingCm = 40.0f;
    constexpr auto ContainmentSampleSpacingCm = 10.0f;
    constexpr auto MaxCurveHeadingStepRadians = UE_PI / 8.0f;

    struct FCenterSample
    {
        FVector _Location = FVector::ZeroVector;
        float _HalfWidth = 0.0f;
    };

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
                ContainmentSampleSpacingCm,
                1.0f))
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
        float InTolerance)
        -> bool
    {
        if (NOT FMath::IsFinite(InSampleSpacing) || InSampleSpacing <= 0.0f)
        { return false; }

        const auto Length = static_cast<float>(FVector::Dist(InFrom, InTo));
        const auto StepCount = FMath::Max(1, FMath::CeilToInt32(Length / InSampleSpacing));

        for (auto Index = 0; Index <= StepCount; ++Index)
        {
            const auto Alpha = static_cast<float>(Index) / StepCount;
            if (NOT Is_PointInsideRibbonRun(
                InNetwork,
                InSpans,
                FMath::Lerp(InFrom, InTo, Alpha),
                InTolerance))
            { return false; }
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

        const auto Controls = Gather_Controls(InNetwork, InSpans);
        if (Controls.Num() < 2)
        { return {}; }

        auto SmoothingDistance = InParams._CornerSmoothingDistance;
        for (auto Attempt = 0; Attempt < 6; ++Attempt)
        {
            const auto Centerline = Build_Centerline(
                Controls,
                InParams._WaypointSpacing,
                SmoothingDistance);
            const auto Waypoints = Apply_SideOffset(Centerline, InParams);

            if (Is_CompiledPathContained(InNetwork, InSpans, Waypoints))
            { return Waypoints; }

            SmoothingDistance = Attempt < 4 ? SmoothingDistance * 0.5f : 0.0f;
        }

        // The selected centerline is the final fail-soft representation: it retains every source
        // corner and cannot create a shortcut merely because the configured spacing is large.
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

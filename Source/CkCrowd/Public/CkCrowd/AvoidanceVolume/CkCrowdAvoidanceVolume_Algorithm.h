#pragma once

#include "CoreMinimal.h"

namespace ck::crowd_avoidance_volume
{
    struct FCk_Obb final
    {
        FTransform _YawTransform = FTransform::Identity;
        FVector _WorldHalfExtents = FVector::ZeroVector;

        auto IsFiniteAndPositive() const -> bool
        {
            return NOT _YawTransform.ContainsNaN() &&
                FMath::IsFinite(_WorldHalfExtents.X) &&
                FMath::IsFinite(_WorldHalfExtents.Y) && FMath::IsFinite(_WorldHalfExtents.Z) &&
                _WorldHalfExtents.X > 0.0f && _WorldHalfExtents.Y > 0.0f && _WorldHalfExtents.Z > 0.0f;
        }

        auto ExpandedXY(float InRadius) const -> FCk_Obb
        {
            auto Result = *this;
            if (NOT FMath::IsFinite(InRadius) || InRadius < 0.0f)
            { return {}; }

            Result._WorldHalfExtents.X += InRadius;
            Result._WorldHalfExtents.Y += InRadius;
            return Result;
        }
    };

    inline auto MakeObb(const FTransform& InAuthoredTransform, const FVector& InAuthoredHalfExtents) -> FCk_Obb
    {
        const auto Scale = InAuthoredTransform.GetScale3D().GetAbs();
        return FCk_Obb{
            FTransform{FRotator{0.0f, InAuthoredTransform.Rotator().Yaw, 0.0f}, InAuthoredTransform.GetLocation(), FVector::OneVector},
            InAuthoredHalfExtents * Scale};
    }

    inline auto MakeEffectiveAgentObb(
        const FCk_Obb& InPhysicalObb,
        const FCk_Obb& InPaintedObb,
        float InAgentRadius) -> FCk_Obb
    {
        const auto PhysicalExpanded = InPhysicalObb.ExpandedXY(InAgentRadius);
        if (NOT PhysicalExpanded.IsFiniteAndPositive() || NOT InPaintedObb.IsFiniteAndPositive())
        { return {}; }

        auto Result = PhysicalExpanded;
        Result._WorldHalfExtents.X = FMath::Max(
            Result._WorldHalfExtents.X, InPaintedObb._WorldHalfExtents.X);
        Result._WorldHalfExtents.Y = FMath::Max(
            Result._WorldHalfExtents.Y, InPaintedObb._WorldHalfExtents.Y);
        Result._WorldHalfExtents.Z = FMath::Max(
            Result._WorldHalfExtents.Z, InPaintedObb._WorldHalfExtents.Z);
        return Result;
    }

    inline auto ContainsPoint(const FCk_Obb& InObb, const FVector& InPoint, float InBoundaryTolerance = 0.0f) -> bool
    {
        if (NOT InObb.IsFiniteAndPositive() || InPoint.ContainsNaN() || NOT FMath::IsFinite(InBoundaryTolerance))
        { return false; }

        const auto Local = InObb._YawTransform.InverseTransformPositionNoScale(InPoint);
        const auto Half = InObb._WorldHalfExtents + FVector{InBoundaryTolerance};
        return FMath::Abs(Local.X) <= Half.X && FMath::Abs(Local.Y) <= Half.Y && FMath::Abs(Local.Z) <= Half.Z;
    }

    // Slab clipping in the yaw-only local frame. Boundary and tangential contact deliberately
    // count as intersection: a path merely touching the physical footprint is not a safe route.
    inline auto GetSegmentInsideInterval(
        const FCk_Obb& InObb,
        const FVector& InStart,
        const FVector& InEnd) -> TOptional<TPair<float, float>>
    {
        if (NOT InObb.IsFiniteAndPositive() || InStart.ContainsNaN() || InEnd.ContainsNaN())
        { return {}; }

        const auto Start = InObb._YawTransform.InverseTransformPositionNoScale(InStart);
        const auto Delta = InObb._YawTransform.InverseTransformVectorNoScale(InEnd - InStart);
        auto Enter = 0.0f;
        auto Exit = 1.0f;
        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto StartAxis = Start[Axis];
            const auto DeltaAxis = Delta[Axis];
            const auto Half = InObb._WorldHalfExtents[Axis];
            if (FMath::IsNearlyZero(DeltaAxis))
            {
                if (StartAxis < -Half || StartAxis > Half)
                { return {}; }
                continue;
            }

            auto T0 = (-Half - StartAxis) / DeltaAxis;
            auto T1 = ( Half - StartAxis) / DeltaAxis;
            if (T0 > T1)
            { Swap(T0, T1); }
            Enter = FMath::Max(Enter, T0);
            Exit = FMath::Min(Exit, T1);
            if (Enter > Exit)
            { return {}; }
        }
        if (Enter > 1.0f || Exit < 0.0f)
        { return {}; }
        return TPair<float, float>{FMath::Clamp(Enter, 0.0f, 1.0f), FMath::Clamp(Exit, 0.0f, 1.0f)};
    }

    inline auto IntersectsSegment(const FCk_Obb& InObb, const FVector& InStart, const FVector& InEnd) -> bool
    {
        return GetSegmentInsideInterval(InObb, InStart, InEnd).IsSet();
    }

    inline auto FindNearestFaceEscapePoint(const FCk_Obb& InObb, const FVector& InPoint, float InMargin) -> TOptional<FVector>
    {
        if (NOT InObb.IsFiniteAndPositive() || InPoint.ContainsNaN() || NOT FMath::IsFinite(InMargin) || InMargin < 0.0f)
        { return {}; }

        const auto Local = InObb._YawTransform.InverseTransformPositionNoScale(InPoint);
        if (FMath::Abs(Local.X) > InObb._WorldHalfExtents.X || FMath::Abs(Local.Y) > InObb._WorldHalfExtents.Y ||
            FMath::Abs(Local.Z) > InObb._WorldHalfExtents.Z)
        { return {}; }

        const auto DistX = InObb._WorldHalfExtents.X - FMath::Abs(Local.X);
        const auto DistY = InObb._WorldHalfExtents.Y - FMath::Abs(Local.Y);
        auto Escaped = Local;
        if (DistX <= DistY)
        { Escaped.X = (Local.X >= 0.0f ? 1.0f : -1.0f) * (InObb._WorldHalfExtents.X + InMargin); }
        else
        { Escaped.Y = (Local.Y >= 0.0f ? 1.0f : -1.0f) * (InObb._WorldHalfExtents.Y + InMargin); }
        return InObb._YawTransform.TransformPositionNoScale(Escaped);
    }

    // Returns the first point strictly outside this OBB along a caller-normalized XY ray. This is
    // intentionally value-only so PathRefresh can compose it into its bounded union-ray search.
    inline auto FindRayExitPoint(
        const FCk_Obb& InObb,
        const FVector& InInsidePoint,
        const FVector2D& InNormalizedDirection,
        float InMargin) -> TOptional<FVector>
    {
        if (NOT InObb.IsFiniteAndPositive() || InInsidePoint.ContainsNaN() ||
            NOT FMath::IsFinite(InMargin) || InMargin < 0.0f ||
            NOT FMath::IsFinite(InNormalizedDirection.X) || NOT FMath::IsFinite(InNormalizedDirection.Y) ||
            InNormalizedDirection.IsNearlyZero())
        { return {}; }

        const auto LocalPoint = InObb._YawTransform.InverseTransformPositionNoScale(InInsidePoint);
        if (FMath::Abs(LocalPoint.X) > InObb._WorldHalfExtents.X ||
            FMath::Abs(LocalPoint.Y) > InObb._WorldHalfExtents.Y ||
            FMath::Abs(LocalPoint.Z) > InObb._WorldHalfExtents.Z)
        { return {}; }

        const auto WorldDirection = FVector{InNormalizedDirection.X, InNormalizedDirection.Y, 0.0f}.GetSafeNormal2D();
        const auto LocalDirection = InObb._YawTransform.InverseTransformVectorNoScale(WorldDirection).GetSafeNormal2D();
        auto ExitDistance = TNumericLimits<float>::Max();
        const auto ConsiderAxis = [&](float InPosition, float InDirection, float InHalfExtent) -> void
        {
            if (FMath::IsNearlyZero(InDirection))
            { return; }
            const auto Boundary = InDirection > 0.0f ? InHalfExtent : -InHalfExtent;
            const auto Distance = (Boundary - InPosition) / InDirection;
            if (Distance >= 0.0f)
            { ExitDistance = FMath::Min(ExitDistance, Distance); }
        };
        ConsiderAxis(LocalPoint.X, LocalDirection.X, InObb._WorldHalfExtents.X);
        ConsiderAxis(LocalPoint.Y, LocalDirection.Y, InObb._WorldHalfExtents.Y);
        if (NOT FMath::IsFinite(ExitDistance))
        { return {}; }
        return InInsidePoint + WorldDirection * (ExitDistance + InMargin);
    }
}

// --------------------------------------------------------------------------------------------------------------------

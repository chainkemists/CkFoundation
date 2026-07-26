#include "CkRewindHistory_Query.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_LagComp_FrameBuffer::
    Init(
        int32 InCapacity)
    -> void
{
    CK_ENSURE_IF_NOT(InCapacity > 0,
        TEXT("FrameBuffer capacity [{}] must be positive"), InCapacity)
    { InCapacity = 1; }

    _Frames.Empty(InCapacity);
    _Frames.SetNum(InCapacity);
    _Head = 0;
    _Count = 0;
}

auto
    FCk_LagComp_FrameBuffer::
    Push(
        FCk_LagComp_RewindFrame InFrame)
    -> void
{
    if (_Frames.IsEmpty())
    { Init(1); }

    if (_Count < _Frames.Num())
    {
        _Frames[(_Head + _Count) % _Frames.Num()] = MoveTemp(InFrame);
        ++_Count;
        return;
    }

    _Frames[_Head] = MoveTemp(InFrame);
    _Head = (_Head + 1) % _Frames.Num();
}

auto
    FCk_LagComp_FrameBuffer::
    Reset()
    -> void
{
    _Head = 0;
    _Count = 0;
}

auto
    FCk_LagComp_FrameBuffer::
    Get_Count() const
    -> int32
{
    return _Count;
}

auto
    FCk_LagComp_FrameBuffer::
    Get_Capacity() const
    -> int32
{
    return _Frames.Num();
}

auto
    FCk_LagComp_FrameBuffer::
    Get_FrameAt(
        int32 InChronologicalIndex) const
    -> const FCk_LagComp_RewindFrame&
{
    return _Frames[(_Head + InChronologicalIndex) % _Frames.Num()];
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::lag_comp_detail
{
    // Smallest t in [0, InRayLength] at which the ray hits the sphere. Starts-inside reports t=0
    static auto
    Get_RayVsSphere(
        const FVector& InRayOrigin,
        const FVector& InRayDirection,
        double InRayLength,
        const FVector& InSphereCenter,
        double InSphereRadius) -> TOptional<double>
    {
        const auto ToOrigin = InRayOrigin - InSphereCenter;

        if (ToOrigin.SizeSquared() <= InSphereRadius * InSphereRadius)
        { return 0.0; }

        const auto B = FVector::DotProduct(ToOrigin, InRayDirection);
        const auto C = ToOrigin.SizeSquared() - InSphereRadius * InSphereRadius;
        const auto Discriminant = B * B - C;

        if (Discriminant < 0.0)
        { return {}; }

        const auto T = -B - FMath::Sqrt(Discriminant);

        if (T < 0.0 || T > InRayLength)
        { return {}; }

        return T;
    }

    static auto
    Get_RayVsCapsule(
        const FVector& InRayOrigin,
        const FVector& InRayDirection,
        double InRayLength,
        const FVector& InCapA,
        const FVector& InCapB,
        double InCapsuleRadius) -> TOptional<double>
    {
        const auto Axis = InCapB - InCapA;
        const auto AxisLengthSquared = Axis.SizeSquared();

        if (AxisLengthSquared < UE_DOUBLE_SMALL_NUMBER)
        { return Get_RayVsSphere(InRayOrigin, InRayDirection, InRayLength, InCapA, InCapsuleRadius); }

        const auto AxisDirection = Axis / FMath::Sqrt(AxisLengthSquared);
        const auto ToOrigin = InRayOrigin - InCapA;

        const auto DirPerp = InRayDirection - FVector::DotProduct(InRayDirection, AxisDirection) * AxisDirection;
        const auto OriginPerp = ToOrigin - FVector::DotProduct(ToOrigin, AxisDirection) * AxisDirection;

        const auto A = DirPerp.SizeSquared();
        const auto B = FVector::DotProduct(OriginPerp, DirPerp);
        const auto C = OriginPerp.SizeSquared() - InCapsuleRadius * InCapsuleRadius;

        auto BestT = TOptional<double>{};

        const auto TryAcceptCylinderHit = [&](double InT) -> void
        {
            if (InT < 0.0 || InT > InRayLength)
            { return; }

            const auto HitPoint = InRayOrigin + InRayDirection * InT;
            const auto AlongAxis = FVector::DotProduct(HitPoint - InCapA, AxisDirection);

            if (AlongAxis < 0.0 || AlongAxis * AlongAxis > AxisLengthSquared)
            { return; }

            if (NOT BestT.IsSet() || InT < *BestT)
            { BestT = InT; }
        };

        if (C <= 0.0 && OriginPerp.SizeSquared() <= InCapsuleRadius * InCapsuleRadius)
        {
            const auto AlongAxis = FVector::DotProduct(ToOrigin, AxisDirection);
            if (AlongAxis >= 0.0 && AlongAxis * AlongAxis <= AxisLengthSquared)
            { return 0.0; }
        }

        if (A > UE_DOUBLE_SMALL_NUMBER)
        {
            const auto Discriminant = B * B - A * C;

            if (Discriminant >= 0.0)
            {
                TryAcceptCylinderHit((-B - FMath::Sqrt(Discriminant)) / A);
            }
        }

        if (const auto CapHitA = Get_RayVsSphere(InRayOrigin, InRayDirection, InRayLength, InCapA, InCapsuleRadius);
            CapHitA.IsSet() && (NOT BestT.IsSet() || *CapHitA < *BestT))
        { BestT = CapHitA; }

        if (const auto CapHitB = Get_RayVsSphere(InRayOrigin, InRayDirection, InRayLength, InCapB, InCapsuleRadius);
            CapHitB.IsSet() && (NOT BestT.IsSet() || *CapHitB < *BestT))
        { BestT = CapHitB; }

        return BestT;
    }

    // Minkowski sum approximated by box inflation — corners are slightly 'square', conservative by design
    static auto
    Get_RayVsOrientedBox(
        const FVector& InRayOrigin,
        const FVector& InRayDirection,
        double InRayLength,
        const FTransform& InBoxTransform,
        const FVector& InBoxHalfExtents) -> TOptional<double>
    {
        const auto LocalOrigin = InBoxTransform.InverseTransformPositionNoScale(InRayOrigin);
        const auto LocalDirection = InBoxTransform.InverseTransformVectorNoScale(InRayDirection);

        auto TMin = 0.0;
        auto TMax = InRayLength;

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto Extent = InBoxHalfExtents[Axis];

            if (FMath::Abs(LocalDirection[Axis]) < UE_DOUBLE_SMALL_NUMBER)
            {
                if (FMath::Abs(LocalOrigin[Axis]) > Extent)
                { return {}; }

                continue;
            }

            const auto InverseDir = 1.0 / LocalDirection[Axis];
            auto T1 = (-Extent - LocalOrigin[Axis]) * InverseDir;
            auto T2 = (Extent - LocalOrigin[Axis]) * InverseDir;

            if (T1 > T2)
            { ::Swap(T1, T2); }

            TMin = FMath::Max(TMin, T1);
            TMax = FMath::Min(TMax, T2);

            if (TMin > TMax)
            { return {}; }
        }

        return TMin;
    }

    // UE convention: HalfHeight spans the full capsule including the caps; axis is local Z
    static auto
    Get_CapsuleSegment(
        const FTransform& InWorldTransform,
        double InHalfHeight,
        double InRadius) -> TPair<FVector, FVector>
    {
        const auto AxisHalfLength = FMath::Max(InHalfHeight - InRadius, 0.0);
        const auto Axis = InWorldTransform.GetUnitAxis(EAxis::Z) * AxisHalfLength;
        const auto Center = InWorldTransform.GetLocation();

        return TPair<FVector, FVector>{Center - Axis, Center + Axis};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::lag_comp
{
    auto
        Get_InterpolatedSnapshots(
            const FCk_LagComp_FrameBuffer& InFrames,
            FCk_Time InWorldTime)
        -> TArray<FCk_LagComp_HitShapeSnapshot>
    {
        if (InFrames.Get_Count() < 2)
        { return {}; }

        // Clamping a time older than the recorded window would invent poses that were never
        // recorded; the one-interval slack is for boundary queries
        const auto OldestTime = InFrames.Get_FrameAt(0).Get_WorldTime();
        const auto OldestInterval = InFrames.Get_FrameAt(1).Get_WorldTime() - OldestTime;

        if (InWorldTime < OldestTime - OldestInterval)
        { return {}; }

        auto AfterIndex = InFrames.Get_Count() - 1;

        for (auto Index = 0; Index < InFrames.Get_Count(); ++Index)
        {
            if (InFrames.Get_FrameAt(Index).Get_WorldTime() >= InWorldTime)
            {
                AfterIndex = Index;
                break;
            }
        }

        const auto BeforeIndex = FMath::Max(AfterIndex - 1, 0);
        AfterIndex = FMath::Max(AfterIndex, BeforeIndex + 1);

        const auto& Before = InFrames.Get_FrameAt(BeforeIndex);
        const auto& After = InFrames.Get_FrameAt(AfterIndex);

        const auto TimeSpan = (After.Get_WorldTime() - Before.Get_WorldTime()).Get_Seconds();
        const auto Alpha = TimeSpan < UE_DOUBLE_SMALL_NUMBER
            ? 0.0
            : FMath::Clamp((InWorldTime - Before.Get_WorldTime()).Get_Seconds() / TimeSpan, 0.0, 1.0);

        const auto SnapshotCount = FMath::Min(Before.Get_Snapshots().Num(), After.Get_Snapshots().Num());

        auto Interpolated = TArray<FCk_LagComp_HitShapeSnapshot>{};
        Interpolated.Reserve(SnapshotCount);

        for (auto Index = 0; Index < SnapshotCount; ++Index)
        {
            const auto& BeforeSnapshot = Before.Get_Snapshots()[Index];
            const auto& AfterSnapshot = After.Get_Snapshots()[Index];

            const auto& BeforeTM = BeforeSnapshot.Get_WorldTransform();
            const auto& AfterTM = AfterSnapshot.Get_WorldTransform();

            const auto BlendedTM = FTransform{
                FQuat::Slerp(BeforeTM.GetRotation(), AfterTM.GetRotation(), Alpha),
                FMath::Lerp(BeforeTM.GetLocation(), AfterTM.GetLocation(), Alpha),
                FMath::Lerp(BeforeTM.GetScale3D(), AfterTM.GetScale3D(), Alpha)};

            Interpolated.Emplace(BeforeSnapshot.Get_Label(), BeforeSnapshot.Get_Shape(), BlendedTM);
        }

        return Interpolated;
    }

    auto
        Sweep_SegmentVsSnapshot(
            const FVector& InSegmentStart,
            const FVector& InSegmentEnd,
            double InSweepRadius,
            const FCk_LagComp_HitShapeSnapshot& InShapeAtStart,
            const FCk_LagComp_HitShapeSnapshot& InShapeAtEnd)
        -> TOptional<FSweepResult>
    {
        const auto ShapeMotion = InShapeAtEnd.Get_WorldTransform().GetLocation() - InShapeAtStart.Get_WorldTransform().GetLocation();
        const auto AdjustedEnd = InSegmentEnd - ShapeMotion;

        const auto AdjustedDelta = AdjustedEnd - InSegmentStart;
        const auto RayLength = AdjustedDelta.Size();

        if (RayLength < UE_DOUBLE_SMALL_NUMBER)
        { return {}; }

        const auto RayDirection = AdjustedDelta / RayLength;
        const auto& ShapeTM = InShapeAtStart.Get_WorldTransform();
        const auto& Shape = InShapeAtStart.Get_Shape();

        auto HitT = TOptional<double>{};
        auto RestFrameNormal = FVector::ZeroVector;

        switch (Shape.Get_ShapeType())
        {
            case ECk_Shape_Type::Sphere:
            {
                const auto Radius = Shape.Get_Sphere().Get_Radius() + InSweepRadius;
                HitT = lag_comp_detail::Get_RayVsSphere(InSegmentStart, RayDirection, RayLength, ShapeTM.GetLocation(), Radius);

                if (HitT.IsSet())
                {
                    const auto HitPoint = InSegmentStart + RayDirection * (*HitT);
                    RestFrameNormal = (HitPoint - ShapeTM.GetLocation()).GetSafeNormal();
                }
                break;
            }
            case ECk_Shape_Type::Capsule:
            case ECk_Shape_Type::Cylinder:
            {
                // Cylinder is swept as its bounding capsule — conservative and analytic
                const auto HalfHeight = Shape.Get_ShapeType() == ECk_Shape_Type::Capsule
                    ? Shape.Get_Capsule().Get_HalfHeight()
                    : Shape.Get_Cylinder().Get_HalfHeight();
                const auto ShapeRadius = Shape.Get_ShapeType() == ECk_Shape_Type::Capsule
                    ? Shape.Get_Capsule().Get_Radius()
                    : Shape.Get_Cylinder().Get_Radius();

                const auto [CapA, CapB] = lag_comp_detail::Get_CapsuleSegment(ShapeTM, HalfHeight, ShapeRadius);
                const auto Radius = ShapeRadius + InSweepRadius;

                HitT = lag_comp_detail::Get_RayVsCapsule(InSegmentStart, RayDirection, RayLength, CapA, CapB, Radius);

                if (HitT.IsSet())
                {
                    const auto HitPoint = InSegmentStart + RayDirection * (*HitT);
                    const auto Axis = (CapB - CapA).GetSafeNormal();
                    const auto AlongAxis = FMath::Clamp(FVector::DotProduct(HitPoint - CapA, Axis), 0.0, (CapB - CapA).Size());
                    const auto ClosestOnAxis = CapA + Axis * AlongAxis;
                    RestFrameNormal = (HitPoint - ClosestOnAxis).GetSafeNormal();
                }
                break;
            }
            case ECk_Shape_Type::Box:
            {
                const auto InflatedExtents = FVector{Shape.Get_Box().Get_HalfExtents()} + FVector{InSweepRadius};
                HitT = lag_comp_detail::Get_RayVsOrientedBox(InSegmentStart, RayDirection, RayLength, ShapeTM, InflatedExtents);

                if (HitT.IsSet())
                {
                    const auto HitPoint = InSegmentStart + RayDirection * (*HitT);
                    const auto LocalHit = ShapeTM.InverseTransformPositionNoScale(HitPoint);
                    const auto Penetration = InflatedExtents - LocalHit.GetAbs();

                    auto LocalNormal = FVector::ZeroVector;
                    if (Penetration.X <= Penetration.Y && Penetration.X <= Penetration.Z)
                    { LocalNormal = FVector{FMath::Sign(LocalHit.X), 0.0, 0.0}; }
                    else if (Penetration.Y <= Penetration.Z)
                    { LocalNormal = FVector{0.0, FMath::Sign(LocalHit.Y), 0.0}; }
                    else
                    { LocalNormal = FVector{0.0, 0.0, FMath::Sign(LocalHit.Z)}; }

                    RestFrameNormal = ShapeTM.TransformVectorNoScale(LocalNormal);
                }
                break;
            }
            case ECk_Shape_Type::None:
            default:
            {
                return {};
            }
        }

        if (NOT HitT.IsSet())
        { return {}; }

        const auto Time01 = *HitT / RayLength;

        auto Result = FSweepResult{};
        Result._Time01 = Time01;
        // Report the hit where the projectile actually was in world space (un-adjusted segment)
        Result._Location = FMath::Lerp(InSegmentStart, InSegmentEnd, Time01);
        Result._Normal = RestFrameNormal;
        Result._HitShapeLabel = InShapeAtStart.Get_Label();

        return Result;
    }

    auto
        Sweep_SegmentVsHistory(
            const FCk_LagComp_FrameBuffer& InFrames,
            const FVector& InSegmentStart,
            const FVector& InSegmentEnd,
            FCk_Time InTimeStart,
            FCk_Time InTimeEnd,
            double InSweepRadius)
        -> TOptional<FCk_LagComp_RewindHit>
    {
        const auto SnapshotsAtStart = Get_InterpolatedSnapshots(InFrames, InTimeStart);
        const auto SnapshotsAtEnd = Get_InterpolatedSnapshots(InFrames, InTimeEnd);

        if (SnapshotsAtStart.IsEmpty() || SnapshotsAtStart.Num() != SnapshotsAtEnd.Num())
        { return {}; }

        auto TargetBounds = FBox{ForceInit};
        for (const auto& Snapshot : SnapshotsAtStart)
        { TargetBounds += Get_SnapshotWorldBounds(Snapshot); }
        for (const auto& Snapshot : SnapshotsAtEnd)
        { TargetBounds += Get_SnapshotWorldBounds(Snapshot); }

        auto SegmentBounds = FBox{ForceInit};
        SegmentBounds += InSegmentStart;
        SegmentBounds += InSegmentEnd;
        SegmentBounds = SegmentBounds.ExpandBy(InSweepRadius);

        if (NOT TargetBounds.Intersect(SegmentBounds))
        { return {}; }

        auto BestResult = TOptional<FSweepResult>{};

        for (auto Index = 0; Index < SnapshotsAtStart.Num(); ++Index)
        {
            const auto SweepResult = Sweep_SegmentVsSnapshot(
                InSegmentStart, InSegmentEnd, InSweepRadius, SnapshotsAtStart[Index], SnapshotsAtEnd[Index]);

            if (NOT SweepResult.IsSet())
            { continue; }

            if (NOT BestResult.IsSet() || SweepResult->_Time01 < BestResult->_Time01)
            { BestResult = SweepResult; }
        }

        if (NOT BestResult.IsSet())
        { return {}; }

        const auto HitWorldTime = InTimeStart + (InTimeEnd - InTimeStart) * BestResult->_Time01;

        return FCk_LagComp_RewindHit{
            BestResult->_HitShapeLabel, HitWorldTime, BestResult->_Location, BestResult->_Normal};
    }

    auto
        Get_SnapshotWorldBounds(
            const FCk_LagComp_HitShapeSnapshot& InSnapshot)
        -> FBox
    {
        const auto& ShapeTM = InSnapshot.Get_WorldTransform();
        const auto& Shape = InSnapshot.Get_Shape();
        const auto Center = ShapeTM.GetLocation();

        switch (Shape.Get_ShapeType())
        {
            case ECk_Shape_Type::Sphere:
            {
                const auto Radius = Shape.Get_Sphere().Get_Radius();
                return FBox{Center - FVector{Radius}, Center + FVector{Radius}};
            }
            case ECk_Shape_Type::Capsule:
            case ECk_Shape_Type::Cylinder:
            {
                const auto HalfHeight = Shape.Get_ShapeType() == ECk_Shape_Type::Capsule
                    ? Shape.Get_Capsule().Get_HalfHeight()
                    : Shape.Get_Cylinder().Get_HalfHeight();
                const auto Radius = Shape.Get_ShapeType() == ECk_Shape_Type::Capsule
                    ? Shape.Get_Capsule().Get_Radius()
                    : Shape.Get_Cylinder().Get_Radius();

                const auto [CapA, CapB] = lag_comp_detail::Get_CapsuleSegment(ShapeTM, HalfHeight, Radius);

                auto Bounds = FBox{ForceInit};
                Bounds += CapA;
                Bounds += CapB;
                return Bounds.ExpandBy(Radius);
            }
            case ECk_Shape_Type::Box:
            {
                const auto HalfExtents = FVector{Shape.Get_Box().Get_HalfExtents()};
                const auto RotationMatrix = FRotationMatrix::Make(ShapeTM.GetRotation());

                const auto WorldExtents = FVector{
                    FMath::Abs(RotationMatrix.M[0][0]) * HalfExtents.X + FMath::Abs(RotationMatrix.M[1][0]) * HalfExtents.Y + FMath::Abs(RotationMatrix.M[2][0]) * HalfExtents.Z,
                    FMath::Abs(RotationMatrix.M[0][1]) * HalfExtents.X + FMath::Abs(RotationMatrix.M[1][1]) * HalfExtents.Y + FMath::Abs(RotationMatrix.M[2][1]) * HalfExtents.Z,
                    FMath::Abs(RotationMatrix.M[0][2]) * HalfExtents.X + FMath::Abs(RotationMatrix.M[1][2]) * HalfExtents.Y + FMath::Abs(RotationMatrix.M[2][2]) * HalfExtents.Z};

                return FBox{Center - WorldExtents, Center + WorldExtents};
            }
            case ECk_Shape_Type::None:
            default:
            {
                return FBox{Center, Center};
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

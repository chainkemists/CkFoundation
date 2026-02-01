namespace Math
{
    const float32 PI = 3.1415926535897932;

    //--------------------------------------------------------------------------------------------------------------------------
    // BASIC ROTATION
    //--------------------------------------------------------------------------------------------------------------------------

    FRotator FindLookAtRotation(FVector InStart, FVector InTarget)
    {
        return FRotator::MakeFromX(InTarget - InStart);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // TRANSFORM
    //--------------------------------------------------------------------------------------------------------------------------

    FVector InverseTransformDirection(FTransform T, FVector Direction)
    {
        return T.InverseTransformVectorNoScale(Direction);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // DISTANCE
    //--------------------------------------------------------------------------------------------------------------------------

    float64 Distance(FVector InA, FVector InB)
    {
        return Math::Square(InB.X - InA.X) + Math::Square(InB.Y - InA.Y) + Math::Square(InB.Z - InA.Z);
    }

    float64 Distance(FVector2D InA, FVector2D InB)
    {
        return Math::Square(InB.X - InA.X) + Math::Square(InB.Y - InA.Y);
    }

    float64 Distance(FIntPoint InA, FIntPoint InB)
    {
        return Math::Square(InB.X - InA.X) + Math::Square(InB.Y - InA.Y);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // VECTOR ROTATION
    //--------------------------------------------------------------------------------------------------------------------------

    // Rotates a vector around an arbitrary axis by the specified angle in degrees.
    FVector RotateVectorAroundAxis(FVector InVector, FVector InAxis, float32 InAngleDegrees)
    {
        const auto AxisNorm = InAxis.GetSafeNormal();

        if (AxisNorm.IsNearlyZero())
        { return InVector; }

        const auto Rotation = FQuat(AxisNorm, Math::DegreesToRadians(InAngleDegrees));
        return Rotation.RotateVector(InVector);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // PROJECTION
    //--------------------------------------------------------------------------------------------------------------------------

    FVector ProjectToPlane(FVector InVector, ECk_Plane_Axis InPlane)
    {
        switch (InPlane)
        {
            case ECk_Plane_Axis::XY:
            {
                return FVector(InVector.X, InVector.Y, 0.0f);
            }
            case ECk_Plane_Axis::XZ:
            {
                return FVector(InVector.X, 0.0f, InVector.Z);
            }
            case ECk_Plane_Axis::YZ:
            {
                return FVector(0.0f, InVector.Y, InVector.Z);
            }
            default:
            {
                return InVector;
            }
        }
    }

    FVector Get_PlaneNormal(ECk_Plane_Axis InPlane)
    {
        switch (InPlane)
        {
            case ECk_Plane_Axis::XY:
            {
                return FVector::UpVector;
            }
            case ECk_Plane_Axis::XZ:
            {
                return FVector::RightVector;
            }
            case ECk_Plane_Axis::YZ:
            {
                return FVector::ForwardVector;
            }
            default:
            {
                return FVector::UpVector;
            }
        }
    }

    // Projects a point onto an infinite line defined by origin and direction.
    FVector ProjectPointOntoLine(FVector InPoint, FVector InLineOrigin, FVector InLineDirection)
    {
        const auto LineDir = InLineDirection.GetSafeNormal();

        if (LineDir.IsNearlyZero())
        { return InLineOrigin; }

        const auto OriginToPoint = InPoint - InLineOrigin;
        const auto ProjectedLength = float32(OriginToPoint.DotProduct(LineDir));

        return InLineOrigin + LineDir * ProjectedLength;
    }

    // Projects a point onto a line segment, clamping to segment endpoints.
    FVector ProjectPointOntoLineSegment(FVector InPoint, FVector InSegmentStart, FVector InSegmentEnd)
    {
        const auto Segment = InSegmentEnd - InSegmentStart;
        const auto SegmentLengthSq = float32(Segment.SizeSquared());

        if (SegmentLengthSq < KINDA_SMALL_NUMBER)
        { return InSegmentStart; }

        const auto StartToPoint = InPoint - InSegmentStart;
        const auto T = Math::Clamp(float32(StartToPoint.DotProduct(Segment)) / SegmentLengthSq, 0.0f, 1.0f);

        return InSegmentStart + Segment * T;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // SIGNED ANGLE
    //--------------------------------------------------------------------------------------------------------------------------

    // Computes the angle in degrees between two vectors around a specified axis.
    // Signed: Returns [-180, 180] where positive = counter-clockwise (looking down the axis).
    // Unsigned: Returns [0, 180] - the absolute angular difference.
    // InAxis should be the normal of the plane containing both vectors (e.g., UpVector for XY plane).
    float32 ComputeAngleDegrees(
        FVector InFrom,
        FVector InTo,
        FVector InAxis,
        ECk_SignedUnsigned InSignedness = ECk_SignedUnsigned::Signed)
    {
        const auto FromNorm = InFrom.GetSafeNormal();
        const auto ToNorm = InTo.GetSafeNormal();

        if (FromNorm.IsNearlyZero() || ToNorm.IsNearlyZero())
        { return 0.0f; }

        // Get unsigned angle via dot product
        const auto Dot = float32(FromNorm.DotProduct(ToNorm));
        const auto ClampedDot = Math::Clamp(Dot, -1.0f, 1.0f);
        auto AngleRadians = Math::Acos(ClampedDot);

        if (InSignedness == ECk_SignedUnsigned::Signed)
        {
            // Determine sign via cross product
            const auto Cross = FromNorm.CrossProduct(ToNorm);
            const auto Sign = float32(Cross.DotProduct(InAxis));

            if (Sign < 0.0f)
            {
                AngleRadians = -AngleRadians;
            }
        }

        return Math::RadiansToDegrees(AngleRadians);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // ANGLE CLAMPING
    //--------------------------------------------------------------------------------------------------------------------------

    // Clamps an angle relative to a rest angle within a specified range.
    // Returns the clamped angle in degrees (unwound to -180 to 180).
    float ClampAngleToRange(float InDesiredAngle, float InRestAngle, const FCk_FloatRange& InRange)
    {
        const auto DeltaFromRest = Math::FindDeltaAngleDegrees(InRestAngle, InDesiredAngle);
        const auto ClampedDelta = Math::Clamp(DeltaFromRest, InRange.Get_Min(), InRange.Get_Max());
        return Math::UnwindDegrees(InRestAngle + ClampedDelta);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // ANGLE INTERPOLATION
    //--------------------------------------------------------------------------------------------------------------------------

    // Interpolates from current angle towards desired angle at a given turn rate.
    // Returns the current angle if already within tolerance or if turn rate is zero.
    float InterpolateAngle(float InCurrentAngle, float InDesiredAngle, float InTurnRate, float InTolerance, float InDeltaTime)
    {
        if (InTurnRate <= 0.0f)
        { return InCurrentAngle; }

        const auto AngleDelta = Math::FindDeltaAngleDegrees(InCurrentAngle, InDesiredAngle);

        if (Math::Abs(AngleDelta) <= InTolerance)
        { return InCurrentAngle; }

        const auto MaxChange = InTurnRate * InDeltaTime;
        const auto ClampedDelta = Math::Clamp(AngleDelta, -MaxChange, MaxChange);

        return InCurrentAngle + ClampedDelta;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // ROTATION ANGLE (PITCH / YAW / ROLL)
    //--------------------------------------------------------------------------------------------------------------------------

    // Computes pitch, yaw, or roll angle (in degrees) from a reference location to a target location.
    // Pitch: vertical angle (positive = looking up, negative = looking down)
    // Yaw: horizontal angle projected to XY plane (positive = counter-clockwise from above)
    // Roll: not meaningful for point-to-point calculations, returns 0
    // Unsigned: returns absolute value - useful for FOV/cone checks where direction doesn't matter.
    float ComputeRotationAngle(
        const FVector& InRefLocation,
        const FVector& InForward,
        const FVector& InTargetLocation,
        ECk_PitchYawRoll InComponent,
        ECk_SignedUnsigned InSignedness = ECk_SignedUnsigned::Signed)
    {
        auto Angle = 0.0f;

        switch (InComponent)
        {
            case ECk_PitchYawRoll::Pitch:
            {
                const auto HorizontalDistance = FVector(
                    InTargetLocation.X - InRefLocation.X,
                    InTargetLocation.Y - InRefLocation.Y,
                    0.0f).Size();

                const auto VerticalDiff = InTargetLocation.Z - InRefLocation.Z;
                Angle = Math::RadiansToDegrees(Math::Atan2(float32(VerticalDiff), float32(HorizontalDistance)));
                break;
            }
            case ECk_PitchYawRoll::Yaw:
            {
                const auto DirectionToTarget = (InTargetLocation - InRefLocation).GetSafeNormal();
                const auto ProjectedForward = FVector(InForward.X, InForward.Y, 0.0f).GetSafeNormal();
                const auto ProjectedDirection = FVector(DirectionToTarget.X, DirectionToTarget.Y, 0.0f).GetSafeNormal();

                if (ProjectedForward.IsNearlyZero() || ProjectedDirection.IsNearlyZero())
                { return 0.0f; }

                Angle = Math::ComputeAngleDegrees(ProjectedForward, ProjectedDirection, FVector::UpVector, InSignedness);
                return Angle;
            }
            case ECk_PitchYawRoll::Roll:
            {
                return 0.0f;
            }
        }

        if (InSignedness == ECk_SignedUnsigned::Unsigned)
        { return Math::Abs(Angle); }

        return Angle;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // ANGLE ON PLANE
    //--------------------------------------------------------------------------------------------------------------------------

    // Computes the angle (in degrees) between a forward direction and a direction to target,
    // projected onto a plane defined by its normal.
    // Unsigned: returns absolute value - useful for FOV/cone checks where direction doesn't matter.
    float ComputeAngleOnPlane(
        const FVector& InRefLocation,
        const FVector& InForward,
        const FVector& InPlaneNormal,
        const FVector& InTargetLocation,
        ECk_SignedUnsigned InSignedness = ECk_SignedUnsigned::Signed)
    {
        const auto DirectionToTarget = (InTargetLocation - InRefLocation).GetSafeNormal();

        if (DirectionToTarget.IsNearlyZero() || InForward.IsNearlyZero())
        { return 0.0f; }

        return Math::ComputeAngleDegrees(InForward, DirectionToTarget, InPlaneNormal, InSignedness);
    }

    // Computes the angle on a plane, projecting both vectors to the plane first.
    // Unsigned: returns absolute value - useful for FOV/cone checks where direction doesn't matter.
    float ComputeAngleOnPlane(
        const FVector& InRefLocation,
        const FVector& InForward,
        ECk_Plane_Axis InPlaneAxis,
        const FVector& InTargetLocation,
        ECk_SignedUnsigned InSignedness = ECk_SignedUnsigned::Signed)
    {
        const auto DirectionToTarget = (InTargetLocation - InRefLocation).GetSafeNormal();
        const auto ProjectedDirection = Math::ProjectToPlane(DirectionToTarget, InPlaneAxis).GetSafeNormal();
        const auto ProjectedForward = Math::ProjectToPlane(InForward, InPlaneAxis).GetSafeNormal();

        if (ProjectedDirection.IsNearlyZero() || ProjectedForward.IsNearlyZero())
        { return 0.0f; }

        const auto PlaneNormal = Math::Get_PlaneNormal(InPlaneAxis);
        return Math::ComputeAngleDegrees(ProjectedForward, ProjectedDirection, PlaneNormal, InSignedness);
    }
}

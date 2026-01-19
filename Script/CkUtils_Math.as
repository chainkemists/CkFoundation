namespace Math
{
    const float32 PI = 3.1415926535897932;

    FRotator FindLookAtRotation(FVector InStart, FVector InTarget)
    {
        return FRotator::MakeFromX(InTarget - InStart);
    }

    FVector InverseTransformDirection(FTransform T, FVector Direction)
    {
        return T.InverseTransformVectorNoScale(Direction);
    }

    float64 Distance(FVector InA, FVector InB)
    {
        return Math::Square(InB.X-InA.X) + Math::Square(InB.Y-InA.Y) + Math::Square(InB.Z-InA.Z);
    }

    float64 Distance(FVector2D InA, FVector2D InB)
    {
        return Math::Square(InB.X-InA.X) + Math::Square(InB.Y-InA.Y);
    }

    float64 Distance(FIntPoint InA, FIntPoint InB)
    {
        return Math::Square(InB.X-InA.X) + Math::Square(InB.Y-InA.Y);
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // VECTOR ROTATION
    //--------------------------------------------------------------------------------------------------------------------------

    // Rotates a vector around an arbitrary axis by the specified angle in degrees
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

    FVector GetPlaneNormal(ECk_Plane_Axis InPlane)
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

    // Projects a point onto an infinite line defined by origin and direction
    FVector ProjectPointOntoLine(FVector InPoint, FVector InLineOrigin, FVector InLineDirection)
    {
        const auto LineDir = InLineDirection.GetSafeNormal();

        if (LineDir.IsNearlyZero())
        { return InLineOrigin; }

        const auto OriginToPoint = InPoint - InLineOrigin;
        const auto ProjectedLength = float32(OriginToPoint.DotProduct(LineDir));

        return InLineOrigin + LineDir * ProjectedLength;
    }

    // Projects a point onto a line segment, clamping to segment endpoints
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

    // Computes the signed angle in degrees between two vectors around a specified axis.
    // Returns a value in the range [-180, 180] where:
    //   - Positive angles indicate counter-clockwise rotation (when looking down the axis)
    //   - Negative angles indicate clockwise rotation
    // Useful for determining rotational direction, such as checking if a target is to the left or right of a forward vector.
    // InAxis should be the normal of the plane containing both vectors (e.g., UpVector for XY plane calculations).
    float32 ComputeSignedAngleDegrees(FVector InFrom, FVector InTo, FVector InAxis)
    {
        const auto FromNorm = InFrom.GetSafeNormal();
        const auto ToNorm = InTo.GetSafeNormal();

        if (FromNorm.IsNearlyZero() || ToNorm.IsNearlyZero())
        { return 0.0f; }

        // Step 1: Get unsigned angle via dot product
        const auto Dot = float32(FromNorm.DotProduct(ToNorm));
        const auto ClampedDot = Math::Clamp(Dot, -1.0f, 1.0f);
        auto AngleRadians = Math::Acos(ClampedDot);

        // Step 2: Determine sign via cross product
        // Cross product gives a vector perpendicular to both inputs
        // Its direction relative to the axis tells us rotation direction
        const auto Cross = FromNorm.CrossProduct(ToNorm);
        const auto Sign = float32(Cross.DotProduct(InAxis));

        if (Sign < 0.0f)
        {
            AngleRadians = -AngleRadians;
        }

        return Math::RadiansToDegrees(AngleRadians);
    }
}
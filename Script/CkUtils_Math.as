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
}
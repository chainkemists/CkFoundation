namespace Math
{
    FRotator FindLookAtRotation(FVector InStart, FVector InTarget)
    {
        return FRotator::MakeFromX(InTarget - InStart);
    }

    FVector InverseTransformDirection(FTransform T, FVector Direction)
    {
        return T.InverseTransformVectorNoScale(Direction);
    }
}
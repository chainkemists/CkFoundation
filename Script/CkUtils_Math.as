namespace Math
{
    FRotator FindLookAtRotation(FVector InStart, FVector InTarget)
    {
        return FRotator::MakeFromX(InTarget - InStart);
    }
}
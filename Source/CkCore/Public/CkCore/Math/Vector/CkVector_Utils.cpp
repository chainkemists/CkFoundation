#include "CkVector_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vector
{
    template <typename T>
    auto
        Length(
            const T& InVector)
        -> float
    {
        return InVector.Size();
    }

    template <typename T>
    auto
        LengthSquared(
            const T& InVector)
        -> float
    {
        return InVector.SizeSquared();
    }

    template <typename T>
    auto
        Length2DSquared(
            const T& InVector)
        -> float
    {
        return InVector.SizeSquared2D();
    }

    template <typename T>
    auto
        ClampLength(
            const T& InVector,
            const FCk_FloatRange& InClampRange)
        -> T
    {
        float VecLength;
        T DirectionVec;

        InVector.ToDirectionAndLength(DirectionVec, VecLength);

        const auto& ClampedLength = InClampRange.Get_ClampedValue(VecLength);

        return DirectionVec * ClampedLength;
    }

    template <typename T>
    auto
        Get_IsLengthGreaterThan(
            const T& InVector,
            float InLength)
        -> bool
    {
        return LengthSquared(InVector) > InLength * InLength;
    }

    template <typename T>
    auto
        Get_IsLengthLessThan(
            const T& InVector,
            float InLength)
        -> bool
    {
        return LengthSquared(InVector) < InLength * InLength;
    }

    template <typename T>
    auto
        Get_IsLengthGreaterThanOrEqualTo(
            const T& InVector,
            float InLength)
        -> bool
    {
        return LengthSquared(InVector) >= InLength * InLength;
    }

    template <typename T>
    auto
        Get_IsLengthLessThanOrEqualTo(
            const T& InVector,
            float InLength)
        -> bool
    {
        return LengthSquared(InVector) <= InLength * InLength;
    }

    template <typename T>
    auto
        Dot(
            const T& InVectorA,
            const T& InVectorB)
        -> float
    {
        return T::DotProduct(InVectorA, InVectorB);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vector3_UE::
    Get_AngleBetweenVectors(
        const FVector& InA,
        const FVector& InB)
    -> float
{
    return FMath::RadiansToDegrees(acosf(ck_vector::Dot(InA, InB)));
}

auto
    UCk_Utils_Vector3_UE::
    Get_HeadingAngle(
        const FVector& InVector)
    -> float
{
    return FMath::RadiansToDegrees(InVector.HeadingAngle());
}

auto
    UCk_Utils_Vector3_UE::
    Get_ClampedLength(
        const FVector&        InVector,
        const FCk_FloatRange& InClampRange)
    -> FVector
{
    return ck_vector::ClampLength(InVector, InClampRange);
}

auto
    UCk_Utils_Vector3_UE::
    Get_DistanceBetweenActors(
        const AActor* InA,
        const AActor* InB)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InA), TEXT("Unable to get distance between Actors. Actor InA is [{}]"), InA)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InB), TEXT("Unable to get distance between Actors. Actor InB is [{}]"), InB)
    { return {}; }

    return FVector::Distance(InA->GetActorLocation(), InB->GetActorLocation());
}

auto
    UCk_Utils_Vector3_UE::
    Get_Flattened(
        const FVector& InVector,
        ECk_Plane_Axis InAxis)
    -> FVector
{
    switch(InAxis)
    {
        case ECk_Plane_Axis::XY:
            return FVector(InVector.X, InVector.Y, 0.0f);
        case ECk_Plane_Axis::XZ:
            return FVector(InVector.X, 0.0f, InVector.Z);
        case ECk_Plane_Axis::YZ:
            return FVector(0.0f, InVector.Y, InVector.Z);
        default:
            CK_INVALID_ENUM(InAxis);
            return InVector;
    }
}

auto
    UCk_Utils_Vector3_UE::
    Get_FlattenedAndNormalized(
        const FVector& InVector,
        ECk_Plane_Axis InAxis)
    -> FVector
{
    auto V = Get_Flattened(InVector, InAxis);
    V.Normalize();
    return V;
}

auto
    UCk_Utils_Vector3_UE::
    Get_IsLengthGreaterThan(
        const FVector& InVector,
        float          InLength)
    -> bool
{
    return ck_vector::Get_IsLengthGreaterThan(InVector, InLength);
}

auto
    UCk_Utils_Vector3_UE::
    Get_IsLengthLessThan(
        const FVector& InVector,
        float          InLength)
    -> bool
{
    return ck_vector::Get_IsLengthLessThan(InVector, InLength);
}

auto
    UCk_Utils_Vector3_UE::
    Get_IsLengthGreaterThanOrEqualTo(
        const FVector& InVector,
        float          InLength)
    -> bool
{
    return ck_vector::Get_IsLengthGreaterThanOrEqualTo(InVector, InLength);
}

auto
    UCk_Utils_Vector3_UE::
    Get_IsLengthLessThanOrEqualTo(
        const FVector& InVector,
        float          InLength)
    -> bool
{
    return ck_vector::Get_IsLengthLessThanOrEqualTo(InVector, InLength);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vector2_UE::
    Get_AngleBetweenVectors(
        const FVector2D& InA,
        const FVector2D& InB)
    -> float
{
    return FMath::RadiansToDegrees(acosf(ck_vector::Dot(InA, InB)));
}

auto
    UCk_Utils_Vector2_UE::
    Get_ClampedLength(
        const FVector2D&      InVector,
        const FCk_FloatRange& InClampRange)
    -> FVector2D
{
    return ck_vector::ClampLength(InVector, InClampRange);
}

auto
    UCk_Utils_Vector2_UE::
    Get_IsLengthGreaterThan(
        const FVector2D& InVector,
        float            InLength)
    -> bool
{
    return ck_vector::Get_IsLengthGreaterThan(InVector, InLength);
}

auto
    UCk_Utils_Vector2_UE::
    Get_IsLengthLessThan(
        const FVector2D& InVector,
        float            InLength)
    -> bool
{
    return ck_vector::Get_IsLengthLessThan(InVector, InLength);
}

auto
    UCk_Utils_Vector2_UE::
    Get_IsLengthGreaterThanOrEqualTo(
        const FVector2D& InVector,
        float            InLength)
    -> bool
{
    return ck_vector::Get_IsLengthGreaterThanOrEqualTo(InVector, InLength);
}

auto
    UCk_Utils_Vector2_UE::
    Get_IsLengthLessThanOrEqualTo(
        const FVector2D& InVector,
        float            InLength)
    -> bool
{
    return ck_vector::Get_IsLengthLessThanOrEqualTo(InVector, InLength);
}

// --------------------------------------------------------------------------------------------------------------------

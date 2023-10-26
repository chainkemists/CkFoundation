#include "CkVector_Utils.h"

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
    ClampLength(
        const FVector&        InVector,
        const FCk_FloatRange& InClampRange)
    -> FVector
{
    return ck_vector::ClampLength(InVector, InClampRange);
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
    ClampLength(
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

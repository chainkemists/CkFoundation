#include "CkVector_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vector
{
    template <typename T>
    auto
        IsNormalized(
            const T& InVector)
        -> bool
    {
        return InVector.IsNormalized();
    }

    auto
        IsNormalized(
            const FVector2D& InVector)
        -> bool
    {
        return FMath::IsNearlyEqual(InVector.Length(), 1.0f);
    }

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
    Get_LocationFromOriginInDirection(
        const FVector& InOrigin,
        const FVector& InDirection,
        float          InDistanceFromOriginInDirection)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck_vector::IsNormalized(InDirection),
        TEXT("Direciton Vector [{}] is NOT normalized. Normalizing for you but this will NOT work in Shipping."),
        InDirection)
    {
        const auto SafeNormal = InDirection.GetSafeNormal();
        const auto CorrectedSafeNormal = SafeNormal.IsNearlyZero() ? FVector::ForwardVector : SafeNormal;
        return Get_LocationFromOriginInDirection(InOrigin, CorrectedSafeNormal, InDistanceFromOriginInDirection);
    }

    return InOrigin + (InDirection * InDistanceFromOriginInDirection);
}

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

auto
    UCk_Utils_Vector3_UE::
    Get_WorldDirection(
        ECk_Direction_3D InDirection)
    -> FVector
{
    switch (InDirection)
    {
        case ECk_Direction_3D::Left:
            return FVector::LeftVector;
        case ECk_Direction_3D::Right:
            return FVector::RightVector;
        case ECk_Direction_3D::Forward:
            return FVector::ForwardVector;
        case ECk_Direction_3D::Back:
            return FVector::BackwardVector;
        case ECk_Direction_3D::Up:
            return FVector::UpVector;
        case ECk_Direction_3D::Down:
            return FVector::DownVector;
        default:
            CK_INVALID_ENUM(InDirection);
            return {};
    }
}

auto
    UCk_Utils_Vector3_UE::
    Get_ClosestWorldDirectionFromVector(
        const FVector& InVector)
    -> ECk_Direction_3D
{
    const auto& NormalizedVector = InVector.GetSafeNormal();

    static const std::map<ECk_Direction_3D, FCk_FloatRange> DirectionThresholds =
    {
        { ECk_Direction_3D::Forward, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_Direction_3D::Right,   FCk_FloatRange(0.5f, 1.0f) },
        { ECk_Direction_3D::Left,    FCk_FloatRange(0.5f, 1.0f) },
        { ECk_Direction_3D::Up,      FCk_FloatRange(0.5f, 1.0f) },
        { ECk_Direction_3D::Down,    FCk_FloatRange(0.5f, 1.0f) },
        { ECk_Direction_3D::Back,    FCk_FloatRange(0.5f, 1.0f) }
    };

    const auto& FoundDirection = std::ranges::find_if
    (
        DirectionThresholds.begin(),
        DirectionThresholds.end(),
        [&](const std::pair<ECk_Direction_3D, FCk_FloatRange>& InKvp)
        {
            const auto& Direction3D        = InKvp.first;
            const auto& DirectionThreshold = InKvp.second;
            const auto& DotProduct         = FVector::DotProduct
            (
                NormalizedVector,
                Get_WorldDirection(Direction3D)
            );

            return DirectionThreshold.Get_IsWithinInclusive(DotProduct);
        }
    );

    CK_ENSURE_IF_NOT(FoundDirection != DirectionThresholds.end(),
        TEXT("Failed to find Direction for Vector [{}]"), InVector)
    { return ECk_Direction_3D::Forward; }

    return FoundDirection->first;
}

auto
    UCk_Utils_Vector3_UE::
    Get_DirectionAndLength(
        const FVector& InVelocity)
    -> FCk_DirectionAndLength
{
    auto Dir = FVector{};
    auto Length = float{};
    InVelocity.ToDirectionAndLength(Dir, Length);

    return FCk_DirectionAndLength{Dir, Length};
}

auto
    UCk_Utils_Vector3_UE::
    Get_DirectionAndLengthBetweenVectors(
        const FVector& InTo,
        const FVector& InFrom)
    -> FCk_DirectionAndLength
{
    const auto Vec = InTo - InFrom;

    return Get_DirectionAndLength(Vec);
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_DirectionVectorFromActor(
        const AActor*    InActor,
        ECk_Direction_3D InDirection)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Unable to get direction vector from Actor [{}] because it is INVALID"), InActor)
    { return {}; }

    switch (InDirection)
    {
        case ECk_Direction_3D::Forward:
        {
            return InActor->GetActorForwardVector();
        }
        case ECk_Direction_3D::Left:
        {
            return ck::Negate(InActor->GetActorRightVector());
        }
        case ECk_Direction_3D::Right:
        {
            return InActor->GetActorRightVector();
        }
        case ECk_Direction_3D::Back:
        {
            return ck::Negate(InActor->GetActorForwardVector());
        }
        case ECk_Direction_3D::Up:
        {
            return InActor->GetActorUpVector();
        }
        case ECk_Direction_3D::Down:
        {
            return ck::Negate(InActor->GetActorUpVector());
        }
        default:
        {
            CK_INVALID_ENUM(InDirection);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorVector3_UE::
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
    UCk_Utils_ActorVector3_UE::
    Get_DistanceFromActor(
        const AActor* InA,
        FVector       InLocation)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InA), TEXT("Unable to get distance between Actors. Actor InA is [{}]"), InA)
    { return {}; }

    return FVector::Distance(InA->GetActorLocation(), InLocation);
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_DirectionAndLengthBetweenActors(
        const AActor* InTo,
        const AActor* InFrom)
    -> FCk_DirectionAndLength
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTo) && ck::IsValid(InFrom),
        TEXT("Unable to calculation direction between Actor [{}] and Actor [{}] because one or both are INVALID"),
        InTo,
        InFrom)
    { return {}; }

    return UCk_Utils_Vector3_UE::Get_DirectionAndLengthBetweenVectors(InTo->GetActorLocation(), InFrom->GetActorLocation());
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_DirectionAndLengthFromActor(
        const FVector& InTo,
        const AActor*  InFrom)
    -> FCk_DirectionAndLength
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFrom),
        TEXT("Unable to calculation direction between Actor [{}] and Location [{}] because the Actor is INVALID"),
        InFrom, InTo)
    { return {}; }

    return UCk_Utils_Vector3_UE::Get_DirectionAndLengthBetweenVectors(InTo, InFrom->GetActorLocation());
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_DirectionAndLengthToActor(
        const AActor*  InTo,
        const FVector& InFrom)
    -> FCk_DirectionAndLength
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTo),
        TEXT("Unable to calculation direction between Actor [{}] and Location [{}] because the Actor is INVALID"),
        InTo, InFrom)
    { return {}; }

    return UCk_Utils_Vector3_UE::Get_DirectionAndLengthBetweenVectors(InTo->GetActorLocation(), InFrom);
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_LocationFromActorInDirection(
        const AActor* InActor,
        FVector InDirection,
        float InDistanceFromOriginInDirection)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Unable to get location from Actor in Direction [{}]. Actor is [{}]"),
        InDirection,
        InActor)
    { return {}; }

    const auto& ActorLocation = InActor->GetActorLocation();

    return UCk_Utils_Vector3_UE::Get_LocationFromOriginInDirection(
        ActorLocation, InDirection, InDistanceFromOriginInDirection);
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_LocationFromActorInFixedDirection(
        const AActor* InActor,
        ECk_Direction_3D InDirection,
        float InDistanceFromOriginInDirection)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Unable to get fixed location from Actor in Direction [{}]. Actor is [{}]"),
        InDirection,
        InActor)
    { return {}; }

    switch(InDirection)
    {
        case ECk_Direction_3D::Forward:
        {
            return Get_LocationFromActorInDirection(
                InActor, InActor->GetActorForwardVector(), InDistanceFromOriginInDirection);
        }
        case ECk_Direction_3D::Back:
        {
            return Get_LocationFromActorInDirection(
                InActor, ck::Negate(InActor->GetActorForwardVector()), InDistanceFromOriginInDirection);
        }
        case ECk_Direction_3D::Right:
        {
            return Get_LocationFromActorInDirection(
                InActor, InActor->GetActorRightVector(), InDistanceFromOriginInDirection);
        }
        case ECk_Direction_3D::Left:
        {
            return Get_LocationFromActorInDirection(
                InActor, ck::Negate(InActor->GetActorRightVector()), InDistanceFromOriginInDirection);
        }
        case ECk_Direction_3D::Up:
        {
            return Get_LocationFromActorInDirection(
                InActor, InActor->GetActorUpVector(), InDistanceFromOriginInDirection);
        }
        case ECk_Direction_3D::Down:
        {
            return Get_LocationFromActorInDirection(
                InActor, ck::Negate(InActor->GetActorUpVector()), InDistanceFromOriginInDirection);
        }
        default:
        {
            CK_INVALID_ENUM(InDirection);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vector2_UE::
    Get_LocationFromOriginInDirection(
        const FVector2D& InOrigin,
        const FVector2D& InDirection,
        float            InDistanceFromOriginInDirection)
    -> FVector2D
{
    CK_ENSURE_IF_NOT(ck_vector::IsNormalized(InDirection),
        TEXT("Direciton Vector [{}] is NOT normalized. Normalizing for you but this will NOT work in Shipping."),
        InDirection)
    {
        return Get_LocationFromOriginInDirection(InOrigin, InDirection.GetSafeNormal(), InDistanceFromOriginInDirection);
    }

    return InOrigin + (InDirection * InDistanceFromOriginInDirection);
}

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

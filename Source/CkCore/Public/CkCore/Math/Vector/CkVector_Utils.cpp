#include "CkVector_Utils.h"

#include "CkComparison_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include <Kismet/KismetMathLibrary.h>

#include <ranges>

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
    Get_DefaultWorldDirectionIfZero(
        const FVector&   InPotentiallyZeroVector,
        ECk_Direction_3D InDirectionToReturnIfZero)
    -> FVector
{
    if (NOT InPotentiallyZeroVector.IsNearlyZero())
    { return InPotentiallyZeroVector; }

    return Get_WorldDirection(InDirectionToReturnIfZero);
}

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
    Get_Swizzle(
        const FVector& InVector,
        ECk_Vector_Axis_Swizzle InOrder)
    -> FVector
{
    switch (InOrder)
    {
        case ECk_Vector_Axis_Swizzle::YXZ:
        {
            return FVector{InVector.Y, InVector.X, InVector.Z};
        }
        case ECk_Vector_Axis_Swizzle::ZYX:
        {
            return FVector{InVector.Z, InVector.Y, InVector.X};
        }
        case ECk_Vector_Axis_Swizzle::XZY:
        {
            return FVector{InVector.X, InVector.Z, InVector.Y};
        }
        case ECk_Vector_Axis_Swizzle::YZX:
        {
            return FVector{InVector.Y, InVector.Z, InVector.X};
        }
        case ECk_Vector_Axis_Swizzle::ZXY:
        {
            return FVector{InVector.Z, InVector.X, InVector.Y};
        }
        default:
        {
            CK_INVALID_ENUM(InOrder);
            return InVector;
        }
    }
}

auto
    UCk_Utils_Vector3_UE::
    SwizzleInplace(
        FVector& InVector,
        ECk_Vector_Axis_Swizzle InOrder)
    -> void
{
    InVector = Get_Swizzle(InVector, InOrder);
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
        {
            return FVector::LeftVector;
        }
        case ECk_Direction_3D::Right:
        {
            return FVector::RightVector;
        }
        case ECk_Direction_3D::Forward:
        {
            return FVector::ForwardVector;
        }
        case ECk_Direction_3D::Back:
        {
            return FVector::BackwardVector;
        }
        case ECk_Direction_3D::Up:
        {
            return FVector::UpVector;
        }
        case ECk_Direction_3D::Down:
        {
            return FVector::DownVector;
        }
        default:
        {
            CK_INVALID_ENUM(InDirection);
            return {};
        }
    }
}

auto
    UCk_Utils_Vector3_UE::
    Get_ClosestWorldDirection(
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
            const auto& Direction          = InKvp.first;
            const auto& DirectionThreshold = InKvp.second;
            const auto& DotProduct         = ck_vector::Dot(NormalizedVector, Get_WorldDirection(Direction));

            return DirectionThreshold.Get_IsWithinInclusive(DotProduct);
        }
    );

    CK_ENSURE_IF_NOT(FoundDirection != DirectionThresholds.end(),
        TEXT("Failed to find World Direction for Vector [{}]"), InVector)
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
    UCk_Utils_Vector3_UE::
    Get_IsDotInsideRange(
        const FVector& InA,
        const FVector& InB,
        FCk_Comparison_FloatRange InAngleBetween)
    -> bool
{
    const auto Dot = FVector::DotProduct(InA, InB);
    const float DotAngle = FMath::RadiansToDegrees(FMath::Acos(Dot));
    return UCk_Utils_FloatComparison_UE::Get_IsInRange(DotAngle, InAngleBetween);
}

auto
    UCk_Utils_Vector3_UE::
    Get_IsInFrontOf(
        const FVector& InA,
        const FVector& InB)
    -> bool
{
    return Get_IsDotInsideRange(InA, InB, FCk_Comparison_FloatRange
    {
        0.0f,
        ECk_ComparisonOperators::GreaterThanOrEqualTo,
        ECk_Logic_And_Or::And,
        ECk_ComparisonOperators::GreaterThanOrEqualTo,
        0.0f
    });
}

auto
    UCk_Utils_Vector3_UE::
    Get_IsBehindOf(
        const FVector& InA,
        const FVector& InB)
    -> bool
{
    return NOT Get_IsInFrontOf(InA, InB);
}

auto
    UCk_Utils_Vector3_UE::
    Get_Rotator(
        const FVector& InDirectionVector)
    -> FRotator
{
    if (InDirectionVector.IsNearlyZero())
    { return {}; }

    return UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, InDirectionVector);
}

auto
    UCk_Utils_Vector3_UE::
    Get_DirectionVector(
        const FRotator&  InRotator,
        ECk_Direction_3D InVectorToRotate)
    -> FVector
{
    const auto& Direction = Get_WorldDirection(InVectorToRotate);
    return InRotator.RotateVector(Direction);
}

auto
    UCk_Utils_Vector3_UE::
    Get_CardinalAndOrdinalDirection(
        ECk_CardinalAndOrdinalDirection InDirection,
        ECk_Plane_Axis                  InAxis)
    -> FVector
{
    const auto& Vec2d = UCk_Utils_Vector2_UE::Get_CardinalAndOrdinalDirection(InDirection);

    switch(InAxis)
    {
        case ECk_Plane_Axis::XY: return FVector(Vec2d.X, Vec2d.Y, 0.0f);
        case ECk_Plane_Axis::XZ: return FVector(Vec2d.X, 0.0f, Vec2d.Y);
        case ECk_Plane_Axis::YZ: return FVector(0.0f, Vec2d.Y, Vec2d.X);
    }

    return {};
}

auto
    UCk_Utils_Vector3_UE::
    Get_OrdinalDirection(
        ECk_OrdinalDirection InDirection,
        ECk_Plane_Axis       InAxis)
    -> FVector
{
    const auto& Vec2d = UCk_Utils_Vector2_UE::Get_OrdinalDirection(InDirection);

    switch(InAxis)
    {
        case ECk_Plane_Axis::XY: return FVector(Vec2d.X, Vec2d.Y, 0.0f);
        case ECk_Plane_Axis::XZ: return FVector(Vec2d.X, 0.0f, Vec2d.Y);
        case ECk_Plane_Axis::YZ: return FVector(0.0f, Vec2d.Y, Vec2d.X);
    }

    return {};
}

auto
    UCk_Utils_Vector3_UE::
    Get_CardinalDirection(
        ECk_CardinalDirection InDirection,
        ECk_Plane_Axis        InAxis)
    -> FVector
{
    const auto& Vec2d = UCk_Utils_Vector2_UE::Get_CardinalDirection(InDirection);

    switch(InAxis)
    {
        case ECk_Plane_Axis::XY: return FVector(Vec2d.X, Vec2d.Y, 0.0f);
        case ECk_Plane_Axis::XZ: return FVector(Vec2d.X, 0.0f, Vec2d.Y);
        case ECk_Plane_Axis::YZ: return FVector(0.0f, Vec2d.Y, Vec2d.X);
    }

    return {};
}

auto
    UCk_Utils_Vector3_UE::
    Get_ClosestCardinalDirection(
        const FVector& InVector,
        ECk_Plane_Axis InAxis)
    -> ECk_CardinalDirection
{
    switch(InAxis)
    {
        case ECk_Plane_Axis::XY: return UCk_Utils_Vector2_UE::Get_ClosestCardinalDirection(FVector2D(InVector.X, InVector.Y));
        case ECk_Plane_Axis::XZ: return UCk_Utils_Vector2_UE::Get_ClosestCardinalDirection(FVector2D(InVector.X, InVector.Z));
        case ECk_Plane_Axis::YZ: return UCk_Utils_Vector2_UE::Get_ClosestCardinalDirection(FVector2D(InVector.Y, InVector.Z));
    }

    return {};
}

auto
    UCk_Utils_Vector3_UE::
    Get_ClosestOrdinalDirection(
        const FVector& InVector,
        ECk_Plane_Axis InAxis)
    -> ECk_OrdinalDirection
{
    switch(InAxis)
    {
        case ECk_Plane_Axis::XY: return UCk_Utils_Vector2_UE::Get_ClosestOrdinalDirection(FVector2D(InVector.X, InVector.Y));
        case ECk_Plane_Axis::XZ: return UCk_Utils_Vector2_UE::Get_ClosestOrdinalDirection(FVector2D(InVector.X, InVector.Z));
        case ECk_Plane_Axis::YZ: return UCk_Utils_Vector2_UE::Get_ClosestOrdinalDirection(FVector2D(InVector.Y, InVector.Z));
    }

    return {};
}

auto
    UCk_Utils_Vector3_UE::
    Get_ClosestCardinalAndOrdinalDirection(
        const FVector& InVector,
        ECk_Plane_Axis InAxis)
    -> ECk_CardinalAndOrdinalDirection
{
    switch(InAxis)
    {
        case ECk_Plane_Axis::XY: return UCk_Utils_Vector2_UE::Get_ClosestCardinalAndOrdinalDirection(FVector2D(InVector.X, InVector.Y));
        case ECk_Plane_Axis::XZ: return UCk_Utils_Vector2_UE::Get_ClosestCardinalAndOrdinalDirection(FVector2D(InVector.X, InVector.Z));
        case ECk_Plane_Axis::YZ: return UCk_Utils_Vector2_UE::Get_ClosestCardinalAndOrdinalDirection(FVector2D(InVector.Y, InVector.Z));
    }

    return {};
}

auto
    UCk_Utils_Vector3_UE::
    Get_AddNoise(
        const FVector& InVector,
        FVector InNoiseHalfRange)
    -> FVector
{
    const auto& HalfRangeX = InNoiseHalfRange.X;
    const auto& HalfRangeY = InNoiseHalfRange.Y;
    const auto& HalfRangeZ = InNoiseHalfRange.Z;

    return FVector
    {
        InVector.X + FMath::RandRange(-HalfRangeX, HalfRangeX),
        InVector.Y + FMath::RandRange(-HalfRangeY, HalfRangeY),
        InVector.Z + FMath::RandRange(-HalfRangeZ, HalfRangeZ)
    };
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
    -> FCk_LocationResultWithActorLocation
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Unable to get location from Actor in Direction [{}]. Actor is [{}]"),
        InDirection,
        InActor)
    { return {}; }

    const auto& ActorLocation = InActor->GetActorLocation();
    const auto& Result = FCk_LocationResultWithActorLocation{UCk_Utils_Vector3_UE::Get_LocationFromOriginInDirection(
        ActorLocation, InDirection, InDistanceFromOriginInDirection), ActorLocation};

    return Result;
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_LocationFromActorInFixedDirection(
        const AActor* InActor,
        ECk_Direction_3D InDirection,
        float InDistanceFromOriginInDirection)
    -> FCk_LocationResultWithActorLocation
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

auto
    UCk_Utils_ActorVector3_UE::
    Get_IsFrontOf(
        const AActor* InA,
        const AActor* InB)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InA),
        TEXT("Unable to calculate whether B is ahead of A. Actor A is [{}]"),
        InA)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InB),
        TEXT("Unable to calculate whether B is ahead of A. Actor B is [{}]"),
        InA)
    { return {}; }

    return UCk_Utils_Vector3_UE::Get_IsInFrontOf(InA->GetActorForwardVector(), InB->GetActorLocation() - InA->GetActorLocation());
}

auto
    UCk_Utils_ActorVector3_UE::
    Get_IsBehindOf(
        const AActor* InA,
        const AActor* InB)
    -> bool
{
    return NOT Get_IsFrontOf(InA, InB);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vector2_UE::
    Get_DefaultDirectionIfZero(
        const FVector2D& InPotentiallyZeroVector,
        ECk_Direction_2D InDirectionToReturnIfZero)
    -> FVector2D
{
    if (NOT InPotentiallyZeroVector.IsNearlyZero())
    { return InPotentiallyZeroVector; }

    return Get_ScreenDirection(InDirectionToReturnIfZero);
}

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

auto
    UCk_Utils_Vector2_UE::
    Get_Swizzle(
        const FVector2D& InVector)
    -> FVector2D
{
    return FVector2D{InVector.Y, InVector.X};
}

auto
    UCk_Utils_Vector2_UE::
    SwizzleInplace(
        FVector2D& InVector)
    -> void
{
    InVector = Get_Swizzle(InVector);
}

auto
    UCk_Utils_Vector2_UE::
    Get_ScreenDirection(
        ECk_Direction_2D InDirection)
    -> FVector2D
{
    switch(InDirection)
    {
        case ECk_Direction_2D::Left: return FVector2D(0, -1);
        case ECk_Direction_2D::Right: return FVector2D(0, 1);
        case ECk_Direction_2D::Up: return FVector2D(1, 0);
        case ECk_Direction_2D::Down: return FVector2d(-1, 0);
    }

    return {};
}

auto
    UCk_Utils_Vector2_UE::
    Get_CardinalAndOrdinalDirection(
        ECk_CardinalAndOrdinalDirection InDirection)
    -> FVector2D
{
    switch (InDirection)
    {
        case ECk_CardinalAndOrdinalDirection::North:
        {
            return FVector2D{1.0f, 0.0f};
        }
        case ECk_CardinalAndOrdinalDirection::NorthEast:
        {
            return FVector2D{0.5f, 0.5f};
        }
        case ECk_CardinalAndOrdinalDirection::East:
        {
            return FVector2D{0.0f, 1.0f};
        }
        case ECk_CardinalAndOrdinalDirection::SouthEast:
        {
            return FVector2D{-0.5f, 0.5f};
        }
        case ECk_CardinalAndOrdinalDirection::South:
        {
            return FVector2D{-1.0f, 0.0f};
        }
        case ECk_CardinalAndOrdinalDirection::SouthWest:
        {
            return FVector2D{-0.5f, -0.5f};
        }
        case ECk_CardinalAndOrdinalDirection::West:
        {
            return FVector2D{0.0f, -1.0f};
        }
        case ECk_CardinalAndOrdinalDirection::NorthWest:
        {
            return FVector2D{0.5f, -0.5f};
        }
        default:
        {
            CK_INVALID_ENUM(InDirection);
            return {};
        }
    }
}

auto
    UCk_Utils_Vector2_UE::
    Get_OrdinalDirection(
        ECk_OrdinalDirection InDirection)
    -> FVector2D
{
    switch (InDirection)
    {
        case ECk_OrdinalDirection::NorthEast:
        {
            return FVector2D{0.5f, 0.5f};
        }
        case ECk_OrdinalDirection::SouthEast:
        {
            return FVector2D{-0.5f, 0.5f};
        }
        case ECk_OrdinalDirection::SouthWest:
        {
            return FVector2D{-0.5f, -0.5f};
        }
        case ECk_OrdinalDirection::NorthWest:
        {
            return FVector2D{0.5f, -0.5f};
        }
        default:
        {
            CK_INVALID_ENUM(InDirection);
            return {};
        }
    }
}

auto
    UCk_Utils_Vector2_UE::
    Get_CardinalDirection(
        ECk_CardinalDirection InDirection)
    -> FVector2D
{
    switch (InDirection)
    {
        case ECk_CardinalDirection::North:
        {
            return FVector2D{1.0f, 0.0f};
        }
        case ECk_CardinalDirection::East:
        {
            return FVector2D{0.0f, 1.0f};
        }
        case ECk_CardinalDirection::South:
        {
            return FVector2D{-1.0f, 0.0f};
        }
        case ECk_CardinalDirection::West:
        {
            return FVector2D{0.0f, -1.0f};
        }
        default:
        {
            CK_INVALID_ENUM(InDirection);
            return {};
        }
    }
}

auto
    UCk_Utils_Vector2_UE::
    Get_ClosestCardinalDirection(
        const FVector2D& InVector)
    -> ECk_CardinalDirection
{
    const auto& NormalizedVector = InVector.GetSafeNormal();

    static const std::map<ECk_CardinalDirection, FCk_FloatRange> DirectionThresholds =
    {
        { ECk_CardinalDirection::North, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalDirection::East,  FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalDirection::South, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalDirection::West,  FCk_FloatRange(0.5f, 1.0f) },
    };

    const auto& FoundDirection = std::ranges::find_if
    (
        DirectionThresholds.begin(),
        DirectionThresholds.end(),
        [&](const std::pair<ECk_CardinalDirection, FCk_FloatRange>& InKvp)
        {
            const auto& Direction          = InKvp.first;
            const auto& DirectionThreshold = InKvp.second;
            const auto& DotProduct         = ck_vector::Dot(NormalizedVector, Get_CardinalDirection(Direction));

            return DirectionThreshold.Get_IsWithinInclusive(DotProduct);
        }
    );

    CK_ENSURE_IF_NOT(FoundDirection != DirectionThresholds.end(),
        TEXT("Failed to find Cardinal Direction for Vector [{}]"), InVector)
    { return ECk_CardinalDirection::North; }

    return FoundDirection->first;
}

auto
    UCk_Utils_Vector2_UE::
    Get_ClosestOrdinalDirection(
        const FVector2D& InVector)
    -> ECk_OrdinalDirection
{
    const auto& NormalizedVector = InVector.GetSafeNormal();

    static const std::map<ECk_OrdinalDirection, FCk_FloatRange> DirectionThresholds =
    {
        { ECk_OrdinalDirection::NorthEast, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_OrdinalDirection::SouthEast, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_OrdinalDirection::SouthWest, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_OrdinalDirection::NorthWest, FCk_FloatRange(0.5f, 1.0f) },
    };

    const auto& FoundDirection = std::ranges::find_if
    (
        DirectionThresholds.begin(),
        DirectionThresholds.end(),
        [&](const std::pair<ECk_OrdinalDirection, FCk_FloatRange>& InKvp)
        {
            const auto& Direction          = InKvp.first;
            const auto& DirectionThreshold = InKvp.second;
            const auto& DotProduct         = ck_vector::Dot(NormalizedVector, Get_OrdinalDirection(Direction));

            return DirectionThreshold.Get_IsWithinInclusive(DotProduct);
        }
    );

    CK_ENSURE_IF_NOT(FoundDirection != DirectionThresholds.end(),
        TEXT("Failed to find Ordinal Direction for Vector [{}]"), InVector)
    { return ECk_OrdinalDirection::NorthEast; }

    return FoundDirection->first;
}

auto
    UCk_Utils_Vector2_UE::
    Get_ClosestCardinalAndOrdinalDirection(
        const FVector2D& InVector)
    -> ECk_CardinalAndOrdinalDirection
{
    const auto& NormalizedVector = InVector.GetSafeNormal();

    static const std::map<ECk_CardinalAndOrdinalDirection, FCk_FloatRange> DirectionThresholds =
    {
        { ECk_CardinalAndOrdinalDirection::North,     FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::NorthEast, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::East,      FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::SouthEast, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::South,     FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::SouthWest, FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::West,      FCk_FloatRange(0.5f, 1.0f) },
        { ECk_CardinalAndOrdinalDirection::NorthWest, FCk_FloatRange(0.5f, 1.0f) },
    };

    const auto& FoundDirection = std::ranges::find_if
    (
        DirectionThresholds.begin(),
        DirectionThresholds.end(),
        [&](const std::pair<ECk_CardinalAndOrdinalDirection, FCk_FloatRange>& InKvp)
        {
            const auto& Direction          = InKvp.first;
            const auto& DirectionThreshold = InKvp.second;
            const auto& DotProduct         = ck_vector::Dot(NormalizedVector, Get_CardinalAndOrdinalDirection(Direction));

            return DirectionThreshold.Get_IsWithinInclusive(DotProduct);
        }
    );

    CK_ENSURE_IF_NOT(FoundDirection != DirectionThresholds.end(),
        TEXT("Failed to find Cardinal and Ordinal Direction for Vector [{}]"), InVector)
    { return ECk_CardinalAndOrdinalDirection::North; }

    return FoundDirection->first;
}

// --------------------------------------------------------------------------------------------------------------------

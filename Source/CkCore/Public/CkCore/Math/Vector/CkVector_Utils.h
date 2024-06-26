#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkVector_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Comparison_FloatRange;

USTRUCT(BlueprintType)
struct CKCORE_API FCk_DirectionAndLength
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DirectionAndLength);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FVector _Direction = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    float _Length = 0.0f;

public:
    CK_PROPERTY_GET(_Direction);
    CK_PROPERTY_GET(_Length);

    CK_DEFINE_CONSTRUCTORS(FCk_DirectionAndLength, _Direction, _Length);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_LocationResultWithActorLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LocationResultWithActorLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FVector _Result;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FVector _ActorLocation;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LocationResultWithActorLocation, _Result, _ActorLocation);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Vector3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vector3_UE);

public:
    // Assumes +X is Forward, +Y is Right, +Z is Up
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Default Direction If Zero (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_DefaultWorldDirectionIfZero(
        const FVector& InPotentiallyZeroVector,
        ECk_Direction_3D InDirectionToReturnIfZero);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Location From Origin In Direction (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_LocationFromOriginInDirection(
        const FVector& InOrigin,
        const FVector& InDirection,
        float InDistanceFromOriginInDirection = 500.0f);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Angle Between Vectors (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static float
    Get_AngleBetweenVectors(
        const FVector& InA,
        const FVector& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Heading Angle",
              Category = "Ck|Utils|Math|Vector3")
    static float
    Get_HeadingAngle(
        const FVector& InVector);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Clamped Length (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_ClampedLength(
        const FVector& InVector,
        const FCk_FloatRange& InClampRange);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Flattened",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_Flattened(
        const FVector& InVector,
        ECk_Plane_Axis InAxis);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Flattened And Normalized",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_FlattenedAndNormalized(
        const FVector& InVector,
        ECk_Plane_Axis InAxis);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Greater Than (Vec3)",
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = ">"))
    static bool
    Get_IsLengthGreaterThan(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Less Than (Vec3)",
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = "<"))
    static bool
    Get_IsLengthLessThan(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Greater Than Or Equal To (Vec3)",
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = ">="))
    static bool
    Get_IsLengthGreaterThanOrEqualTo(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Less Than Or Equal To (Vec3)",
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = "<="))
    static bool
    Get_IsLengthLessThanOrEqualTo(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Swizzle (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_Swizzle(
        const FVector& InVector,
        ECk_Vector_Axis_Swizzle InOrder);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Swizzle Inplace (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static void
    SwizzleInplace(
        UPARAM(ref) FVector& InVector,
        ECk_Vector_Axis_Swizzle InOrder);

    // Assumes +X is Forward, +Y is Right, +Z is Up
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get World Direction",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_WorldDirection(
        ECk_Direction_3D InDirection);

    // Assumes +X is Forward, +Y is Right, +Z is Up
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest World Direction",
              Category = "Ck|Utils|Math|Vector3")
    static ECk_Direction_3D
    Get_ClosestWorldDirection(
        const FVector& InVector);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length",
              Category = "Ck|Utils|Math|Vector3")
    static FCk_DirectionAndLength
    Get_DirectionAndLength(
        const FVector& InVelocity);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length Between Vectors",
              Category = "Ck|Utils|Math|Vector3")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthBetweenVectors(
        const FVector& InTo,
        const FVector& InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Dot Inside Range",
              Category = "Ck|Utils|Math|Vector3")
    static bool
    Get_IsDotInsideRange(
        const FVector& InA,
        const FVector& InB,
        FCk_Comparison_FloatRange InAngleBetween);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Vector In Front Of",
              Category = "Ck|Utils|Math|Vector3")
    static bool
    Get_IsInFrontOf(
        const FVector& InA,
        const FVector& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Vector Behind Of",
              Category = "Ck|Utils|Math|Vector3")
    static bool
    Get_IsBehindOf(
        const FVector& InA,
        const FVector& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Rotator",
              Category = "Ck|Utils|Math|Vector3")
    static FRotator
    Get_Rotator(
        const FVector& InDirectionVector);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction Vector",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_DirectionVector(
        const FRotator& InRotator,
        ECk_Direction_3D InVectorToRotate = ECk_Direction_3D::Forward);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Cardinal & Ordinal Direction",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_CardinalAndOrdinalDirection(
        ECk_CardinalAndOrdinalDirection InDirection,
        ECk_Plane_Axis InAxis = ECk_Plane_Axis::XY);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Ordinal Direction",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_OrdinalDirection(
        ECk_OrdinalDirection InDirection,
        ECk_Plane_Axis InAxis = ECk_Plane_Axis::XY);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Cardinal Direction",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_CardinalDirection(
        ECk_CardinalDirection InDirection,
        ECk_Plane_Axis InAxis = ECk_Plane_Axis::XY);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Cardinal Direction",
              Category = "Ck|Utils|Math|Vector3")
    static ECk_CardinalDirection
    Get_ClosestCardinalDirection(
        const FVector& InVector,
        ECk_Plane_Axis InAxis = ECk_Plane_Axis::XY);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Ordinal Direction",
              Category = "Ck|Utils|Math|Vector3")
    static ECk_OrdinalDirection
    Get_ClosestOrdinalDirection(
        const FVector& InVector,
        ECk_Plane_Axis InAxis = ECk_Plane_Axis::XY);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Cardinal & Ordinal Direction",
              Category = "Ck|Utils|Math|Vector3")
    static ECk_CardinalAndOrdinalDirection
    Get_ClosestCardinalAndOrdinalDirection(
        const FVector& InVector,
        ECk_Plane_Axis InAxis = ECk_Plane_Axis::XY);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Add Noise",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_AddNoise(
        const FVector& InVector,
        FVector InNoiseHalfRange);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_ActorVector3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vector3_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction Vector From Actor",
              Category = "Ck|Utils|Math|Actor")
    static FVector
    Get_DirectionVectorFromActor(
         const AActor* InActor,
         ECk_Direction_3D InDirection = ECk_Direction_3D::Forward);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Distance Between Actors",
              Category = "Ck|Utils|Math|Actor")
    static float
    Get_DistanceBetweenActors(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Distance To/From Actor",
              Category = "Ck|Utils|Math|Actor")
    static float
    Get_DistanceFromActor(
        const AActor* InA,
        FVector InLocation);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length (Between Actors)",
              Category = "Ck|Utils|Math|Actor")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthBetweenActors(
        const AActor* InTo,
        const AActor* InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length (From Actor)",
              Category = "Ck|Utils|Math|Actor")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthFromActor(
        const FVector& InTo,
        const AActor* InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length (To Actor)",
              Category = "Ck|Utils|Math|Actor")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthToActor(
        const AActor* InTo,
        const FVector& InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Location From Actor In Direction",
              Category = "Ck|Utils|Math|Actor")
    static FCk_LocationResultWithActorLocation
    Get_LocationFromActorInDirection(
        const AActor* InActor,
        FVector InDirection,
        float InDistanceFromOriginInDirection = 500.0f);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Location From Actor In Fixed Direction",
              Category = "Ck|Utils|Math|Actor")
    static FCk_LocationResultWithActorLocation
    Get_LocationFromActorInFixedDirection(
        const AActor* InActor,
        ECk_Direction_3D InDirection = ECk_Direction_3D::Forward,
        float InDistanceFromOriginInDirection = 500.0f);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Actor In Front Of",
              Category = "Ck|Utils|Math|Actor")
    static bool
    Get_IsFrontOf(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Actor Behind Of",
              Category = "Ck|Utils|Math|Actor")
    static bool
    Get_IsBehindOf(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Dot Product Between Actors",
              Category = "Ck|Utils|Math|Actor")
    static float
    Get_DotProductBetweenActors(
         const AActor* InA,
         const AActor* InB,
         ECk_Direction_3D InDirectionToUseForA = ECk_Direction_3D::Forward);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Dot Product Between Actors Based on Direction",
              Category = "Ck|Utils|Math|Actor")
    static float
    Get_DotProductBetweenActorsBasedOnDirection(
         const AActor* InA,
         const AActor* InB,
         const FVector& InDirectionToUseForA);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Vector2_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vector2_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Default Direction If Zero (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_DefaultDirectionIfZero(
        const FVector2D& InPotentiallyZeroVector,
        ECk_Direction_2D InDirectionToReturnIfZero);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Location From Origin In Direction (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_LocationFromOriginInDirection(
        const FVector2D& InOrigin,
        const FVector2D& InDirection,
        float InDistanceFromOriginInDirection);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Angle Between Vectors (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static float
    Get_AngleBetweenVectors(
        const FVector2D& InA,
        const FVector2D& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Clamped Length (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_ClampedLength(
        const FVector2D& InVector,
        const FCk_FloatRange& InClampRange);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Greater Than (Vec2)",
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = ">"))
    static bool
    Get_IsLengthGreaterThan(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Less Than (Vec2)",
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = "<"))
    static bool
    Get_IsLengthLessThan(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Greater Than Or Equal To (Vec2)",
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = ">="))
    static bool
    Get_IsLengthGreaterThanOrEqualTo(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Length Less Than Or Equal To (Vec2)",
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = "<="))
    static bool
    Get_IsLengthLessThanOrEqualTo(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Swizzle (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_Swizzle(
        const FVector2D& InVector);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Swizzle Inplace (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static void
    SwizzleInplace(
        UPARAM(ref) FVector2D& InVector);

    // Assumes +X is Up, +Y is Right
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Screen Direction",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_ScreenDirection(
        ECk_Direction_2D InDirection);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Cardinal & Ordinal Direction",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_CardinalAndOrdinalDirection(
        ECk_CardinalAndOrdinalDirection InDirection);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Ordinal Direction",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_OrdinalDirection(
        ECk_OrdinalDirection InDirection);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Cardinal Direction",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_CardinalDirection(
        ECk_CardinalDirection InDirection);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Cardinal Direction",
              Category = "Ck|Utils|Math|Vector2")
    static ECk_CardinalDirection
    Get_ClosestCardinalDirection(
        const FVector2D& InVector);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Ordinal Direction",
              Category = "Ck|Utils|Math|Vector2")
    static ECk_OrdinalDirection
    Get_ClosestOrdinalDirection(
        const FVector2D& InVector);

    // Assumes +X is North, +Y is East
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Cardinal & Ordinal Direction",
              Category = "Ck|Utils|Math|Vector2")
    static ECk_CardinalAndOrdinalDirection
    Get_ClosestCardinalAndOrdinalDirection(
        const FVector2D& InVector);
};

// --------------------------------------------------------------------------------------------------------------------

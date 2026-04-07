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
    FVector _Result = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FVector _ActorLocation = FVector::ZeroVector;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_LocationResultWithActorLocation, _Result, _ActorLocation);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FVector"))
class CKCORE_API UCk_Utils_Vector3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vector3_UE);

public:
    /**
     * Determines which local-space direction a point is closest to, given a transform.
     * 
     * @param InOrigin The origin point defining the center of the frame of reference
     * @param InPoint The point to evaluate
     * @param InPlanarBias -1.0 = favor vertical, 0.0 = balanced, +1.0 = favor horizontal
     * @param InLocalToWorldTransform Transform defining the local space orientation
     * @return The closest local direction (Forward/Back/Left/Right/Up/Down)
     */
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Local Direction To Point",
              Category = "Ck|Utils|Math|Vector3")
    static ECk_Direction_3D
    Get_ClosestLocalDirectionToPoint(
        const FVector& InOrigin,
        const FVector& InPoint,
        FCk_FloatRange_Minus1to1 InPlanarBias,
        FTransform InLocalToWorldTransform);

    /**
     * Like Get_ClosestLocalDirectionToPoint, but accepts a direction vector
     * instead of a point. Useful when you already have a velocity, look vector,
     * or impact normal and need to discretize it.
     *
     * @param InDirection The world-space direction to evaluate (does NOT need to be normalized)
     * @param InPlanarBias Controls bias between vertical axis and horizontal plane:
     *                     -1.0 = Heavily favor vertical (Up/Down), ignore horizontal
     *                      0.0 = Balanced (all directions weighted equally) [DEFAULT]
     *                     +1.0 = Heavily favor horizontal (Forward/Back/Left/Right), ignore vertical
     * @param InLocalToWorldTransform Transform defining the local space orientation
     * @return The closest local direction (Forward/Back/Left/Right/Up/Down)
     */
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Local Direction To Direction",
              Category = "Ck|Utils|Math|Vector3")
    static ECk_Direction_3D
    Get_ClosestLocalDirectionToDirection(
        const FVector& InDirection,
        FCk_FloatRange_Minus1to1 InPlanarBias,
        FTransform InLocalToWorldTransform);

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
              DisplayName = "[Ck] Get Angle Between Vectors (Vec3 | Degrees)",
              Category = "Ck|Utils|Math|Vector3")
    static float
    Get_AngleBetweenVectors_Degrees(
        const FVector& InA,
        const FVector& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Angle Between Vectors (Vec3 | Radians)",
              Category = "Ck|Utils|Math|Vector3")
    static float
    Get_AngleBetweenVectors_Radians(
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
              DisplayName = "[Ck] Get Plane Axis Rotation",
              Category = "Ck|Utils|Math|Vector3")
    static FQuat
    Get_PlaneAxisRotation(
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

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Any Axis Nearly Zero (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static bool
    Get_IsAnyAxisNearlyZero(
        const FVector& InVector);

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

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Point in Radius",
              Category = "Ck|Utils|Math|Vector3")
    static bool
    Get_IsPointInRadius(
        const FVector& InPoint,
        const FVector& InMeasureRadiusFrom,
        float InRadius);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Unit Vector In Cone (Normal Distribution | Degrees)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_RandCone_NormalDistribution_Degrees(
        const FVector& InDirection,
        float InConeHalfAngleDegrees,
        float InExponent);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Unit Vector In Cone (Normal Distribution | Radians)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_RandCone_NormalDistribution_Radians(
        const FVector& InDirection,
        float InConeHalfAngleRadians,
        float InExponent);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Unit Vector (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_RandomUnitVector();

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Vector In Range (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_RandomVectorInRange(
        const FCk_FloatRange& InRangeX,
        const FCk_FloatRange& InRangeY,
        const FCk_FloatRange& InRangeZ);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Jitter (Vec3)",
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_Jitter(
        const FVector& InVector,
        const FCk_FloatRange& InJitterRangeX,
        const FCk_FloatRange& InJitterRangeY,
        const FCk_FloatRange& InJitterRangeZ);

    // C++-only (not UFUNCTION) — rotates two parallel arrays of vertices and
    // normals in-place by the rotation for the given plane axis. Used by CkPmg
    // processors to apply axis rotations to generated mesh data. Not exposed
    // to Blueprint because the in/out TArray<FVector>& signature is awkward in
    // BP and the parallel-array semantics are too specific to be generally
    // useful there.
    static void
    ApplyPlaneAxisRotation(
        TArray<FVector>& InOutVertices,
        TArray<FVector>& InOutNormals,
        ECk_Plane_Axis InAxis);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_ActorVector3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vector3_UE);

public:
    /**
     * Determines which local direction of InActor the point InPoint is closest to.
     * @param InActor The actor whose local space defines the frame of reference
     * @param InPoint The world-space point to evaluate
     * @param InPlanarBias -1.0 = favor vertical, 0.0 = balanced, +1.0 = favor horizontal
     * @return The closest local direction (Forward/Back/Left/Right/Up/Down)
     */
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Local Direction To Point (Actor)",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static ECk_Direction_3D
    Get_ClosestLocalDirectionToPoint(
        const AActor* InActor,
        const FVector& InPoint,
        FCk_FloatRange_Minus1to1 InPlanarBias);

    /**
     * Determines which local direction of InActor another actor (InOther) is closest to.
     *
     * @param InActor The actor whose local space defines the frame of reference
     * @param InOther The actor whose location is evaluated against InActor's local directions
     * @param InPlanarBias -1.0 = favor vertical, 0.0 = balanced, +1.0 = favor horizontal
     * @return The closest local direction (Forward/Back/Left/Right/Up/Down)
     */
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Closest Local Direction To Actor",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static ECk_Direction_3D
    Get_ClosestLocalDirectionToActor(
        const AActor* InActor,
        const AActor* InOther,
        FCk_FloatRange_Minus1to1 InPlanarBias);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction Vector From Actor",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static FVector
    Get_DirectionVectorFromActor(
         const AActor* InActor,
         ECk_Direction_3D InDirection = ECk_Direction_3D::Forward);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Distance Between Actors",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static float
    Get_DistanceBetweenActors(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Distance To/From Actor",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static float
    Get_DistanceFromActor(
        const AActor* InA,
        FVector InLocation);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length (Between Actors)",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthBetweenActors(
        const AActor* InTo,
        const AActor* InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length (From Actor)",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthFromActor(
        const FVector& InTo,
        const AActor* InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Direction And Length (To Actor)",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static FCk_DirectionAndLength
    Get_DirectionAndLengthToActor(
        const AActor* InTo,
        const FVector& InFrom);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Location From Actor In Direction",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static FCk_LocationResultWithActorLocation
    Get_LocationFromActorInDirection(
        const AActor* InActor,
        FVector InDirection,
        float InDistanceFromOriginInDirection = 500.0f);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Location From Actor In Fixed Direction",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static FCk_LocationResultWithActorLocation
    Get_LocationFromActorInFixedDirection(
        const AActor* InActor,
        ECk_Direction_3D InDirection = ECk_Direction_3D::Forward,
        float InDistanceFromOriginInDirection = 500.0f);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Actor In Front Of",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static bool
    Get_IsFrontOf(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Actor Behind Of",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static bool
    Get_IsBehindOf(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Dot Product Between Actors",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static float
    Get_DotProductBetweenActors(
         const AActor* InA,
         const AActor* InB,
         ECk_Direction_3D InDirectionToUseForA = ECk_Direction_3D::Forward);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Dot Product Between Actors Based on Direction",
              Category = "Ck|Utils|Math|Vector3|Actor")
    static float
    Get_DotProductBetweenActorsBasedOnDirection(
         const AActor* InA,
         const AActor* InB,
         const FVector& InDirectionToUseForA);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FVector2D"))
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
              DisplayName = "[Ck] Get Angle Between Vectors (Vec2 | Degrees)",
              Category = "Ck|Utils|Math|Vector2")
    static float
    Get_AngleBetweenVectors_Degrees(
        const FVector2D& InA,
        const FVector2D& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Angle Between Vectors (Vec2 | Radians)",
              Category = "Ck|Utils|Math|Vector2")
    static float
    Get_AngleBetweenVectors_Radians(
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

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Any Axis Nearly Zero (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static bool
    Get_IsAnyAxisNearlyZero(
        const FVector2D& InVector);

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

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Point in Radius",
              Category = "Ck|Utils|Math|Vector2")
    static bool
    Get_IsPointInRadius(
        const FVector2D& InPoint,
        const FVector2D& InMeasureRadiusFrom,
        float InRadius);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Unit Vector (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_RandomUnitVector();

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Random Vector In Range (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_RandomVectorInRange(
        const FCk_FloatRange& InRangeX,
        const FCk_FloatRange& InRangeY);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Jitter (Vec2)",
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_Jitter(
        const FVector2D& InVector,
        const FCk_FloatRange& InJitterRangeX,
        const FCk_FloatRange& InJitterRangeY);
};

// --------------------------------------------------------------------------------------------------------------------

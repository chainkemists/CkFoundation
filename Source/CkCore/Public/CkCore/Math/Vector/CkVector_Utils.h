#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkVector_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Vector3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Vector3_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_LocationFromOriginInDirection(
        const FVector& InOrigin,
        const FVector& InDirection,
        float InDistanceFromOriginInDirection = 500.0f);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static float
    Get_AngleBetweenVectors(
        const FVector& InA,
        const FVector& InB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static float
    Get_HeadingAngle(
        const FVector& InVector);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_ClampedLength(
        const FVector& InVector,
        const FCk_FloatRange& InClampRange);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_Flattened(
        const FVector& InVector,
        ECk_Plane_Axis InAxis);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_FlattenedAndNormalized(
        const FVector& InVector,
        ECk_Plane_Axis InAxis);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = ">"))
    static bool
    Get_IsLengthGreaterThan(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = "<"))
    static bool
    Get_IsLengthLessThan(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = ">="))
    static bool
    Get_IsLengthGreaterThanOrEqualTo(
        const FVector& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3",
              meta     = (KeyWords = "<="))
    static bool
    Get_IsLengthLessThanOrEqualTo(
        const FVector& InVector,
        float InLength);

    // Assumes +X is Forward, +Y is Right, +Z is Up
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static FVector
    Get_WorldDirection(
        ECk_Direction_3D InDirection);

    // Assumes +X is Forward, +Y is Right, +Z is Up
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector3")
    static ECk_Direction_3D
    Get_ClosestWorldDirectionFromVector(
        const FVector& InVector);
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
              Category = "Ck|Utils|Math|Actor")
    static float
    Get_DistanceBetweenActors(
        const AActor* InA,
        const AActor* InB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Actor")
    static FVector
    Get_LocationFromActorInDirection(
        const AActor* InActor,
        ECk_Direction_3D InDirection = ECk_Direction_3D::Forward,
        float InDistanceFromOriginInDirection = 500.0f);
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
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_LocationFromOriginInDirection(
        const FVector2D& InOrigin,
        const FVector2D& InDirection,
        float InDistanceFromOriginInDirection);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector2")
    static float
    Get_AngleBetweenVectors(
        const FVector2D& InA,
        const FVector2D& InB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector2")
    static FVector2D
    Get_ClampedLength(
        const FVector2D& InVector,
        const FCk_FloatRange& InClampRange);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = ">"))
    static bool
    Get_IsLengthGreaterThan(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = "<"))
    static bool
    Get_IsLengthLessThan(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = ">="))
    static bool
    Get_IsLengthGreaterThanOrEqualTo(
        const FVector2D& InVector,
        float InLength);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|Vector2",
              meta     = (KeyWords = "<="))
    static bool
    Get_IsLengthLessThanOrEqualTo(
        const FVector2D& InVector,
        float InLength);
};

// --------------------------------------------------------------------------------------------------------------------

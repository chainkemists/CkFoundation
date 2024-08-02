#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkGeometry_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_LineSegment
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LineSegment);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _LineStart = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _LineEnd = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_LineStart);
    CK_PROPERTY_GET(_LineEnd);

    CK_DEFINE_CONSTRUCTORS(FCk_LineSegment, _LineStart, _LineEnd);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_LineSegment2D
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_LineSegment2D);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _LineStart = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _LineEnd = FVector2D::ZeroVector;

public:
    CK_PROPERTY_GET(_LineStart);
    CK_PROPERTY_GET(_LineEnd);

    CK_DEFINE_CONSTRUCTORS(FCk_LineSegment2D, _LineStart, _LineEnd);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Geometry_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Geometry_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Project Box3D To Screen",
              Category = "Ck|Utils|Math|Geometry")
    static FBox2D
    Project_Box_ToScreen(
        APlayerController* InPlayerController,
        const FBox& InBox3D);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box3D Vertices",
              Category = "Ck|Utils|Math|Geometry")
    static TArray<FVector>
    Get_Box_Vertices(
        const FBox& InBox3D);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box3D Edges",
              Category = "Ck|Utils|Math|Geometry")
    static TArray<FCk_LineSegment>
    Get_Box_Edges(
        const FBox& InBox3D);

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Box (With Center And Extents)",
              Category = "Ck|Utils|Math|Geometry")
    static void
    Break_Box_WithCenterAndExtents(
        const FBox& InBox,
        FVector& OutMin,
        FVector& OutMax,
        FVector& OutCenter,
        FVector& InExtents,
        bool& OutIsValidBox);

public:
    static auto
    ForEach_BoxEdges(
        const FBox& InBox,
        TFunction<void(const FCk_LineSegment&)> InFunc) -> ECk_SucceededFailed;

    static auto
    ForEach_BoxVertices(
        const FBox& InBox,
        TFunction<void(const FVector&)> InFunc) -> ECk_SucceededFailed;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Geometry2D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Geometry2D_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Vertices",
              Category = "Ck|Utils|Math|Geometry2D")
    static TArray<FVector2D>
    Get_Box_Vertices(
        const FBox2D& InBox2D);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Box2D Edges",
              Category = "Ck|Utils|Math|Geometry2D")
    static TArray<FCk_LineSegment2D>
    Get_Box_Edges(
        const FBox2D& InBox2D);

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Box2D (With Center And Extents)",
              Category = "Ck|Utils|Math|Geometry2D")
    static void
    Break_Box_WithCenterAndExtents(
        const FBox2D& InBox,
        FVector2D& OutMin,
        FVector2D& OutMax,
        FVector2D& OutCenter,
        FVector2D& InExtents,
        bool& OutIsValidBox);

public:
    static auto
    ForEach_BoxEdges(
        const FBox2D& InBox,
        TFunction<void(const FCk_LineSegment2D&)> InFunc) -> ECk_SucceededFailed;

    static auto
    ForEach_BoxVertices(
        const FBox2D& InBox,
        TFunction<void(const FVector2D&)> InFunc) -> ECk_SucceededFailed;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Geometry_Actor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Geometry_Actor_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Actor Bounds (By Component Class)",
              Category = "Ck|Utils|Math|Geometry|Actor")
    static FBox
    Get_ActorBounds_ByComponentClass(
        AActor* InActor,
        TSubclassOf<USceneComponent> InComponentToAllow,
        bool InIncludeFromChildActors = true);
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkGeometry_Utils.generated.h"

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
    ProjectBox3D_ToScreen(
        APlayerController* InPlayerController,
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

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Box2D (With Center And Extents)",
              Category = "Ck|Utils|Math|Geometry")
    static void
    Break_Box2D_WithCenterAndExtents(
        const FBox2D& InBox,
        FVector2D& OutMin,
        FVector2D& OutMax,
        FVector2D& OutCenter,
        FVector2D& InExtents,
        bool& OutIsValidBox);
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

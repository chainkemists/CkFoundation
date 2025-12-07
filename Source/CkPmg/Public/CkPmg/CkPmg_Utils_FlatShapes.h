#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPmg_Utils_FlatShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPMG_API UCk_Utils_Pmg_FlatShapes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_FlatShapes);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Circle")
    static FCk_Handle_Pmg_DebugShape
    Add_Circle(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        bool InDrawDirectionLine = false,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Triangle")
    static FCk_Handle_Pmg_DebugShape
    Add_Triangle(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Plane")
    static FCk_Handle_Pmg_DebugShape
    Add_Plane(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InWidth = 100.0f,
        float InHeight = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Ring")
    static FCk_Handle_Pmg_DebugShape
    Add_Ring(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InOuterRadius = 100.0f,
        float InInnerRadius = 50.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Cross")
    static FCk_Handle_Pmg_DebugShape
    Add_Cross(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Star")
    static FCk_Handle_Pmg_DebugShape
    Add_Star(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InOuterRadius = 100.0f,
        int32 InPoints = 5,
        float InInnerRadiusRatio = 0.5f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Checkmark")
    static FCk_Handle_Pmg_DebugShape
    Add_Checkmark(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Diamond")
    static FCk_Handle_Pmg_DebugShape
    Add_Diamond(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Circle")
    static FCk_Handle_Pmg_DebugShape
    Create_Circle(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        bool InDrawDirectionLine = false,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Triangle")
    static FCk_Handle_Pmg_DebugShape
    Create_Triangle(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Plane")
    static FCk_Handle_Pmg_DebugShape
    Create_Plane(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InWidth = 100.0f,
        float InHeight = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Ring")
    static FCk_Handle_Pmg_DebugShape
    Create_Ring(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InOuterRadius = 100.0f,
        float InInnerRadius = 50.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Cross")
    static FCk_Handle_Pmg_DebugShape
    Create_Cross(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Star")
    static FCk_Handle_Pmg_DebugShape
    Create_Star(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InOuterRadius = 100.0f,
        int32 InPoints = 5,
        float InInnerRadiusRatio = 0.5f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Checkmark")
    static FCk_Handle_Pmg_DebugShape
    Create_Checkmark(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Diamond")
    static FCk_Handle_Pmg_DebugShape
    Create_Diamond(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Circle",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        bool InDrawDirectionLine = false,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Triangle",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledTriangle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Plane",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledPlane(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InWidth = 100.0f,
        float InHeight = 100.0f,
        FRotator InRotation = FRotator(0, 0, 0),
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Ring",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledRing(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InOuterRadius = 100.0f,
        float InInnerRadius = 50.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Cross",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledCross(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Star",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledStar(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InOuterRadius = 100.0f,
        int32 InPoints = 5,
        float InInnerRadiusRatio = 0.5f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Checkmark",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledCheckmark(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Diamond",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledDiamond(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize = 50.0f,
        float InThickness = 5.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);
};

// --------------------------------------------------------------------------------------------------------------------

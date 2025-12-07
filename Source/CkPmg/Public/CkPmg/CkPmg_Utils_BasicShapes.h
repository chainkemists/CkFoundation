#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPmg_Utils_BasicShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPMG_API UCk_Utils_Pmg_BasicShapes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_BasicShapes);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Sphere")
    static FCk_Handle_Pmg_DebugShape
    Add_Sphere(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Box")
    static FCk_Handle_Pmg_DebugShape
    Add_Box(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        FVector InExtent = FVector(100.0f, 100.0f, 100.0f),
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Cone")
    static FCk_Handle_Pmg_DebugShape
    Add_Cone(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        int32 InSegments = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Cylinder")
    static FCk_Handle_Pmg_DebugShape
    Add_Cylinder(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        int32 InSegments = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Capsule")
    static FCk_Handle_Pmg_DebugShape
    Add_Capsule(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 50.0f,
        float InHalfHeight = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 8,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Pyramid")
    static FCk_Handle_Pmg_DebugShape
    Add_Pyramid(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InBaseSize = 100.0f,
        float InHeight = 100.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Hemisphere")
    static FCk_Handle_Pmg_DebugShape
    Add_Hemisphere(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 8,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Torus")
    static FCk_Handle_Pmg_DebugShape
    Add_Torus(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InMajorRadius = 100.0f,
        float InMinorRadius = 25.0f,
        int32 InMajorSegments = 32,
        int32 InMinorSegments = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Sphere")
    static FCk_Handle_Pmg_DebugShape
    Create_Sphere(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Box")
    static FCk_Handle_Pmg_DebugShape
    Create_Box(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        FVector InExtent = FVector(100.0f, 100.0f, 100.0f),
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Cone")
    static FCk_Handle_Pmg_DebugShape
    Create_Cone(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        int32 InSegments = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Cylinder")
    static FCk_Handle_Pmg_DebugShape
    Create_Cylinder(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        int32 InSegments = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Capsule")
    static FCk_Handle_Pmg_DebugShape
    Create_Capsule(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 50.0f,
        float InHalfHeight = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 8,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Pyramid")
    static FCk_Handle_Pmg_DebugShape
    Create_Pyramid(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InBaseSize = 100.0f,
        float InHeight = 100.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Hemisphere")
    static FCk_Handle_Pmg_DebugShape
    Create_Hemisphere(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 8,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Torus")
    static FCk_Handle_Pmg_DebugShape
    Create_Torus(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InMajorRadius = 100.0f,
        float InMinorRadius = 25.0f,
        int32 InMajorSegments = 32,
        int32 InMinorSegments = 16,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Sphere",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledSphere(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 16,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Box",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledBox(
        const UObject* InWorldContextObject,
        FVector InCenter,
        FVector InExtent = FVector(100.0f, 100.0f, 100.0f),
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Cone",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledCone(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        int32 InSegments = 16,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Cylinder",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledCylinder(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        int32 InSegments = 16,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Capsule",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledCapsule(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 50.0f,
        float InHalfHeight = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 8,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Pyramid",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledPyramid(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InBaseSize = 100.0f,
        float InHeight = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Hemisphere",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledHemisphere(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 16,
        int32 InRings = 8,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Torus",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledTorus(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InMajorRadius = 100.0f,
        float InMinorRadius = 25.0f,
        int32 InMajorSegments = 32,
        int32 InMinorSegments = 16,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPmg_Utils_AngularShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPMG_API UCk_Utils_Pmg_AngularShapes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_AngularShapes);

public:
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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Wedge")
    static FCk_Handle_Pmg_DebugShape
    Add_Wedge(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Arc")
    static FCk_Handle_Pmg_DebugShape
    Add_Arc(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        float InThickness = 5.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Frustum")
    static FCk_Handle_Pmg_DebugShape
    Add_Frustum(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength = 200.0f,
        float InNearWidth = 50.0f,
        float InNearHeight = 50.0f,
        float InFarWidth = 200.0f,
        float InFarHeight = 200.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Wedge Cone")
    static FCk_Handle_Pmg_DebugShape
    Add_WedgeCone(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        int32 InSegments = 16,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Pyramid")
    static FCk_Handle_Pmg_DebugShape
    Add_Pyramid(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InBaseSize = 100.0f,
        float InHeight = 200.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Wedge")
    static FCk_Handle_Pmg_DebugShape
    Create_Wedge(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Arc")
    static FCk_Handle_Pmg_DebugShape
    Create_Arc(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        float InThickness = 5.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Frustum")
    static FCk_Handle_Pmg_DebugShape
    Create_Frustum(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength = 200.0f,
        float InNearWidth = 50.0f,
        float InNearHeight = 50.0f,
        float InFarWidth = 200.0f,
        float InFarHeight = 200.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Wedge Cone")
    static FCk_Handle_Pmg_DebugShape
    Create_WedgeCone(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        int32 InSegments = 16,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Pyramid")
    static FCk_Handle_Pmg_DebugShape
    Create_Pyramid(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InBaseSize = 100.0f,
        float InHeight = 200.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
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
        FVector InOrigin,
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
              DisplayName="[Ck][Debug] Draw Filled Wedge",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledWedge(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Arc",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledArc(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        float InThickness = 5.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Frustum",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledFrustum(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InLength = 200.0f,
        float InNearWidth = 50.0f,
        float InNearHeight = 50.0f,
        float InFarWidth = 200.0f,
        float InFarHeight = 200.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Wedge Cone",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledWedgeCone(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius = 100.0f,
        float InHeight = 200.0f,
        float InStartAngle = 0.0f,
        float InEndAngle = 90.0f,
        int32 InSegments = 16,
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
        FVector InOrigin,
        float InBaseSize = 100.0f,
        float InHeight = 200.0f,
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
};

// --------------------------------------------------------------------------------------------------------------------

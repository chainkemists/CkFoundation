#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkScene_Utils.generated.h"

// ----------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_Utils_Scene_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Scene_UE);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Draw Point (Visualizer)",
              Category = "Ck|Utils|Scene")
    static void
    DrawPoint(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InPosition,
        FLinearColor InColor,
        float InPointSize,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Draw Line (Visualizer)",
              Category = "Ck|Utils|Scene")
    static void
    DrawLine(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InStart,
        FVector InEnd,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness = 0.0f,
        float InDepthBias = 0.0f,
        bool InIsScreenSpace = false);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Draw Box (Visualizer)",
              Category = "Ck|Utils|Scene")
    static void
    DrawBox(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        FRotator InRotation,
        FVector InExtents,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness = 0.0f,
        float InDepthBias = 0.0f,
        bool InIsScreenSpace = false);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Draw Sphere (Visualizer)",
              Category = "Ck|Utils|Scene")
    static void
    DrawSphere(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        float InRadius,
        int32 InNumSides,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness = 0.0f,
        float InDepthBias = 0.0f,
        bool InIsScreenSpace = false);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Draw Capsule (Visualizer)",
              Category = "Ck|Utils|Scene")
    static void
    DrawCapsule(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius,
        float InHalfHeight,
        int32 InNumSides,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness = 0.0f,
        float InDepthBias = 0.0f,
        bool InIsScreenSpace = false);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Draw Cylinder (Visualizer)",
              Category = "Ck|Utils|Scene")
    static void
    DrawCylinder(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius,
        float InHalfHeight,
        int32 InNumSides,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness = 0.0f,
        float InDepthBias = 0.0f,
        bool InIsScreenSpace = false);
};

// ----------------------------------------------------------------------------------------------------------------;
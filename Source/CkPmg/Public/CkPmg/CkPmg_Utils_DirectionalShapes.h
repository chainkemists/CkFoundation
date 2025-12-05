#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPmg_Utils_DirectionalShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPMG_API UCk_Utils_Pmg_DirectionalShapes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_DirectionalShapes);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Arrow")
    static FCk_Handle_Pmg_DebugShape
    Add_Arrow(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength = 200.0f,
        float InShaftWidth = 20.0f,
        float InArrowHeadRatio = 0.3f,
        float InArrowHeadWidthMultiplier = 2.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Pivot")
    static FCk_Handle_Pmg_DebugShape
    Add_Pivot(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InAxisLength = 100.0f,
        float InArrowSize = 10.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Dashed Line")
    static FCk_Handle_Pmg_DebugShape
    Add_DashedLine(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength = 100.0f,
        float InDashLength = 20.0f,
        float InGapLength = 10.0f,
        float InThickness = 2.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f),
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Arrow")
    static FCk_Handle_Pmg_DebugShape
    Create_Arrow(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength = 200.0f,
        float InShaftWidth = 20.0f,
        float InArrowHeadRatio = 0.3f,
        float InArrowHeadWidthMultiplier = 2.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Pivot")
    static FCk_Handle_Pmg_DebugShape
    Create_Pivot(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InAxisLength = 100.0f,
        float InArrowSize = 10.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Dashed Line")
    static FCk_Handle_Pmg_DebugShape
    Create_DashedLine(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength = 100.0f,
        float InDashLength = 20.0f,
        float InGapLength = 10.0f,
        float InThickness = 2.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f),
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Arrow",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledArrow(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InDirection,
        float InLength = 200.0f,
        float InShaftWidth = 20.0f,
        float InArrowHeadRatio = 0.3f,
        float InArrowHeadWidthMultiplier = 2.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Pivot",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawPivot(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation = FRotator(0, 0, 0),
        float InAxisLength = 100.0f,
        float InArrowSize = 10.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Dashed Line",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawDashedLine(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        float InDashLength = 20.0f,
        float InGapLength = 10.0f,
        float InThickness = 2.0f,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f),
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);
};

// --------------------------------------------------------------------------------------------------------------------

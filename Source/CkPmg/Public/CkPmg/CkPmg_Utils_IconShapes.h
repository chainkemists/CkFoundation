#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPmg_Utils_IconShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPMG_API UCk_Utils_Pmg_IconShapes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_IconShapes);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Warning")
    static FCk_Handle_Pmg_DebugShape
    Add_Warning(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Prohibition")
    static FCk_Handle_Pmg_DebugShape
    Add_Prohibition(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add No Entry")
    static FCk_Handle_Pmg_DebugShape
    Add_NoEntry(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Info Circle")
    static FCk_Handle_Pmg_DebugShape
    Add_InfoCircle(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Warning")
    static FCk_Handle_Pmg_DebugShape
    Create_Warning(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Prohibition")
    static FCk_Handle_Pmg_DebugShape
    Create_Prohibition(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create No Entry")
    static FCk_Handle_Pmg_DebugShape
    Create_NoEntry(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Info Circle")
    static FCk_Handle_Pmg_DebugShape
    Create_InfoCircle(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Warning",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledWarning(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Prohibition",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledProhibition(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled No Entry",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledNoEntry(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Filled Info Circle",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawFilledInfoCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius = 100.0f,
        int32 InSegments = 32,
        FLinearColor InColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XY,
        float InDuration = 0.0f);
};

// --------------------------------------------------------------------------------------------------------------------

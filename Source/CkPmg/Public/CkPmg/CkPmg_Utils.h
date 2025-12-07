#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkPmg_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Meta = (ScriptMixin = "FCk_Handle_Pmg_Donut"))
class CKPMG_API UCk_Utils_Pmg_Donut_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_Donut_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Pmg_Donut);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Add")
    static FCk_Handle_Pmg_Donut
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Pmg_Donut_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Pmg|Donut",
        DisplayName="[Ck][Pmg][Donut] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Pmg_Donut
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Pmg|Donut",
        DisplayName="[Ck][Pmg][Donut] Handle -> Pmg Donut Handle",
        meta = (CompactNodeTitle = "<AsPmgDonut>", BlueprintAutocast))
    static FCk_Handle_Pmg_Donut
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Pmg Donut Handle",
        Category = "Ck|Utils|Pmg|Donut",
        meta = (CompactNodeTitle = "INVALID_PmgDonutHandle", Keywords = "make"))
    static FCk_Handle_Pmg_Donut
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Update Params")
    static FCk_Handle_Pmg_Donut
    Request_UpdateParams(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        const FCk_Request_Pmg_Donut_UpdateParams& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Inner Radius")
    static FCk_Handle_Pmg_Donut
    Request_SetInnerRadius(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        float InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Outer Radius")
    static FCk_Handle_Pmg_Donut
    Request_SetOuterRadius(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        float InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Segments")
    static FCk_Handle_Pmg_Donut
    Request_SetSegments(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        int32 InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Fill Angle")
    static FCk_Handle_Pmg_Donut
    Request_SetFillAngle(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        float InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Material")
    static FCk_Handle_Pmg_Donut
    Request_SetMaterial(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        UMaterialInterface* InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Enable Collision")
    static FCk_Handle_Pmg_Donut
    Request_SetEnableCollision(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        bool InValue);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Request Set Render Mode")
    static FCk_Handle_Pmg_Donut
    Request_SetRenderMode(
        UPARAM(ref) FCk_Handle_Pmg_Donut& InDonut,
        ECk_Pmg_RenderMode InValue);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Get Inner Radius")
    static float
    Get_InnerRadius(
        const FCk_Handle_Pmg_Donut& InDonut);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Get Outer Radius")
    static float
    Get_OuterRadius(
        const FCk_Handle_Pmg_Donut& InDonut);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Get Segments")
    static int32
    Get_Segments(
        const FCk_Handle_Pmg_Donut& InDonut);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Get Fill Angle")
    static float
    Get_FillAngle(
        const FCk_Handle_Pmg_Donut& InDonut);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|Donut",
              DisplayName="[Ck][Pmg][Donut] Get Render Mode")
    static ECk_Pmg_RenderMode
    Get_RenderMode(
        const FCk_Handle_Pmg_Donut& InDonut);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Meta = (ScriptMixin = "FCk_Handle_Pmg_DebugShape"))
class CKPMG_API UCk_Utils_Pmg_DebugShape_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_DebugShape_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Pmg_DebugShape);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Pmg|DebugShape",
        DisplayName="[Ck][Pmg][DebugShape] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Pmg_DebugShape
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Pmg|DebugShape",
        DisplayName="[Ck][Pmg][DebugShape] Handle -> Pmg DebugShape Handle",
        meta = (CompactNodeTitle = "<AsPmgDebugShape>", BlueprintAutocast))
    static FCk_Handle_Pmg_DebugShape
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Pmg DebugShape Handle",
        Category = "Ck|Utils|Pmg|DebugShape",
        meta = (CompactNodeTitle = "INVALID_PmgDebugShapeHandle", Keywords = "make"))
    static FCk_Handle_Pmg_DebugShape
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------

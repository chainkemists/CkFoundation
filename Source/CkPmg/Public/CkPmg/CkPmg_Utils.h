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
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add")
    static FCk_Handle_Pmg_DebugShape
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform);

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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create")
    static FCk_Handle_Pmg_DebugShape
    Create(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform);

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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
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
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create (Transient Owner)",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle_Pmg_DebugShape
    Create_TransientOwner(
        const UObject* InWorldContextObject,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform);

public:
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
        FRotator InRotation = FRotator(0, 0, 0),
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
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
        FRotator InRotation = FRotator(0, 0, 0),
        int32 InSegments = 16,
        int32 InRings = 8,
        FLinearColor InColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f),
        bool InDrawLines = true,
        float InLineThickness = 2.0f,
        float InDuration = 0.0f);

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

#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Engine/FontFace.h>

#include "CkPmg_Utils_TextShapes.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPMG_API UCk_Utils_Pmg_TextShapes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Pmg_TextShapes);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Add Text")
    static FCk_Handle_Pmg_DebugShape
    Add_Text(
        UPARAM(ref) FCk_Handle& InHandle,
        FTransform InTransform,
        FString InText,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor::White,
        bool InDrawLines = true,
        bool InDrawFilled = true,
        float InLineThickness = 2.0f,
        ECk_Pmg_TextAlign InAlign = ECk_Pmg_TextAlign::Left,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XZ,
        UFontFace* InFontOverride = nullptr,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Create Text")
    static FCk_Handle_Pmg_DebugShape
    Create_Text(
        UPARAM(ref) FCk_Handle& InOwningEntity,
        FTransform InTransform,
        FString InText,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor::White,
        bool InDrawLines = true,
        bool InDrawFilled = true,
        float InLineThickness = 2.0f,
        ECk_Pmg_TextAlign InAlign = ECk_Pmg_TextAlign::Left,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XZ,
        UFontFace* InFontOverride = nullptr,
        float InDuration = 0.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Debug|Filled",
              DisplayName="[Ck][Debug] Draw Text",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static FCk_Handle_Pmg_DebugShape
    DrawText(
        const UObject* InWorldContextObject,
        FVector InCenter,
        FString InText,
        float InSize = 100.0f,
        FLinearColor InColor = FLinearColor::White,
        bool InDrawLines = true,
        bool InDrawFilled = true,
        float InLineThickness = 2.0f,
        ECk_Pmg_TextAlign InAlign = ECk_Pmg_TextAlign::Left,
        ECk_Plane_Axis InDefaultAxis = ECk_Plane_Axis::XZ,
        UFontFace* InFontOverride = nullptr,
        float InDuration = 0.0f);

    // Builds a string from space-separated hex Unicode code points (e.g. "48 49 2190 1F600" -> "HI<-:)").
    // Lets BP/AS author symbol/emoji strings without embedding non-ASCII literals. Handles supplementary
    // planes (codepoints > 0xFFFF) via UTF-16 surrogate pairs.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Pmg|DebugShape",
              DisplayName="[Ck][Pmg][DebugShape] Make Text From Hex Codepoints")
    static FString
    MakeText_FromHexCodepoints(const FString& InHexCodepoints);
};

// --------------------------------------------------------------------------------------------------------------------

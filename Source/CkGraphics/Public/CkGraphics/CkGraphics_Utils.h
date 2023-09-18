#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGraphics_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKGRAPHICS_API UCk_Utils_Graphics_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Graphics_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Graphics")
    static FColor
    Get_ModifiedColorIntensity(
        FColor InColor,
        float InIntensity = 1.0f);
};

// --------------------------------------------------------------------------------------------------------------------

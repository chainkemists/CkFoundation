#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkInput_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKINPUT_API UCk_Utils_Input_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Input_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input")
    static bool
    WasInputChordJustPressed(
        APlayerController* InPlayerController,
        const FInputChord& InInputChord);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input")
    static bool
    WasInputKeyJustPressed_WithCustomModifier(
        APlayerController* InPlayerController,
        const FKey& InInputKey,
        const FKey& InCustomModiferKey);
};

// --------------------------------------------------------------------------------------------------------------------

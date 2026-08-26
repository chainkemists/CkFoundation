#pragma once

#include <UObject/Interface.h>

#include "CkLoadingScreen_MoviePlayerSafe_Interface.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Declares the widget class does nothing on Tick/Paint that requires the game thread - the MoviePlayer ticks Mode A
// widgets on the Slate LOADING thread, so Blueprint/AngelScript Tick overrides are NOT safe. Empty by contract:
// nothing to implement, only something to declare; Mode A refuses any widget class that does not carry it.
UINTERFACE(MinimalAPI, Blueprintable)
class UCk_LoadingScreen_MoviePlayerSafe : public UInterface { GENERATED_BODY() };
class CKLOADINGSCREEN_API ICk_LoadingScreen_MoviePlayerSafe
{
    GENERATED_BODY()

public:
    // Fail closed: null or unmarked is not safe.
    static auto
    Get_IsMoviePlayerSafe(
        const UClass* InWidgetClass) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

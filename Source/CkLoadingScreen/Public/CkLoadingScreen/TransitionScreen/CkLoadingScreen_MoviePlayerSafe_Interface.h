#pragma once

#include <UObject/Interface.h>

#include "CkLoadingScreen_MoviePlayerSafe_Interface.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Implementing this interface asserts the widget class does nothing on Tick/Paint that requires the game thread -
 * the MoviePlayer ticks Mode A widgets on the Slate loading thread. Blueprint/AngelScript Tick overrides are NOT safe.
 *
 * Empty by contract: there is nothing to implement, only something to declare. Mode A refuses any widget class that
 * does not carry it.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCk_LoadingScreen_MoviePlayerSafe : public UInterface { GENERATED_BODY() };
class CKLOADINGSCREEN_API ICk_LoadingScreen_MoviePlayerSafe
{
    GENERATED_BODY()

public:
    // A null or unmarked class is not safe - the answer is the admission decision, so it is never optimistic.
    static auto
    Get_IsMoviePlayerSafe(
        const UClass* InWidgetClass) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

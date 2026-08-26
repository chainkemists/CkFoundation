#pragma once

#include "CkLoadingScreen/TransitionScreen/CkLoadingScreen_MoviePlayerSafe_Interface.h"

#include <UObject/Object.h>

#include "Test_LoadingScreen_Fixtures.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Positive control for the Mode A marker gate. Without it, a Get_IsMoviePlayerSafe that answers "no" for every class
 * would pass the refusal test - the negative case alone cannot tell a working gate from a stuck one.
 */
UCLASS()
class UCkTest_LoadingScreen_MoviePlayerSafeDeclaration
    : public UObject
    , public ICk_LoadingScreen_MoviePlayerSafe
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

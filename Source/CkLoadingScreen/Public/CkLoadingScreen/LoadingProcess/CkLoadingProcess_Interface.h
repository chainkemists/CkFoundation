#pragma once

#include <UObject/Interface.h>

#include "CkLoadingProcess_Interface.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Interface for anything that might cause loading to happen which requires the loading screen to stay up.
 *
 * The LoadingScreen subsystem automatically polls the GameState, its components, every local
 * PlayerController and their components, plus anything registered via
 * UCk_LoadingScreen_Subsystem_UE::RegisterLoadingProcessor. The screen stays up while ANY
 * implementer returns true.
 *
 * C++-only implementation surface (virtual, polled every frame). Blueprint/AngelScript holders
 * should use UCk_LoadingProcess_Task_UE (push-model) instead of implementing this interface.
 */
UINTERFACE(MinimalAPI)
class UCk_LoadingProcess : public UInterface { GENERATED_BODY() };
class CKLOADINGSCREEN_API ICk_LoadingProcess
{
    GENERATED_BODY()

public:
    // Checks whether InTestObject implements the interface, and if so asks whether the
    // loading screen should currently be shown. OutReason is only written when returning true.
    static auto
    Get_ShouldShowLoadingScreen(
        UObject* InTestObject,
        FString& OutReason) -> bool;

    virtual auto
    Get_ShouldShowLoadingScreen(
        FString& OutReason) const -> bool
    {
        return false;
    }
};

// --------------------------------------------------------------------------------------------------------------------

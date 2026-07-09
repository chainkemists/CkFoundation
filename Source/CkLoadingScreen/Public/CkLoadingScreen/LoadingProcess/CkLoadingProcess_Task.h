#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkLoadingScreen/LoadingProcess/CkLoadingProcess_Interface.h"

#include <UObject/Object.h>

#include "CkLoadingProcess_Task.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Push-model loading screen holder for Blueprint/AngelScript (and C++ convenience).
 *
 * Create one to hold the loading screen up with a reason; call Request_Unregister to release it.
 * This is the intended holder surface for script code — implementing ICk_LoadingProcess directly
 * is the C++ poll-model alternative.
 */
UCLASS(NotBlueprintable, BlueprintType)
class CKLOADINGSCREEN_API UCk_LoadingProcess_Task_UE
    : public UObject
    , public ICk_LoadingProcess
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_LoadingProcess_Task_UE);

public:
    /**
     * Creates a task that holds the loading screen up until Request_Unregister is called.
     * Returns nullptr where no loading screen subsystem exists (e.g. dedicated servers).
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Create Loading Process Task",
              meta = (WorldContext = "InWorldContextObject"))
    static UCk_LoadingProcess_Task_UE*
    Create(
        UObject* InWorldContextObject,
        const FString& InShowLoadingScreenReason);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Unregister Loading Process Task")
    void
    Request_Unregister();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Set Loading Process Task Reason")
    void
    Request_SetReason(
        const FString& InReason);

public:
    auto
    Get_ShouldShowLoadingScreen(
        FString& OutReason) const -> bool override;

private:
    FString _Reason;

public:
    CK_PROPERTY_GET(_Reason);
};

// --------------------------------------------------------------------------------------------------------------------

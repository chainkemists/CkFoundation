#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"

// --------------------------------------------------------------------------------------------------------------------

class SWidget;
class UUserWidget;

// --------------------------------------------------------------------------------------------------------------------

class FCkLoadingScreenModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    /** MoviePlayer asks for a screen right before it plays one — this is where the mode is decided. */
    auto DoHandlePrepareLoadingScreen() -> void;

    auto DoHandleMoviePlaybackFinished() -> void;

    auto DoBuildTransitionWidget() const -> TSharedPtr<SWidget>;
    auto DoBuildModeAWidget() -> TSharedPtr<SWidget>;

private:
    /**
     * Mode A only. The UUserWidget handed to the loading thread has no other owner for the duration
     * of the movie, so this IS its GC root; released when playback finishes.
     */
    TStrongObjectPtr<UUserWidget> _ModeAWidget;
};

// --------------------------------------------------------------------------------------------------------------------

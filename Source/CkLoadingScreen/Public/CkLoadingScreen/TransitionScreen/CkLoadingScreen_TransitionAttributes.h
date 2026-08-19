#pragma once

#include <Containers/Array.h>
#include <Containers/UnrealString.h>
#include <MoviePlayer.h>
#include <Templates/SharedPointer.h>

// --------------------------------------------------------------------------------------------------------------------

class SWidget;

// --------------------------------------------------------------------------------------------------------------------

/**
 * How the MoviePlayer transition screen hands back to the game-thread loading screen.
 *
 * Deliberately NOT a UENUM: this is an internal build input, and keeping the header out of UHT
 * keeps MoviePlayer.h out of the reflection path.
 */
enum class ECk_LoadingScreen_TransitionMode : uint8
{
    /**
     * The movie holds until the subsystem confirms its own screen is mounted, then stops it.
     * Kills the one-frame-of-raw-world seam at the handoff.
     */
    Handshake,

    /** Retired-plugin parity: the movie tears itself down the moment loading completes. */
    ParityAutoComplete,

    /**
     * EXPERIMENTAL, ships dark. The configured UMG widget is handed to the loading thread instead
     * of the Slate one. Takes parity flags on purpose: when the loading-thread widget IS the
     * game-thread widget there is no seam left for a handshake to cover, and manual-stop on an
     * unhardened path is the one failure mode that can wedge a boot.
     */
    ModeA_UmgHandOff
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Pure translation from "which mode" to FLoadingScreenAttributes.
 *
 * Split out from the module so the flag matrix is unit-testable: the MoviePlayer itself is inert
 * headless, so these flags are the only part of the transition layer automation can assert on.
 */
class CKLOADINGSCREEN_API FCk_LoadingScreen_TransitionAttributesBuilder
{
public:
    static auto
    Build(
        ECk_LoadingScreen_TransitionMode InMode,
        TSharedPtr<SWidget> InWidget) -> FLoadingScreenAttributes;

    static auto
    Build_StartupMovies(
        const TArray<FString>& InMoviePaths) -> FLoadingScreenAttributes;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::loading_screen::transition
{
    /**
     * Handshake arming is module-scoped state rather than a subsystem member because the two ends
     * live on opposite sides of the blocking wait: the MODULE decides the mode when MoviePlayer
     * asks it to prepare a screen, and the SUBSYSTEM is what ticks (via the engine tick inside
     * WaitForMovieToFinish) and calls StopMovie. Only an armed handshake may stop a movie - that
     * is what keeps the subsystem from cutting the boot logos short.
     */
    CKLOADINGSCREEN_API auto Request_ArmHandshake() -> void;
    CKLOADINGSCREEN_API auto Request_DisarmHandshake() -> void;
    CKLOADINGSCREEN_API auto Get_IsHandshakeArmed() -> bool;
}

// --------------------------------------------------------------------------------------------------------------------

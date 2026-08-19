#include "CkLoadingScreen_TransitionAttributes.h"

#include <Widgets/SWidget.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_loading_screen_transition
{
    static bool HandshakeArmed = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_LoadingScreen_TransitionAttributesBuilder::
    Build(
        ECk_LoadingScreen_TransitionMode InMode,
        TSharedPtr<SWidget> InWidget)
    -> FLoadingScreenAttributes
{
    auto Attributes = FLoadingScreenAttributes{};

    Attributes.WidgetLoadingScreen = InWidget;
    Attributes.MinimumLoadingScreenDisplayTime = -1.0f;
    Attributes.PlaybackType = EMoviePlaybackType::MT_Normal;

    // A click must never be able to dismiss a transition screen: it covers a blocking LoadMap, so
    // dismissing it early is exactly the frozen-world frame this whole layer exists to prevent.
    Attributes.bMoviesAreSkippable = false;

    // No movie paths - the transition screen is the widget. Boot logos go through
    // Build_StartupMovies instead.
    Attributes.bAllowInEarlyStartup = false;

    switch (InMode)
    {
        case ECk_LoadingScreen_TransitionMode::Handshake:
        {
            Attributes.bWaitForManualStop = true;
            Attributes.bAutoCompleteWhenLoadingCompletes = false;

            // MANDATORY, not a tunable. The manual-stop wait loop blocks the game thread; without
            // the engine tick the subsystem never runs, so nothing can ever reach StopMovie and
            // the game deadlocks at load end. Guarded by a unit test for exactly this reason.
            Attributes.bAllowEngineTick = true;
            break;
        }
        case ECk_LoadingScreen_TransitionMode::ParityAutoComplete:
        case ECk_LoadingScreen_TransitionMode::ModeA_UmgHandOff:
        {
            Attributes.bWaitForManualStop = false;
            Attributes.bAutoCompleteWhenLoadingCompletes = true;
            Attributes.bAllowEngineTick = false;
            break;
        }
    }

    return Attributes;
}

auto
    FCk_LoadingScreen_TransitionAttributesBuilder::
    Build_StartupMovies(
        const TArray<FString>& InMoviePaths)
    -> FLoadingScreenAttributes
{
    auto Attributes = FLoadingScreenAttributes{};

    Attributes.MoviePaths = InMoviePaths;
    Attributes.MinimumLoadingScreenDisplayTime = -1.0f;
    Attributes.bAutoCompleteWhenLoadingCompletes = true;
    Attributes.bMoviesAreSkippable = true;
    Attributes.bWaitForManualStop = false;
    Attributes.bAllowInEarlyStartup = false;
    Attributes.bAllowEngineTick = false;
    Attributes.PlaybackType = EMoviePlaybackType::MT_Normal;

    return Attributes;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::loading_screen::transition::
    Request_ArmHandshake()
    -> void
{
    ck_loading_screen_transition::HandshakeArmed = true;
}

auto
    ck::loading_screen::transition::
    Request_DisarmHandshake()
    -> void
{
    ck_loading_screen_transition::HandshakeArmed = false;
}

auto
    ck::loading_screen::transition::
    Get_IsHandshakeArmed()
    -> bool
{
    return ck_loading_screen_transition::HandshakeArmed;
}

// --------------------------------------------------------------------------------------------------------------------

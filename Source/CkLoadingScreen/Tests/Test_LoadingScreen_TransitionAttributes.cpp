// Flag-matrix contract for the MoviePlayer transition screen.
//
// The MoviePlayer is inert in automation (no RHI, no movie streamer), so the visual layer cannot
// be tested headless at all. What CAN be tested is the attribute block the module hands it - and
// one combination in that block is a hard deadlock, so it gets an invariant of its own.

#include "CkLoadingScreen/Tests/Test_LoadingScreen_Fixtures.h"
#include "CkLoadingScreen/TransitionScreen/CkLoadingScreen_MoviePlayerSafe_Interface.h"
#include "CkLoadingScreen/TransitionScreen/CkLoadingScreen_TransitionAttributes.h"

#include "CkCore/Macros/CkMacros.h"

#include "Blueprint/UserWidget.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_LoadingScreen_TransitionAttributes_ModeMatrix,
    "Ck.LoadingScreen.TransitionAttributes.ModeMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_LoadingScreen_TransitionAttributes_NoManualStopWithoutEngineTick,
    "Ck.LoadingScreen.TransitionAttributes.NoManualStopWithoutEngineTick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_LoadingScreen_TransitionAttributes_StartupMovies,
    "Ck.LoadingScreen.TransitionAttributes.StartupMovies",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_LoadingScreen_TransitionAttributes_HandshakeArming,
    "Ck.LoadingScreen.TransitionAttributes.HandshakeArming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_LoadingScreen_TransitionAttributes_ModeARefusesUnmarkedWidget,
    "Ck.LoadingScreen.TransitionAttributes.ModeARefusesUnmarkedWidget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_LoadingScreen_TransitionAttributes_ModeMatrix::RunTest(const FString&)
{
    const auto Handshake = FCk_LoadingScreen_TransitionAttributesBuilder::Build(
        ECk_LoadingScreen_TransitionMode::Handshake, nullptr);

    TestTrue(TEXT("handshake waits for a manual stop"), Handshake.bWaitForManualStop);
    TestFalse(TEXT("handshake does not auto-complete - the subsystem decides when to stop"),
        Handshake.bAutoCompleteWhenLoadingCompletes);
    TestTrue(TEXT("handshake allows the engine tick"), Handshake.bAllowEngineTick);

    const auto Parity = FCk_LoadingScreen_TransitionAttributesBuilder::Build(
        ECk_LoadingScreen_TransitionMode::ParityAutoComplete, nullptr);

    TestFalse(TEXT("parity does not wait for a manual stop"), Parity.bWaitForManualStop);
    TestTrue(TEXT("parity auto-completes when loading completes"), Parity.bAutoCompleteWhenLoadingCompletes);

    const auto ModeA = FCk_LoadingScreen_TransitionAttributesBuilder::Build(
        ECk_LoadingScreen_TransitionMode::ModeA_UmgHandOff, nullptr);

    TestFalse(TEXT("Mode A does not wait for a manual stop"), ModeA.bWaitForManualStop);
    TestTrue(TEXT("Mode A auto-completes when loading completes"), ModeA.bAutoCompleteWhenLoadingCompletes);

    // A transition screen covers a blocking LoadMap; letting a click dismiss it re-opens exactly
    // the frozen-world window the layer exists to close.
    TestFalse(TEXT("the transition screen is never click-skippable"), Handshake.bMoviesAreSkippable);
    TestFalse(TEXT("the transition screen is never click-skippable"), Parity.bMoviesAreSkippable);
    TestFalse(TEXT("the transition screen is never click-skippable"), ModeA.bMoviesAreSkippable);

    return true;
}

bool FCkTest_LoadingScreen_TransitionAttributes_NoManualStopWithoutEngineTick::RunTest(const FString&)
{
    // THE deadlock invariant. bWaitForManualStop spins the game thread inside
    // WaitForMovieToFinish; with bAllowEngineTick false, GEngine->Tick never runs in that loop,
    // so the subsystem never ticks, so nothing can ever call StopMovie. The game hangs forever at
    // the end of every load. No mode may ever emit that pair.
    const auto Modes = TArray<ECk_LoadingScreen_TransitionMode>
    {
        ECk_LoadingScreen_TransitionMode::Handshake,
        ECk_LoadingScreen_TransitionMode::ParityAutoComplete,
        ECk_LoadingScreen_TransitionMode::ModeA_UmgHandOff
    };

    for (const auto Mode : Modes)
    {
        const auto Attributes = FCk_LoadingScreen_TransitionAttributesBuilder::Build(Mode, nullptr);

        TestFalse(
            FString::Printf(TEXT("mode [%d] must never wait for a manual stop without the engine tick"),
                static_cast<int32>(Mode)),
            Attributes.bWaitForManualStop && NOT Attributes.bAllowEngineTick);
    }

    const auto StartupMovies = FCk_LoadingScreen_TransitionAttributesBuilder::Build_StartupMovies(
        TArray<FString>{TEXT("Game"), TEXT("Studio")});

    TestFalse(TEXT("startup movies must never wait for a manual stop without the engine tick"),
        StartupMovies.bWaitForManualStop && NOT StartupMovies.bAllowEngineTick);

    return true;
}

bool FCkTest_LoadingScreen_TransitionAttributes_StartupMovies::RunTest(const FString&)
{
    const auto Paths = TArray<FString>{TEXT("Game"), TEXT("Studio")};
    const auto Attributes = FCk_LoadingScreen_TransitionAttributesBuilder::Build_StartupMovies(Paths);

    TestEqual(TEXT("movie path count passes through"), Attributes.MoviePaths.Num(), Paths.Num());
    for (auto Index = 0; Index < Paths.Num() && Index < Attributes.MoviePaths.Num(); ++Index)
    {
        TestEqual(TEXT("movie paths pass through in authored order"),
            Attributes.MoviePaths[Index], Paths[Index]);
    }
    TestTrue(TEXT("boot logos auto-complete"), Attributes.bAutoCompleteWhenLoadingCompletes);
    TestFalse(TEXT("boot logos never wait for a manual stop"), Attributes.bWaitForManualStop);
    TestFalse(TEXT("boot logos carry no widget"), Attributes.WidgetLoadingScreen.IsValid());

    const auto Empty = FCk_LoadingScreen_TransitionAttributesBuilder::Build_StartupMovies(TArray<FString>{});
    TestFalse(TEXT("an empty movie list produces nothing playable"), Empty.IsValid());

    return true;
}

bool FCkTest_LoadingScreen_TransitionAttributes_HandshakeArming::RunTest(const FString&)
{
    // Only an ARMED handshake may stop a movie. That is what stops the subsystem from cutting the
    // boot logos short: the module arms it exclusively from OnPrepareLoadingScreen, and the boot
    // logos are set up outside that path.
    const auto WasArmed = ck::loading_screen::transition::Get_IsHandshakeArmed();

    ck::loading_screen::transition::Request_DisarmHandshake();
    TestFalse(TEXT("disarmed by default"), ck::loading_screen::transition::Get_IsHandshakeArmed());

    ck::loading_screen::transition::Request_ArmHandshake();
    TestTrue(TEXT("arming takes"), ck::loading_screen::transition::Get_IsHandshakeArmed());

    ck::loading_screen::transition::Request_DisarmHandshake();
    TestFalse(TEXT("disarming takes"), ck::loading_screen::transition::Get_IsHandshakeArmed());

    if (WasArmed)
    { ck::loading_screen::transition::Request_ArmHandshake(); }

    return true;
}

bool FCkTest_LoadingScreen_TransitionAttributes_ModeARefusesUnmarkedWidget::RunTest(const FString&)
{
    // DoBuildModeAWidget itself needs a GameInstance and a live MoviePlayer, so what is asserted headless is the
    // admission decision it makes plus the attribute block the refusal lands on.
    TestFalse(TEXT("a null widget class is never MoviePlayer-safe"),
        ICk_LoadingScreen_MoviePlayerSafe::Get_IsMoviePlayerSafe(nullptr));
    TestFalse(TEXT("a plain UMG widget class does not declare the marker"),
        ICk_LoadingScreen_MoviePlayerSafe::Get_IsMoviePlayerSafe(UUserWidget::StaticClass()));
    TestTrue(TEXT("a class that declares the marker is accepted"),
        ICk_LoadingScreen_MoviePlayerSafe::Get_IsMoviePlayerSafe(
            UCkTest_LoadingScreen_MoviePlayerSafeDeclaration::StaticClass()));

    // A refused Mode A returns nullptr, and the module then rebuilds the Slate screen under whichever mode
    // _TransitionWaitForGameThreadScreen names. Both fallbacks must still be presentable and non-skippable.
    const auto Handshake = FCk_LoadingScreen_TransitionAttributesBuilder::Build(
        ECk_LoadingScreen_TransitionMode::Handshake, nullptr);
    const auto Parity = FCk_LoadingScreen_TransitionAttributesBuilder::Build(
        ECk_LoadingScreen_TransitionMode::ParityAutoComplete, nullptr);

    TestTrue(TEXT("the handshake fallback still ticks the engine, so the manual stop can be reached"),
        Handshake.bAllowEngineTick);
    TestFalse(TEXT("the handshake fallback is not click-skippable"), Handshake.bMoviesAreSkippable);
    TestTrue(TEXT("the parity fallback auto-completes"), Parity.bAutoCompleteWhenLoadingCompletes);
    TestFalse(TEXT("the parity fallback is not click-skippable"), Parity.bMoviesAreSkippable);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif

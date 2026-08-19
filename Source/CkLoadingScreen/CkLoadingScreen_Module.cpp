#include "CkLoadingScreen_Module.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkLoadingScreen/CkLoadingScreen_Log.h"
#include "CkLoadingScreen/Settings/CkLoadingScreen_Settings.h"
#include "CkLoadingScreen/Subsystem/CkLoadingScreen_Subsystem.h"
#include "CkLoadingScreen/TransitionScreen/CkLoadingScreen_TransitionAttributes.h"
#include "CkLoadingScreen/TransitionScreen/CkLoadingScreen_TransitionScreen.h"

#include <Blueprint/UserWidget.h>
#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/Texture2D.h>
#include <Framework/Application/SlateApplication.h>
#include <MoviePlayer.h>
#include <Slate/DeferredCleanupSlateBrush.h>

#define LOCTEXT_NAMESPACE "FCkLoadingScreenModule"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_loading_screen_module
{
    // ResolveObject, never TryLoad. This runs on the game thread at travel start, so a sync load
    // here is precisely the stall the transition screen exists to hide. The subsystem's
    // FStreamableHandle is what makes these textures resident (and GC-rooted) ahead of time; a miss
    // degrades to "no logo" / "no backdrop", never to a hitch.
    auto DoTryMakeBrush(
        const FSoftObjectPath& InPath,
        FVector2D InSize) -> TSharedPtr<FDeferredCleanupSlateBrush>
    {
        if (InPath.IsNull())
        { return nullptr; }

        const auto Texture = Cast<UTexture2D>(InPath.ResolveObject());
        if (ck::Is_NOT_Valid(Texture))
        { return nullptr; }

        return InSize.IsNearlyZero()
            ? FDeferredCleanupSlateBrush::CreateBrush(Texture)
            : FDeferredCleanupSlateBrush::CreateBrush(Texture, InSize);
    }

    auto DoTryGet_GameInstance() -> UGameInstance*
    {
        if (ck::Is_NOT_Valid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        for (const auto& WorldContext : GEngine->GetWorldContexts())
        {
            if (WorldContext.WorldType != EWorldType::Game && WorldContext.WorldType != EWorldType::PIE)
            { continue; }

            if (ck::IsValid(WorldContext.OwningGameInstance))
            { return WorldContext.OwningGameInstance; }
        }

        return nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------

void FCkLoadingScreenModule::StartupModule()
{
    // Timing mirrors the retired AsyncLoadingScreen module exactly: a dedicated server has no movie
    // player at all, and none of this is legal before Slate is up.
    if (IsRunningDedicatedServer() || NOT FSlateApplication::IsInitialized())
    { return; }

    if (NOT IsMoviePlayerEnabled())
    { return; }

    GetMoviePlayer()->OnPrepareLoadingScreen().AddRaw(this, &FCkLoadingScreenModule::DoHandlePrepareLoadingScreen);
    GetMoviePlayer()->OnMoviePlaybackFinished().AddRaw(this, &FCkLoadingScreenModule::DoHandleMoviePlaybackFinished);

    const auto& StartupMoviePaths = UCk_Utils_LoadingScreen_Settings_UE::Get_StartupMoviePaths();
    if (StartupMoviePaths.IsEmpty())
    { return; }

    // Guarded, and expected to be a no-op today: this module loads at ELoadingPhase::Default, which
    // LaunchEngineLoop reaches AFTER it has already called PlayMovie() for the startup screen
    // (PreLoadingScreen modules 3748, PlayMovie 3810, Default modules 4617). Setting up over a live
    // playback would fight it. Projects get boot logos from the engine's own
    // [/Script/MoviePlayer.MoviePlayerSettings] StartupMovies instead; this path only becomes live
    // if the module is ever moved to an earlier loading phase.
    if (GetMoviePlayer()->IsMovieCurrentlyPlaying() || GetMoviePlayer()->LoadingScreenIsPrepared())
    {
        ck::loading_screen::Verbose(
            TEXT("Skipping _StartupMoviePaths setup - the movie player already has a screen prepared or playing. "
                 "Boot logos belong to [/Script/MoviePlayer.MoviePlayerSettings] StartupMovies at this loading phase."));
        return;
    }

    GetMoviePlayer()->SetupLoadingScreen(
        FCk_LoadingScreen_TransitionAttributesBuilder::Build_StartupMovies(StartupMoviePaths));
}

void FCkLoadingScreenModule::ShutdownModule()
{
    if (IsRunningDedicatedServer() || NOT IsMoviePlayerEnabled())
    { return; }

    // The retired plugin left this as a "TODO: Unregister later" and unbound unconditionally.
    if (const auto MoviePlayer = GetMoviePlayer();
        ck::IsValid(MoviePlayer, ck::IsValid_Policy_NullptrOnly{}))
    {
        MoviePlayer->OnPrepareLoadingScreen().RemoveAll(this);
        MoviePlayer->OnMoviePlaybackFinished().RemoveAll(this);
    }

    ck::loading_screen::transition::Request_DisarmHandshake();
    _ModeAWidget.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkLoadingScreenModule::
    DoHandlePrepareLoadingScreen()
    -> void
{
    if (NOT UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionScreenEnabled())
    { return; }

    // The same suppression list the game-thread screen honours - commandlet / -unattended / null-RHI
    // / ck.LoadingScreen.Disable. Shared rather than duplicated so the two can never disagree about
    // whether this run is allowed to present anything.
    if (UCk_LoadingScreen_Subsystem_UE::Get_IsPresentationSuppressed())
    { return; }

    const auto RequestedMode = [&]() -> ECk_LoadingScreen_TransitionMode
    {
        if (UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionModeA_UMGHandOff() &&
            NOT UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().IsNull())
        { return ECk_LoadingScreen_TransitionMode::ModeA_UmgHandOff; }

        return UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionWaitForGameThreadScreen()
            ? ECk_LoadingScreen_TransitionMode::Handshake
            : ECk_LoadingScreen_TransitionMode::ParityAutoComplete;
    }();

    auto ResolvedMode = RequestedMode;
    auto Widget = RequestedMode == ECk_LoadingScreen_TransitionMode::ModeA_UmgHandOff
        ? DoBuildModeAWidget()
        : DoBuildTransitionWidget();

    // Mode A is experimental and its widget can legitimately fail to build. Falling back to the
    // Slate screen keeps a failed experiment cosmetic instead of leaving the load uncovered.
    if (NOT Widget.IsValid() && RequestedMode == ECk_LoadingScreen_TransitionMode::ModeA_UmgHandOff)
    {
        ck::loading_screen::Warning(
            TEXT("Mode A UMG hand-off could not build a widget - falling back to the Slate transition screen."));

        ResolvedMode = UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionWaitForGameThreadScreen()
            ? ECk_LoadingScreen_TransitionMode::Handshake
            : ECk_LoadingScreen_TransitionMode::ParityAutoComplete;

        Widget = DoBuildTransitionWidget();
    }

    GetMoviePlayer()->SetupLoadingScreen(
        FCk_LoadingScreen_TransitionAttributesBuilder::Build(ResolvedMode, Widget));

    // Arm LAST, and only here. The subsystem may only stop a movie it was told about — that is what
    // keeps it from cutting short a startup-movie playback it had no part in setting up.
    if (ResolvedMode == ECk_LoadingScreen_TransitionMode::Handshake)
    { ck::loading_screen::transition::Request_ArmHandshake(); }

    ck::loading_screen::Log(TEXT("Transition screen prepared in mode [{}]"), static_cast<int32>(ResolvedMode));
}

auto
    FCkLoadingScreenModule::
    DoHandleMoviePlaybackFinished()
    -> void
{
    ck::loading_screen::transition::Request_DisarmHandshake();
    _ModeAWidget.Reset();
}

auto
    FCkLoadingScreenModule::
    DoBuildTransitionWidget() const
    -> TSharedPtr<SWidget>
{
    const auto LogoBrush = ck_loading_screen_module::DoTryMakeBrush(
        UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoTexture(),
        UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoSize());

    const auto BackdropBrush = [&]() -> TSharedPtr<FDeferredCleanupSlateBrush>
    {
        const auto& Backgrounds = UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionBackgroundImages();
        if (Backgrounds.IsEmpty())
        { return nullptr; }

        // Sizeless on purpose: the widget scales the backdrop to fill, so a baked size fights it.
        const auto Index = FMath::RandRange(0, Backgrounds.Num() - 1);
        return ck_loading_screen_module::DoTryMakeBrush(Backgrounds[Index], FVector2D::ZeroVector);
    }();

    const auto TipText = [&]() -> FText
    {
        const auto& Tips = UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionTipTexts();
        if (Tips.IsEmpty())
        { return FText::GetEmpty(); }

        // Picked once, for the whole load - nothing out there can tick a rotation.
        return Tips[FMath::RandRange(0, Tips.Num() - 1)];
    }();

    return SNew(SCk_LoadingScreen_Transition)
        .LogoBrush(LogoBrush)
        .BackdropBrush(BackdropBrush)
        .BackgroundTint(UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionBackgroundTint())
        .LogoOffset(UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoOffset())
        .ThrobPeriod(UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoThrobPeriod())
        .ThrobMinOpacity(UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoMinOpacity())
        .TipText(TipText);
}

auto
    FCkLoadingScreenModule::
    DoBuildModeAWidget()
    -> TSharedPtr<SWidget>
{
    const auto GameInstance = ck_loading_screen_module::DoTryGet_GameInstance();
    if (ck::Is_NOT_Valid(GameInstance))
    { return nullptr; }

    const auto WidgetClass = TSubclassOf<UUserWidget>{
        UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().TryLoadClass<UUserWidget>()};

    if (ck::Is_NOT_Valid(WidgetClass))
    { return nullptr; }

    const auto UserWidget = UUserWidget::CreateWidgetInstance(*GameInstance, WidgetClass, NAME_None);
    if (ck::Is_NOT_Valid(UserWidget))
    { return nullptr; }

    _ModeAWidget.Reset(UserWidget);

    return UserWidget->TakeWidget();
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkLoadingScreenModule, CkLoadingScreen)

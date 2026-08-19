#include "CkLoadingScreen_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkLoadingScreen/CkLoadingScreen_Log.h"
#include "CkLoadingScreen/LoadingProcess/CkLoadingProcess_Interface.h"
#include "CkLoadingScreen/Settings/CkLoadingScreen_Settings.h"
#include "CkLoadingScreen/TransitionScreen/CkLoadingScreen_TransitionAttributes.h"

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/GameViewportClient.h>
#include <Framework/Application/IInputProcessor.h>
#include <Framework/Application/SlateApplication.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/WorldSettings.h>
#include <HAL/ThreadHeartBeat.h>
#include <Misc/App.h>
#include <Misc/CommandLine.h>
#include <Misc/ConfigCacheIni.h>
#include <Engine/AssetManager.h>
#include <Engine/StreamableManager.h>
#include <MoviePlayer.h>
#include <PreLoadScreen.h>
#include <PreLoadScreenManager.h>
#include <ProfilingDebugging/CsvProfiler.h>
#include <ShaderPipelineCache.h>
#include <Widgets/Images/SThrobber.h>
#include <Blueprint/UserWidget.h>

// --------------------------------------------------------------------------------------------------------------------

CSV_DEFINE_CATEGORY(CkLoadingScreen, true);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_loading_screen_cvars
{
    static float HoldLoadingScreenAdditionalSecs = 2.0f;
    static FAutoConsoleVariableRef CVarHoldLoadingScreenAdditionalSecs(
        TEXT("ck.LoadingScreen.HoldAdditionalSecs"),
        HoldLoadingScreenAdditionalSecs,
        TEXT("How long to hold the loading screen up after other loading finishes (in seconds) to try to give texture streaming a chance to avoid blurriness"),
        ECVF_Default | ECVF_Preview);

    static bool LogLoadingScreenReasonEveryFrame = false;
    static FAutoConsoleVariableRef CVarLogLoadingScreenReasonEveryFrame(
        TEXT("ck.LoadingScreen.LogReasonEveryFrame"),
        LogLoadingScreenReasonEveryFrame,
        TEXT("When true, the reason the loading screen is shown or hidden will be printed to the log every frame."),
        ECVF_Default);

    static bool ForceLoadingScreenVisible = false;
    static FAutoConsoleVariableRef CVarForceLoadingScreenVisible(
        TEXT("ck.LoadingScreen.AlwaysShow"),
        ForceLoadingScreenVisible,
        TEXT("Force the loading screen to show."),
        ECVF_Default);

    static bool DisableLoadingScreen = false;
    static FAutoConsoleVariableRef CVarDisableLoadingScreen(
        TEXT("ck.LoadingScreen.Disable"),
        DisableLoadingScreen,
        TEXT("Escape hatch: fully suppress the loading screen presentation. Holder bookkeeping still runs."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_loading_screen_subsystem
{
    // Fail-open cap on the transition handshake. A cosmetic handoff must never be able to hold the
    // game hostage, so past this many seconds after loading finished we stop the movie regardless.
    constexpr auto TransitionHandshakeFailOpenSecs = 5.0;

    // Eats ALL input so menus underneath the loading screen cannot be interacted with
    class FLoadingScreenInputPreProcessor : public IInputProcessor
    {
    public:
        auto Get_CanEatInput() const -> bool
        {
            return NOT GIsEditor;
        }

        auto Tick(const float InDeltaTime, FSlateApplication& InSlateApp, TSharedRef<ICursor> InCursor) -> void override { }

        auto HandleKeyDownEvent(FSlateApplication& InSlateApp, const FKeyEvent& InKeyEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleKeyUpEvent(FSlateApplication& InSlateApp, const FKeyEvent& InKeyEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleAnalogInputEvent(FSlateApplication& InSlateApp, const FAnalogInputEvent& InAnalogInputEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleMouseMoveEvent(FSlateApplication& InSlateApp, const FPointerEvent& InMouseEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleMouseButtonDownEvent(FSlateApplication& InSlateApp, const FPointerEvent& InMouseEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleMouseButtonUpEvent(FSlateApplication& InSlateApp, const FPointerEvent& InMouseEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleMouseButtonDoubleClickEvent(FSlateApplication& InSlateApp, const FPointerEvent& InMouseEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleMouseWheelOrGestureEvent(FSlateApplication& InSlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) -> bool override { return Get_CanEatInput(); }
        auto HandleMotionDetectedEvent(FSlateApplication& InSlateApp, const FMotionEvent& InMotionEvent) -> bool override { return Get_CanEatInput(); }
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_LoadingScreen_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisType::DoHandlePreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisType::DoHandlePostLoadMap);

    DoRequestTransitionAssets();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Deinitialize()
    -> void
{
    DoStopBlockingInput();

    DoRemoveWidgetFromViewport();

    FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

    _TransitionAssetsHandle.Reset();

    SetTickableTickType(ETickableTickType::Never);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    const auto GameInstance = CastChecked<UGameInstance>(InOuter);
    return NOT GameInstance->IsDedicatedServerInstance();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    DoUpdateLoadingScreen();

    // Deliberately AFTER DoUpdateLoadingScreen: during a handshake this Tick is running from inside
    // the MoviePlayer's blocking wait loop (via GEngine->Tick), so the update above is what mounts
    // our widget and this is what notices it went up - both in the same frame.
    DoTickTransitionHandshake();

    _TimeUntilNextLogHeartbeatSeconds = FMath::Max(_TimeUntilNextLogHeartbeatSeconds - InDeltaTime, 0.0);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    GetTickableTickType() const
    -> ETickableTickType
{
    if (IsTemplate())
    { return ETickableTickType::Never; }

    return ETickableTickType::Conditional;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    IsTickable() const
    -> bool
{
    // Catches cases that ShouldCreateSubsystem does not
    const auto GameInstance = GetGameInstance();
    return ck::IsValid(GameInstance) && ck::IsValid(GameInstance->GetGameViewportClient());
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    GetStatId() const
    -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_LoadingScreen_Subsystem_UE, STATGROUP_Tickables);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    GetTickableGameObjectWorld() const
    -> UWorld*
{
    return GetGameInstance()->GetWorld();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_LoadingScreen_Subsystem_UE::
    Get_DebugReason() const
    -> FString
{
    return _DebugReason;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Get_IsLoadingScreenShowing() const
    -> bool
{
    return _CurrentlyShowingLoadingScreen;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Get_NeedsLoadingScreen()
    -> bool
{
    return DoCheckForAnyNeedToShowLoadingScreen();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    BindTo_OnVisibilityChanged(
        const FCk_Delegate_LoadingScreen_OnVisibilityChanged& InDelegate)
    -> void
{
    _OnVisibilityChanged.AddUnique(InDelegate);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    UnbindFrom_OnVisibilityChanged(
        const FCk_Delegate_LoadingScreen_OnVisibilityChanged& InDelegate)
    -> void
{
    _OnVisibilityChanged.Remove(InDelegate);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Register_LoadingProcessor(
        TScriptInterface<ICk_LoadingProcess> InInterface)
    -> void
{
    _ExternalLoadingProcessors.Add(InInterface.GetObject());
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Unregister_LoadingProcessor(
        TScriptInterface<ICk_LoadingProcess> InInterface)
    -> void
{
    _ExternalLoadingProcessors.Remove(InInterface.GetObject());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoHandlePreLoadMap(
        const FWorldContext& InWorldContext,
        const FString& InMapName)
    -> void
{
    if (InWorldContext.OwningGameInstance != GetGameInstance())
    { return; }

    _CurrentlyInLoadMap = true;

    if (GEngine->IsInitialized())
    { DoUpdateLoadingScreen(); }
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoHandlePostLoadMap(
        UWorld* InWorld)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld) || InWorld->GetGameInstance() != GetGameInstance())
    { return; }

    _CurrentlyInLoadMap = false;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoUpdateLoadingScreen()
    -> void
{
    auto LogLoadingScreenStatus = ck_loading_screen_cvars::LogLoadingScreenReasonEveryFrame;

    if (DoShouldShowLoadingScreen())
    {
        // Missing the checkpoint within the window trips the hang detector, pinpointing the stall
        FThreadHeartBeat::Get().MonitorCheckpointStart(GetFName(),
            UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenHeartbeatHangDuration());

        DoShowLoadingScreen();

        const auto LogHeartbeatInterval = UCk_Utils_LoadingScreen_Settings_UE::Get_LogLoadingScreenHeartbeatInterval();
        if (LogHeartbeatInterval > 0.0f && _TimeUntilNextLogHeartbeatSeconds <= 0.0)
        {
            LogLoadingScreenStatus = true;
            _TimeUntilNextLogHeartbeatSeconds = LogHeartbeatInterval;
        }
    }
    else
    {
        DoHideLoadingScreen();

        FThreadHeartBeat::Get().MonitorCheckpointEnd(GetFName());
    }

    if (LogLoadingScreenStatus)
    {
        ck::loading_screen::Log(TEXT("Loading screen showing: [{}]. Reason: [{}]"),
            _CurrentlyShowingLoadingScreen, _DebugReason);
    }
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoCheckForAnyNeedToShowLoadingScreen()
    -> bool
{
    _DebugReason = TEXT("Reason for Showing/Hiding LoadingScreen is unknown!");

    const auto LocalGameInstance = GetGameInstance();

    if (ck_loading_screen_cvars::ForceLoadingScreenVisible)
    {
        _DebugReason = TEXT("ck.LoadingScreen.AlwaysShow is true");
        return true;
    }

    const auto Context = LocalGameInstance->GetWorldContext();
    if (ck::Is_NOT_Valid(Context, ck::IsValid_Policy_NullptrOnly{}))
    {
        _DebugReason = TEXT("The game instance has a null WorldContext");
        return true;
    }

    const auto World = Context->World();
    if (ck::Is_NOT_Valid(World))
    {
        _DebugReason = TEXT("We have no world (FWorldContext's World() is null)");
        return true;
    }

    const auto GameState = World->GetGameState<AGameStateBase>();
    if (ck::Is_NOT_Valid(GameState))
    {
        _DebugReason = TEXT("GameState hasn't yet replicated (it's null)");
        return true;
    }

    if (_CurrentlyInLoadMap)
    {
        _DebugReason = TEXT("_CurrentlyInLoadMap is true");
        return true;
    }

    if (NOT Context->TravelURL.IsEmpty())
    {
        _DebugReason = TEXT("We have pending travel (the TravelURL is not empty)");
        return true;
    }

    if (ck::IsValid(Context->PendingNetGame))
    {
        _DebugReason = TEXT("We are connecting to another server (PendingNetGame != nullptr)");
        return true;
    }

    if (NOT World->HasBegunPlay())
    {
        _DebugReason = TEXT("World hasn't begun play");
        return true;
    }

    if (World->IsInSeamlessTravel())
    {
        _DebugReason = TEXT("We are in seamless travel");
        return true;
    }

    if (UCk_Utils_LoadingScreen_Settings_UE::Get_WaitForStreamingLevels())
    {
        auto NumPendingStreamingLevels = 0;
        for (const auto StreamingLevel : World->GetStreamingLevels())
        {
            if (ck::Is_NOT_Valid(StreamingLevel))
            { continue; }

            const auto PendingLoad = StreamingLevel->ShouldBeLoaded() && NOT StreamingLevel->IsLevelLoaded();
            const auto PendingVisibility = StreamingLevel->ShouldBeVisible() && NOT StreamingLevel->IsLevelVisible();

            if (PendingLoad || PendingVisibility)
            { ++NumPendingStreamingLevels; }
        }

        if (NumPendingStreamingLevels > 0)
        {
            _DebugReason = ck::Format_UE(TEXT("Waiting on [{}] streaming sublevel(s) to finish loading"),
                NumPendingStreamingLevels);
            return true;
        }
    }

    if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(GameState, /*out*/ _DebugReason))
    { return true; }

    for (const auto TestComponent : GameState->GetComponents())
    {
        if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(TestComponent, /*out*/ _DebugReason))
        { return true; }
    }

    for (const auto& Processor : _ExternalLoadingProcessors)
    {
        if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(Processor.GetObject(), /*out*/ _DebugReason))
        { return true; }
    }

    auto FoundAnyLocalPC = false;
    auto MissingAnyLocalPC = false;

    for (const auto LocalPlayer : LocalGameInstance->GetLocalPlayers())
    {
        if (ck::Is_NOT_Valid(LocalPlayer))
        { continue; }

        const auto PlayerController = LocalPlayer->PlayerController;
        if (ck::Is_NOT_Valid(PlayerController))
        {
            MissingAnyLocalPC = true;
            continue;
        }

        FoundAnyLocalPC = true;

        if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(PlayerController, /*out*/ _DebugReason))
        { return true; }

        for (const auto TestComponent : PlayerController->GetComponents())
        {
            if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(TestComponent, /*out*/ _DebugReason))
            { return true; }
        }
    }

    const auto GameViewportClient = LocalGameInstance->GetGameViewportClient();
    const auto IsInSplitscreen = GameViewportClient->GetCurrentSplitscreenConfiguration() != ESplitScreenType::None;

    if (IsInSplitscreen && MissingAnyLocalPC)
    {
        _DebugReason = TEXT("At least one missing local player controller in splitscreen");
        return true;
    }

    if (NOT IsInSplitscreen && NOT FoundAnyLocalPC)
    {
        _DebugReason = TEXT("Need at least one local player controller");
        return true;
    }

    _DebugReason = TEXT("(nothing wants to show it anymore)");
    return false;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoShouldShowLoadingScreen()
    -> bool
{
    if (Get_IsPresentationSuppressed())
    {
        _DebugReason = TEXT("Loading screen presentation is suppressed (commandlet/unattended/null-RHI or ck.LoadingScreen.Disable)");
        return false;
    }

#if NOT UE_BUILD_SHIPPING
    static const auto CmdLineNoLoadingScreen = FParse::Param(FCommandLine::Get(), TEXT("NoLoadingScreen"));
    if (CmdLineNoLoadingScreen)
    {
        _DebugReason = TEXT("CommandLine has 'NoLoadingScreen'");
        return false;
    }
#endif

    const auto LocalGameInstance = GetGameInstance();
    if (ck::Is_NOT_Valid(LocalGameInstance->GetGameViewportClient()))
    { return false; }

    const auto NeedToShowLoadingScreen = DoCheckForAnyNeedToShowLoadingScreen();

    auto WantToForceShowLoadingScreen = false;

    if (NeedToShowLoadingScreen)
    {
        _TimeLoadingScreenLastDismissed = -1.0;
    }
    else
    {
        const auto CurrentTime = FPlatformTime::Seconds();
        const auto CanHoldLoadingScreen =
            NOT GIsEditor || UCk_Utils_LoadingScreen_Settings_UE::Get_HoldLoadingScreenAdditionalSecsEvenInEditor();
        const auto HoldLoadingScreenAdditionalSecs = CanHoldLoadingScreen
            ? static_cast<double>(ck_loading_screen_cvars::HoldLoadingScreenAdditionalSecs)
            : 0.0;

        if (_TimeLoadingScreenLastDismissed < 0.0)
        { _TimeLoadingScreenLastDismissed = CurrentTime; }

        const auto TimeSinceScreenDismissed = CurrentTime - _TimeLoadingScreenLastDismissed;

        if (HoldLoadingScreenAdditionalSecs > 0.0 && TimeSinceScreenDismissed < HoldLoadingScreenAdditionalSecs)
        {
            // World rendering must be back on during the hold or textures never stream in
            const auto GameViewportClient = GetGameInstance()->GetGameViewportClient();
            GameViewportClient->bDisableWorldRendering = false;

            _DebugReason = ck::Format_UE(
                TEXT("Keeping loading screen up for an additional [{}] seconds to allow texture streaming"),
                HoldLoadingScreenAdditionalSecs);
            WantToForceShowLoadingScreen = true;
        }
    }

    return NeedToShowLoadingScreen || WantToForceShowLoadingScreen;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoGet_IsShowingInitialLoadingScreen() const
    -> bool
{
    const auto PreLoadScreenManager = FPreLoadScreenManager::Get();
    return ck::IsValid(PreLoadScreenManager, ck::IsValid_Policy_NullptrOnly{}) &&
           PreLoadScreenManager->HasValidActivePreLoadScreen();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoShowLoadingScreen()
    -> void
{
    if (_CurrentlyShowingLoadingScreen)
    { return; }

    if (const auto PreLoadScreenManager = FPreLoadScreenManager::Get();
        ck::IsValid(PreLoadScreenManager, ck::IsValid_Policy_NullptrOnly{}) &&
        PreLoadScreenManager->HasActivePreLoadScreenType(EPreLoadScreenTypes::EngineLoadingScreen))
    { return; }

    _TimeLoadingScreenShown = FPlatformTime::Seconds();

    _CurrentlyShowingLoadingScreen = true;

    CSV_EVENT(CkLoadingScreen, TEXT("Show"));

    if (DoGet_IsShowingInitialLoadingScreen())
    {
        ck::loading_screen::Log(TEXT("Showing loading screen while the initial PreLoadScreen is active. Reason: [{}]"),
            _DebugReason);
        return;
    }

    ck::loading_screen::Log(TEXT("Showing loading screen. Reason: [{}]"), _DebugReason);

    const auto LocalGameInstance = GetGameInstance();

    DoStartBlockingInput();

    _OnVisibilityChanged.Broadcast(ECk_LoadingScreen_Visibility::Visible);

    const auto LoadingScreenWidgetClass = TSubclassOf<UUserWidget>{
        UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().TryLoadClass<UUserWidget>()};

    if (const auto UserWidget = UUserWidget::CreateWidgetInstance(*LocalGameInstance, LoadingScreenWidgetClass, NAME_None);
        ck::IsValid(UserWidget))
    {
        _LoadingScreenWidget = UserWidget->TakeWidget();
    }
    else
    {
        // No widget configured is a legitimate choice (the throbber IS the loading screen), so it
        // must not shout. A path that IS set and fails to load is a real content error.
        if (UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().IsNull())
        {
            ck::loading_screen::Verbose(
                TEXT("No loading screen widget configured - using the built-in throbber."));
        }
        else
        {
            ck::loading_screen::Error(TEXT("Failed to load the loading screen widget [{}], falling back to placeholder."),
                UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().ToString());
        }

        _LoadingScreenWidget = SNew(SThrobber);
    }

    const auto GameViewportClient = LocalGameInstance->GetGameViewportClient();
    GameViewportClient->AddViewportWidgetContent(_LoadingScreenWidget.ToSharedRef(),
        UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenZOrder());

    constexpr auto EnablingLoadingScreen = true;
    DoChangePerformanceSettings(EnablingLoadingScreen);

    if (NOT GIsEditor || UCk_Utils_LoadingScreen_Settings_UE::Get_ForceTickLoadingScreenEvenInEditor())
    {
        // Tick Slate to make sure the loading screen is displayed immediately
        FSlateApplication::Get().Tick();
    }
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoHideLoadingScreen()
    -> void
{
    if (NOT _CurrentlyShowingLoadingScreen)
    { return; }

    DoStopBlockingInput();

    if (DoGet_IsShowingInitialLoadingScreen())
    {
        ck::loading_screen::Log(TEXT("Hiding loading screen while the initial PreLoadScreen is active. Reason: [{}]"),
            _DebugReason);
    }
    else
    {
        ck::loading_screen::Log(TEXT("Hiding loading screen. Reason: [{}]"), _DebugReason);

        ck::loading_screen::Log(TEXT("Garbage Collecting before dropping load screen"));
        GEngine->ForceGarbageCollection(true);

        DoRemoveWidgetFromViewport();

        constexpr auto EnablingLoadingScreen = false;
        DoChangePerformanceSettings(EnablingLoadingScreen);

        _OnVisibilityChanged.Broadcast(ECk_LoadingScreen_Visibility::Hidden);
    }

    CSV_EVENT(CkLoadingScreen, TEXT("Hide"));

    const auto LoadingScreenDuration = FPlatformTime::Seconds() - _TimeLoadingScreenShown;
    ck::loading_screen::Log(TEXT("LoadingScreen was visible for [{}]s"), LoadingScreenDuration);

    _CurrentlyShowingLoadingScreen = false;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoRemoveWidgetFromViewport()
    -> void
{
    if (NOT _LoadingScreenWidget.IsValid())
    { return; }

    const auto LocalGameInstance = GetGameInstance();
    if (const auto GameViewportClient = LocalGameInstance->GetGameViewportClient();
        ck::IsValid(GameViewportClient))
    {
        GameViewportClient->RemoveViewportWidgetContent(_LoadingScreenWidget.ToSharedRef());
    }

    _LoadingScreenWidget.Reset();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoStartBlockingInput()
    -> void
{
    if (_InputPreProcessor.IsValid())
    { return; }

    _InputPreProcessor = MakeShared<ck_loading_screen_subsystem::FLoadingScreenInputPreProcessor>();

    constexpr auto ProcessorPriority = 0;
    FSlateApplication::Get().RegisterInputPreProcessor(_InputPreProcessor, ProcessorPriority);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoStopBlockingInput()
    -> void
{
    if (NOT _InputPreProcessor.IsValid())
    { return; }

    FSlateApplication::Get().UnregisterInputPreProcessor(_InputPreProcessor);
    _InputPreProcessor.Reset();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoChangePerformanceSettings(
        bool InEnablingLoadingScreen)
    -> void
{
    const auto LocalGameInstance = GetGameInstance();
    const auto GameViewportClient = LocalGameInstance->GetGameViewportClient();

    FShaderPipelineCache::SetBatchMode(InEnablingLoadingScreen
        ? FShaderPipelineCache::BatchMode::Fast
        : FShaderPipelineCache::BatchMode::Background);

    GameViewportClient->bDisableWorldRendering = InEnablingLoadingScreen;

    if (const auto ViewportWorld = GameViewportClient->GetWorld();
        ck::IsValid(ViewportWorld))
    {
        constexpr auto CheckStreamingPersistent = false;
        constexpr auto Precache = false;
        if (const auto WorldSettings = ViewportWorld->GetWorldSettings(CheckStreamingPersistent, Precache);
            ck::IsValid(WorldSettings))
        {
            WorldSettings->bHighPriorityLoadingLocal = InEnablingLoadingScreen;
        }
    }

    if (InEnablingLoadingScreen)
    {
        auto HangDurationMultiplier = double{};
        if (ck::Is_NOT_Valid(GConfig, ck::IsValid_Policy_NullptrOnly{}) ||
            NOT GConfig->GetDouble(TEXT("Core.System"), TEXT("LoadingScreenHangDurationMultiplier"),
                /*out*/ HangDurationMultiplier, GEngineIni))
        {
            HangDurationMultiplier = 1.0;
        }
        FThreadHeartBeat::Get().SetDurationMultiplier(HangDurationMultiplier);

        FGameThreadHitchHeartBeat::Get().SuspendHeartBeat();
    }
    else
    {
        FThreadHeartBeat::Get().SetDurationMultiplier(1.0);

        FGameThreadHitchHeartBeat::Get().ResumeHeartBeat();
    }
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    Get_IsPresentationSuppressed()
    -> bool
{
    return IsRunningCommandlet() ||
           FApp::IsUnattended() ||
           NOT FApp::CanEverRender() ||
           ck_loading_screen_cvars::DisableLoadingScreen;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoRequestTransitionAssets()
    -> void
{
    if (NOT UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionScreenEnabled())
    { return; }

    if (NOT UAssetManager::IsInitialized())
    { return; }

    auto PathsToLoad = TArray<FSoftObjectPath>{};

    if (const auto& LogoPath = UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoTexture();
        NOT LogoPath.IsNull())
    { PathsToLoad.Add(LogoPath); }

    for (const auto& BackgroundPath : UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionBackgroundImages())
    {
        if (BackgroundPath.IsNull())
        { continue; }

        PathsToLoad.Add(BackgroundPath);
    }

    if (PathsToLoad.IsEmpty())
    { return; }

    // Async and unwaited-on. If the very first travel beats the load in, the module simply builds
    // no brush for whatever is not resident yet - never a sync load, never a block.
    _TransitionAssetsHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(PathsToLoad);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoTickTransitionHandshake()
    -> void
{
    // Only a handshake the module armed may stop a movie - that is what keeps this from cutting
    // short a startup-movie playback it had no part in setting up.
    if (NOT ck::loading_screen::transition::Get_IsHandshakeArmed())
    { return; }

    const auto MoviePlayer = GetMoviePlayer();
    if (ck::Is_NOT_Valid(MoviePlayer, ck::IsValid_Policy_NullptrOnly{}) ||
        NOT MoviePlayer->IsMovieCurrentlyPlaying())
    { return; }

    // MOUNTED, not painted. While the movie owns the window its content has replaced the game
    // viewport entirely (FDefaultGameMoviePlayer::WaitForMovieToFinish), so a viewport widget
    // CANNOT paint until after the movie stops - waiting for a paint would burn the fail-open cap
    // on every single travel. Mounted is the signal that actually matters: the screen is already in
    // the viewport when the movie tears down, so there is no frame of raw world in between.
    if (_CurrentlyShowingLoadingScreen && _LoadingScreenWidget.IsValid())
    {
        ck::loading_screen::Log(
            TEXT("Transition handshake satisfied - stopping the movie, the game-thread screen is mounted"));

        ck::loading_screen::transition::Request_DisarmHandshake();
        _TimeTransitionLoadingFinished = -1.0;
        MoviePlayer->StopMovie();
        return;
    }

    if (NOT MoviePlayer->IsLoadingFinished())
    {
        _TimeTransitionLoadingFinished = -1.0;
        return;
    }

    const auto CurrentTime = FPlatformTime::Seconds();
    if (_TimeTransitionLoadingFinished < 0.0)
    {
        _TimeTransitionLoadingFinished = CurrentTime;
        return;
    }

    if (CurrentTime - _TimeTransitionLoadingFinished < ck_loading_screen_subsystem::TransitionHandshakeFailOpenSecs)
    { return; }

    ck::loading_screen::Warning(
        TEXT("Transition handshake timed out after [{}]s with no game-thread screen mounted - stopping the movie anyway"),
        ck_loading_screen_subsystem::TransitionHandshakeFailOpenSecs);

    ck::loading_screen::transition::Request_DisarmHandshake();
    _TimeTransitionLoadingFinished = -1.0;
    MoviePlayer->StopMovie();
}

// --------------------------------------------------------------------------------------------------------------------

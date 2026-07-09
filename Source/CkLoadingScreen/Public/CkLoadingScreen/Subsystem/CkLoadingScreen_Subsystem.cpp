#include "CkLoadingScreen_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkLoadingScreen/CkLoadingScreen_Log.h"
#include "CkLoadingScreen/LoadingProcess/CkLoadingProcess_Interface.h"
#include "CkLoadingScreen/Settings/CkLoadingScreen_Settings.h"

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/GameViewportClient.h>
#include <Framework/Application/IInputProcessor.h>
#include <Framework/Application/SlateApplication.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/WorldSettings.h>
#include <HAL/ThreadHeartBeat.h>
#include <Misc/App.h>
#include <Misc/CommandLine.h>
#include <Misc/ConfigCacheIni.h>
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
    // Input processor registered while the loading screen is shown.
    // Captures all input so menus underneath the loading screen cannot be interacted with.
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

    // We are done, so do not attempt to tick us again
    SetTickableTickType(ETickableTickType::Never);
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    // Only clients have loading screens
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
    // Don't tick if we don't have a game viewport client, this catches cases that ShouldCreateSubsystem does not
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

    // Update the loading screen immediately if the engine is initialized
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
        // If we don't make it to the specified checkpoint in the given time, trigger the hang detector
        // so we can better determine where progress stalled.
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
    // Start out with 'unknown' reason in case someone forgets to put a reason when changing this in the future.
    _DebugReason = TEXT("Reason for Showing/Hiding LoadingScreen is unknown!");

    const auto LocalGameInstance = GetGameInstance();

    if (ck_loading_screen_cvars::ForceLoadingScreenVisible)
    {
        _DebugReason = TEXT("ck.LoadingScreen.AlwaysShow is true");
        return true;
    }

    const auto Context = LocalGameInstance->GetWorldContext();
    if (Context == nullptr)
    {
        // We don't have a world context right now... better show a loading screen
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
        // The game state has not yet replicated.
        _DebugReason = TEXT("GameState hasn't yet replicated (it's null)");
        return true;
    }

    if (_CurrentlyInLoadMap)
    {
        // Show a loading screen if we are in LoadMap
        _DebugReason = TEXT("_CurrentlyInLoadMap is true");
        return true;
    }

    if (NOT Context->TravelURL.IsEmpty())
    {
        // Show a loading screen when pending travel
        _DebugReason = TEXT("We have pending travel (the TravelURL is not empty)");
        return true;
    }

    if (Context->PendingNetGame != nullptr)
    {
        // Connecting to another server
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
        // Show a loading screen during seamless travel
        _DebugReason = TEXT("We are in seamless travel");
        return true;
    }

    // Ask the game state if it needs a loading screen
    if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(GameState, /*out*/ _DebugReason))
    { return true; }

    // Ask any game state components if they need a loading screen
    for (const auto TestComponent : GameState->GetComponents())
    {
        if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(TestComponent, /*out*/ _DebugReason))
        { return true; }
    }

    // Ask any of the external loading processors that may have been registered. These might be actors or
    // components that were registered by game code to tell us to keep the loading screen up while perhaps
    // something finishes streaming in.
    for (const auto& Processor : _ExternalLoadingProcessors)
    {
        if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(Processor.GetObject(), /*out*/ _DebugReason))
        { return true; }
    }

    // Check each local player
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

        // Ask the PC itself if it needs a loading screen
        if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(PlayerController, /*out*/ _DebugReason))
        { return true; }

        // Ask any PC components if they need a loading screen
        for (const auto TestComponent : PlayerController->GetComponents())
        {
            if (ICk_LoadingProcess::Get_ShouldShowLoadingScreen(TestComponent, /*out*/ _DebugReason))
            { return true; }
        }
    }

    const auto GameViewportClient = LocalGameInstance->GetGameViewportClient();
    const auto IsInSplitscreen = GameViewportClient->GetCurrentSplitscreenConfiguration() != ESplitScreenType::None;

    // In splitscreen we need all player controllers to be present
    if (IsInSplitscreen && MissingAnyLocalPC)
    {
        _DebugReason = TEXT("At least one missing local player controller in splitscreen");
        return true;
    }

    // And in non-splitscreen we need at least one player controller to be present
    if (NOT IsInSplitscreen && NOT FoundAnyLocalPC)
    {
        _DebugReason = TEXT("Need at least one local player controller");
        return true;
    }

    // Victory! The loading screen can go away now
    _DebugReason = TEXT("(nothing wants to show it anymore)");
    return false;
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoShouldShowLoadingScreen()
    -> bool
{
    if (DoGet_IsPresentationSuppressed())
    {
        _DebugReason = TEXT("Loading screen presentation is suppressed (commandlet/unattended/null-RHI or ck.LoadingScreen.Disable)");
        return false;
    }

    // Check debugging commands that force the state one way or another
#if NOT UE_BUILD_SHIPPING
    static const auto CmdLineNoLoadingScreen = FParse::Param(FCommandLine::Get(), TEXT("NoLoadingScreen"));
    if (CmdLineNoLoadingScreen)
    {
        _DebugReason = TEXT("CommandLine has 'NoLoadingScreen'");
        return false;
    }
#endif

    // Can't show a loading screen if there's no game viewport
    const auto LocalGameInstance = GetGameInstance();
    if (ck::Is_NOT_Valid(LocalGameInstance->GetGameViewportClient()))
    { return false; }

    // Check for a need to show the loading screen
    const auto NeedToShowLoadingScreen = DoCheckForAnyNeedToShowLoadingScreen();

    // Keep the loading screen up a bit longer if desired
    auto WantToForceShowLoadingScreen = false;

    if (NeedToShowLoadingScreen)
    {
        // Still need to show it
        _TimeLoadingScreenLastDismissed = -1.0;
    }
    else
    {
        // Don't *need* to show the screen anymore, but might still want to for a bit
        const auto CurrentTime = FPlatformTime::Seconds();
        const auto CanHoldLoadingScreen =
            NOT GIsEditor || UCk_Utils_LoadingScreen_Settings_UE::Get_HoldLoadingScreenAdditionalSecsEvenInEditor();
        const auto HoldLoadingScreenAdditionalSecs = CanHoldLoadingScreen
            ? static_cast<double>(ck_loading_screen_cvars::HoldLoadingScreenAdditionalSecs)
            : 0.0;

        if (_TimeLoadingScreenLastDismissed < 0.0)
        { _TimeLoadingScreenLastDismissed = CurrentTime; }

        const auto TimeSinceScreenDismissed = CurrentTime - _TimeLoadingScreenLastDismissed;

        // Hold for an extra X seconds, to cover up streaming
        if (HoldLoadingScreenAdditionalSecs > 0.0 && TimeSinceScreenDismissed < HoldLoadingScreenAdditionalSecs)
        {
            // Make sure we're rendering the world at this point, so that textures will actually stream in
            //@TODO: If NeedToShowLoadingScreen bounces back true during this window, we won't turn this off again...
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
    return PreLoadScreenManager != nullptr && PreLoadScreenManager->HasValidActivePreLoadScreen();
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoShowLoadingScreen()
    -> void
{
    if (_CurrentlyShowingLoadingScreen)
    { return; }

    // Unable to show loading screen if the engine is still loading with its loading screen.
    if (FPreLoadScreenManager::Get() &&
        FPreLoadScreenManager::Get()->HasActivePreLoadScreenType(EPreLoadScreenTypes::EngineLoadingScreen))
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

    // Eat input while the loading screen is displayed
    DoStartBlockingInput();

    _OnVisibilityChanged.Broadcast(ECk_LoadingScreen_Visibility::Visible);

    // Create the loading screen widget
    const auto LoadingScreenWidgetClass = TSubclassOf<UUserWidget>{
        UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().TryLoadClass<UUserWidget>()};

    if (const auto UserWidget = UUserWidget::CreateWidgetInstance(*LocalGameInstance, LoadingScreenWidgetClass, NAME_None);
        ck::IsValid(UserWidget))
    {
        _LoadingScreenWidget = UserWidget->TakeWidget();
    }
    else
    {
        ck::loading_screen::Error(TEXT("Failed to load the loading screen widget [{}], falling back to placeholder."),
            UCk_Utils_LoadingScreen_Settings_UE::Get_LoadingScreenWidget().ToString());
        _LoadingScreenWidget = SNew(SThrobber);
    }

    // Add to the viewport at a high ZOrder to make sure it is on top of most things
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

        // Let observers know that the loading screen is done
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

    // Don't bother drawing the 3D world while we're loading
    GameViewportClient->bDisableWorldRendering = InEnablingLoadingScreen;

    // Make sure to prioritize streaming in levels if the loading screen is up
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
        // Set a new hang detector timeout multiplier when the loading screen is visible.
        auto HangDurationMultiplier = double{};
        if (NOT GConfig ||
            NOT GConfig->GetDouble(TEXT("Core.System"), TEXT("LoadingScreenHangDurationMultiplier"),
                /*out*/ HangDurationMultiplier, GEngineIni))
        {
            HangDurationMultiplier = 1.0;
        }
        FThreadHeartBeat::Get().SetDurationMultiplier(HangDurationMultiplier);

        // Do not report hitches while the loading screen is up
        FGameThreadHitchHeartBeat::Get().SuspendHeartBeat();
    }
    else
    {
        // Restore the hang detector timeout when we hide the loading screen
        FThreadHeartBeat::Get().SetDurationMultiplier(1.0);

        // Resume reporting hitches now that the loading screen is down
        FGameThreadHitchHeartBeat::Get().ResumeHeartBeat();
    }
}

auto
    UCk_LoadingScreen_Subsystem_UE::
    DoGet_IsPresentationSuppressed()
    -> bool
{
    return IsRunningCommandlet() ||
           FApp::IsUnattended() ||
           NOT FApp::CanEverRender() ||
           ck_loading_screen_cvars::DisableLoadingScreen;
}

// --------------------------------------------------------------------------------------------------------------------

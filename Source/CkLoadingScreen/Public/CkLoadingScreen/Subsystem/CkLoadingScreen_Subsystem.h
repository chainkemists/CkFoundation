#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkLoadingScreen/CkLoadingScreen_Common.h"

#include <Subsystems/GameInstanceSubsystem.h>
#include <Tickable.h>
#include <UObject/WeakInterfacePtr.h>

#include "CkLoadingScreen_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ICk_LoadingProcess;
class IInputProcessor;
class SWidget;
class FSubsystemCollectionBase;
struct FStreamableHandle;
struct FWorldContext;
template <typename InterfaceType> class TScriptInterface;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Dependency-gated loading screen manager.
 *
 * Evaluates every tick whether ANY condition needs the loading screen (in LoadMap, pending travel,
 * connecting, GameState not yet replicated, world hasn't begun play, seamless travel, or any
 * ICk_LoadingProcess holder — polled on the GameState, its components, local PlayerControllers and
 * their components, plus explicitly registered processors). The screen drops only when nothing
 * holds it, then optionally stays up HoldAdditionalSecs with world rendering re-enabled so texture
 * streaming and PSO compilation can warm up behind it.
 *
 * Presentation (widget, input block, render suppression) is automatically suppressed for
 * commandlets, -unattended, and null-RHI runs, and via ck.LoadingScreen.Disable — the holder
 * bookkeeping still runs so tests can assert on Get_NeedsLoadingScreen().
 *
 * Restyled from Lyra's CommonLoadingScreen ULoadingScreenManager (see the module design notes for
 * port provenance and deliberate divergences).
 */
UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_LoadingScreen")
class CKLOADINGSCREEN_API UCk_LoadingScreen_Subsystem_UE
    : public UGameInstanceSubsystem
    , public FTickableGameObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_LoadingScreen_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

public:
    auto Tick(float InDeltaTime) -> void override;
    auto GetTickableTickType() const -> ETickableTickType override;
    auto IsTickable() const -> bool override;
    auto GetStatId() const -> TStatId override;
    auto GetTickableGameObjectWorld() const -> UWorld* override;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Debug Reason")
    FString
    Get_DebugReason() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Is Loading Screen Showing")
    bool
    Get_IsLoadingScreenShowing() const;

    /**
     * Evaluates the raw "does anything need the loading screen" check right now and returns the
     * result (updating Get_DebugReason). Works even when presentation is suppressed — this is the
     * assertion surface for automated tests.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Needs Loading Screen")
    bool
    Get_NeedsLoadingScreen();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Bind To OnVisibilityChanged")
    void
    BindTo_OnVisibilityChanged(
        const FCk_Delegate_LoadingScreen_OnVisibilityChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Unbind From OnVisibilityChanged")
    void
    UnbindFrom_OnVisibilityChanged(
        const FCk_Delegate_LoadingScreen_OnVisibilityChanged& InDelegate);

public:
    auto
    Register_LoadingProcessor(
        TScriptInterface<ICk_LoadingProcess> InInterface) -> void;

    auto
    Unregister_LoadingProcessor(
        TScriptInterface<ICk_LoadingProcess> InInterface) -> void;

public:
    /**
     * Commandlet / -unattended / null-RHI / ck.LoadingScreen.Disable — presentation off, bookkeeping
     * on. Public because the MoviePlayer transition layer gates on the SAME list; two copies of it
     * would eventually disagree about whether a run may present anything.
     */
    static auto Get_IsPresentationSuppressed() -> bool;

private:
    auto DoHandlePreLoadMap(const FWorldContext& InWorldContext, const FString& InMapName) -> void;
    auto DoHandlePostLoadMap(UWorld* InWorld) -> void;

    /** Determines whether to show or hide the loading screen. Called every tick. */
    auto DoUpdateLoadingScreen() -> void;

    /** Returns true if anything NEEDS the loading screen up right now. */
    auto DoCheckForAnyNeedToShowLoadingScreen() -> bool;

    /** Returns true if we WANT the screen up (needed, or held for the additional-secs window). */
    auto DoShouldShowLoadingScreen() -> bool;

    /** Returns true while the engine's startup PreLoadScreen still owns the screen. */
    auto DoGet_IsShowingInitialLoadingScreen() const -> bool;

    auto DoShowLoadingScreen() -> void;
    auto DoHideLoadingScreen() -> void;
    auto DoRemoveWidgetFromViewport() -> void;

    auto DoStartBlockingInput() -> void;
    auto DoStopBlockingInput() -> void;

    auto DoChangePerformanceSettings(bool InEnablingLoadingScreen) -> void;

    /**
     * Roots the transition screen's textures for the subsystem's whole life. The Slate widget that
     * draws them runs on the loading thread and must never touch a UObject, so it only ever gets
     * pre-built brushes — this handle is what guarantees the textures behind those brushes are
     * resident and GC-safe when the module builds them.
     */
    auto DoRequestTransitionAssets() -> void;

    /** Drives the MoviePlayer -> game-thread-screen handoff. Ticks from inside the blocking wait. */
    auto DoTickTransitionHandshake() -> void;

private:
    UPROPERTY(Transient)
    FCk_Delegate_LoadingScreen_OnVisibilityChanged_MC _OnVisibilityChanged;

    TSharedPtr<SWidget> _LoadingScreenWidget;
    TSharedPtr<IInputProcessor> _InputPreProcessor;
    TSharedPtr<FStreamableHandle> _TransitionAssetsHandle;

    TArray<TWeakInterfacePtr<ICk_LoadingProcess>> _ExternalLoadingProcessors;

    FString _DebugReason;

    double _TimeLoadingScreenShown = 0.0;
    double _TimeLoadingScreenLastDismissed = -1.0;
    double _TimeUntilNextLogHeartbeatSeconds = 0.0;
    double _TimeTransitionLoadingFinished = -1.0;

    bool _CurrentlyInLoadMap = false;
    bool _CurrentlyShowingLoadingScreen = false;
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkLoadingScreen/CkLoadingScreen_Common.h"
#include "CkLoadingScreen/LoadingProcess/CkLoadingProcess_Interface.h"

#include "CkPso/CkPso_Common.h"
#include "CkPso/Drain/CkPso_DrainTracker.h"

#include <Subsystems/GameInstanceSubsystem.h>
#include <Tickable.h>

#include "CkPso_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FSubsystemCollectionBase;
class UCk_LoadingScreen_Subsystem_UE;
struct FWorldContext;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Holds the loading screen until pending PSO work drains, and measures the runtime PSO hitches that got past it.
 *
 * A drain window arms when the loading screen becomes visible, when a map load starts, or on
 * Request_BeginDrainWindow - never restarting a window that is still holding. Each tick feeds
 * FShaderPipelineCache::NumPrecompilesRemaining() (bundled cache + runtime precache requests) into an
 * FCk_Pso_DrainTracker, which completes after a quiet period at zero and fails open on its max-wait and stall
 * budgets.
 *
 * INERT where there is nothing to wait on: with no simulated count, PSO precaching disabled (always the case under
 * WITH_EDITOR) and no bundled cache precompiling, the tracker stays Idle and this never holds, so editor, PIE and
 * headless runs are unchanged.
 *
 * Not created on dedicated servers.
 */
UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_Pso")
class CKPSO_API UCk_Pso_Subsystem_UE
    : public UGameInstanceSubsystem
    , public FTickableGameObject
    , public ICk_LoadingProcess
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Pso_Subsystem_UE);

public:
    UCk_Pso_Subsystem_UE();

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
    auto
    Get_ShouldShowLoadingScreen(
        FString& OutReason) const -> bool override;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Get Drain Progress")
    FCk_Pso_DrainProgress
    Get_DrainProgress() const;

    /** True while the drain window holds: Draining, or Settling through the quiet period. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Get Is Draining")
    bool
    Get_IsDraining() const;

    /** Whether the engine precaches PSOs at runtime on this build and RHI. Always false under WITH_EDITOR. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Get Is Precaching Supported")
    bool
    Get_IsPrecachingSupported() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Get Hitch Report")
    FCk_Pso_HitchReport
    Get_HitchReport() const;

    /** Multi-line drain snapshot + hitch report; what ck.Pso.DumpReport prints. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Get Debug Report")
    FString
    Get_DebugReport() const;

    /**
     * Arms a drain window now, for work a caller knows is coming that no map load or loading screen announces.
     * Does nothing while a window is still holding, or while the gate is inert.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Request Begin Drain Window")
    void
    Request_BeginDrainWindow();

    /** Fires when the drain snapshot changes; elapsed time alone is not a change - poll Get_DrainProgress for a clock. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Bind To OnDrainProgressChanged")
    void
    BindTo_OnDrainProgressChanged(
        const FCk_Delegate_Pso_OnDrainProgressChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Pso",
              DisplayName = "[Ck][Pso] Unbind From OnDrainProgressChanged")
    void
    UnbindFrom_OnDrainProgressChanged(
        const FCk_Delegate_Pso_OnDrainProgressChanged& InDelegate);

private:
    UFUNCTION()
    void
    DoHandle_LoadingScreenVisibilityChanged(
        ECk_LoadingScreen_Visibility InVisibility);

    auto
    DoHandle_PreLoadMap(
        const FWorldContext& InWorldContext,
        const FString& InMapName) -> void;

    auto
    DoMake_VisibilityChangedDelegate() -> FCk_Delegate_LoadingScreen_OnVisibilityChanged;

    auto
    DoTryBegin_DrainWindow(
        const FString& InTrigger) -> void;

    auto
    DoGet_IsInert() const -> bool;

    auto
    DoGet_IsWindowClosed() const -> bool;

    auto
    DoGet_IsGateEnabled() const -> bool;

    auto
    DoGet_NumRemaining() const -> uint32;

    auto
    DoHandle_DrainStateTransition() -> void;

    auto
    DoBroadcast_DrainProgressIfChanged() -> void;

    auto
    DoOpen_GameplayWindow() -> void;

    auto
    DoClose_GameplayWindow() -> void;

private:
    UPROPERTY(Transient)
    FCk_Delegate_Pso_OnDrainProgressChanged_MC _OnDrainProgressChanged;

    TWeakObjectPtr<UCk_LoadingScreen_Subsystem_UE> _LoadingScreenSubsystem;

    FCk_Pso_DrainTracker _DrainTracker;
    ECk_Pso_DrainState _LastObservedDrainState = ECk_Pso_DrainState::Idle;
    FCk_Pso_DrainProgress _LastBroadcastDrainProgress;

    FCk_Pso_HitchReport _HitchBaseline;
    bool _GameplayWindowOpen = false;
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJoltEditor/Cook/CkJoltCook_MeshShapeCooker.h"
#include "CkJoltEditor/Cook/CkJoltCook_Types.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"

#include <AssetRegistry/AssetData.h>
#include <Containers/Ticker.h>
#include <EditorSubsystem.h>
#include <Engine/TimerHandle.h>
#include <Framework/Notifications/NotificationManager.h>
#include <UObject/ObjectSaveContext.h>

#include "CkJoltCook_EditorSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UPackage;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

/// Editor entry point for the Jolt cookers, and the owner of auto-cook-on-save. Cooks run sliced
/// across frames behind a status-bar progress notification (Source/EDITOR_MODULES.md rule 5) —
/// except the full map cook, whose World Partition walk owns its own loop.
UCLASS()
class CKJOLTEDITOR_API UCk_JoltCook_EditorSubsystem_UE : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_JoltCook_EditorSubsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

public:
    /// Blocking, and rewrites every cell. Prefer Cook_CurrentWorld_Incremental for routine re-cooks.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Static World (Current Map)")
    bool
    Cook_CurrentWorld();

    /// What auto-cook-on-save runs. Falls back to a full cook when the incremental path declines.
    /// BLOCKS until finished; prefer Request_CookStaticWorld from interactive code.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Static World (Current Map, Incremental)")
    bool
    Cook_CurrentWorld_Incremental();

    /// The same incremental cook, sliced across frames behind a progress notification. False when a
    /// cook is already running.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Static World (Current Map, Non-Blocking)")
    bool
    Request_CookStaticWorld();

    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Static World (Current Map, Dry Run)")
    bool
    Cook_CurrentWorld_DryRun();

    /// True when nothing is stale. Logs the stale actors; writes nothing.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Validate Cooked Static World (Current Map)")
    bool
    Validate_CurrentWorld();

    /// BLOCKS for as long as it takes to load every candidate mesh — BodySetupGuid is not an
    /// asset-registry tag, so staleness cannot be judged without loading.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Mesh Shapes (Baked Roots)")
    bool
    Cook_MeshShapes();

    /// The same sweep, sliced. False when there is nothing to sweep or one is already running.
    UFUNCTION(BlueprintCallable, Category = "Ck|Jolt",
              DisplayName = "[Ck][Jolt] Cook Mesh Shapes (Baked Roots, Non-Blocking)")
    bool
    Request_CookMeshShapes();

private:
    auto DoHandle_PostSaveWorld(UWorld* InWorld, FObjectPostSaveContext InContext) -> void;
    auto DoHandle_PackageSaved(const FString& InPackageFileName, UPackage* InPackage,
        FObjectPostSaveContext InContext) -> void;

    static auto Get_IsAutoCookAllowed() -> bool;

    static auto Get_IsMapExcludedFromCook(const FString& InMapPackageName) -> bool;

    auto Request_ScheduleAutoCook() -> void;
    auto Execute_ScheduledAutoCook() -> void;

    auto Start_Drain(const FText& InProgressLabel) -> bool;
    auto Tick_Drain() -> bool;
    auto Tick_MeshCooks(FCk_Time InBudget) -> void;
    auto Tick_WorldCook(FCk_Time InBudget) -> bool;
    auto Finish_Drain() -> void;

    auto DoAcquire_JoltGlobals() -> void;
    auto DoRelease_JoltGlobals() -> void;

    auto Dismiss_ProgressNotification() -> void;
    auto Dismiss_DrainTicker() -> void;

private:
    TArray<FSoftObjectPath> _PendingMeshCooks;
    bool _PendingWorldCook = false;

    TArray<FAssetData> _SweepCandidates;
    int32 _SweepNextIndex = 0;
    TSet<FString> _SweepCookedPathsInUse;
    bool _SweepReportsOrphans = false;

    FCk_Jolt_MeshShapeCooker::FCookStats _DrainStats;
    int32 _DrainCompletedItems = 0;
    int32 _DrainTotalItems = 0;
    bool _DrainWorldCookPending = false;
    TUniquePtr<FCk_Jolt_IncrementalCookDriver> _WorldCookDriver;
    int32 _WorldCookUnitsCounted = 0;

    /// Held for the whole drain: a per-frame release would UnregisterTypes and delete the Factory
    /// out from under the JPH::Refs the runtime shape cache holds.
    bool _HoldsJoltGlobals = false;

    FTimerHandle _AutoCookDebounceTimer;
    FTSTicker::FDelegateHandle _DrainTickerHandle;
    FProgressNotificationHandle _ActiveProgressNotification;

    FDelegateHandle _PostSaveWorldHandle;
    FDelegateHandle _PackageSavedHandle;
};

// --------------------------------------------------------------------------------------------------------------------

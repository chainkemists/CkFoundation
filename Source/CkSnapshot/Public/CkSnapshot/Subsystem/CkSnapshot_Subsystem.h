#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Delegates.h"

#include <Subsystems/GameInstanceSubsystem.h>

#include "Containers/Ticker.h"

#include "CkSnapshot_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_Snapshot")
class CKSNAPSHOT_API UCk_Snapshot_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Snapshot_Subsystem_UE);

public:
    // Saves the current ECS world to InSlotName. Server/authority only. Pumps the world to quiescence
    // (via UCk_EcsWorld_Subsystem_UE::Request_PumpToQuiescence) before capturing so the snapshot reflects
    // a settled world, then writes a UCk_Snapshot_SaveGame to the slot. Broadcasts OnPreSave / OnSaveComplete
    // and fires InDelegate with the result.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Save",
              meta = (AutoCreateRefTerm = "InDelegate"))
    void
    Request_Save(
        FName InSlotName,
        const FCk_Delegate_OnSaveComplete& InDelegate);

    // Loads the snapshot in InSlotName back into the current ECS world. Server/authority only. Validates the
    // format version, then restores via the manifest-driven loader. Broadcasts OnPreLoad / OnLoadComplete and
    // fires InDelegate with the load report.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Load",
              meta = (AutoCreateRefTerm = "InDelegate"))
    void
    Request_Load(
        FName InSlotName,
        const FCk_Delegate_OnLoadComplete& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Save Slot")
    bool
    Get_HasSaveSlot(
        FName InSlotName) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Save Slot Header")
    FCk_Snapshot_Header
    Get_SaveSlotHeader(
        FName InSlotName) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Try Resolve Save Key")
    bool
    TryResolve_SaveKey(
        FGuid InKey,
        FCk_Handle& OutHandle) const;

public:
    // SaveKey resolver -- maps a stable FGuid (stored on the FFragment_SaveKey of a saved entity) to the
    // live FCk_Handle. Populated during load so post-load consumers can re-acquire entities by key.
    auto Publish_SaveKey(FGuid InKey, FCk_Handle InHandle) -> void;
    auto Consume_SaveKey(FGuid InKey) -> void;

public:
    // True from the start of a Request_Load until OnLoadComplete fires (spans real frames). Distinct from the
    // synchronous _SnapshotInProgress save guard. The live signal M2b's bridged-actor abstention reads at BeginPlay.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Load In Progress")
    bool
    Get_IsLoadInProgress() const;

protected:
    virtual auto Deinitialize() -> void override;

#if WITH_AUTOMATION_TESTS
public:
    // Phase 8 test hook: the pump-pass count Request_Save observed on its last invocation.
    auto TestOnly_LastPumpCount() const -> int32 { return _LastPumpCount; }
#endif

private:
    auto DoGet_SnapshotSource() const -> FCk_Handle;

    // ---- M2b-1 load orchestration (extends M2a) ---------------------------------------------------------------
    enum class ELoadPhase : uint8 { Idle, TearingDown, Traveling, AwaitingWorld, Restoring, RespawningActors };

    auto DoInitiate_Teardown() -> void;                  // M2a: Request_DestroyEntity all gameplay roots; record them
    auto DoIs_TeardownComplete() const -> bool;          // M2a: all requested roots now invalid
    auto DoInitiate_Travel() -> void;                    // M2b: OpenLevel the current map (once)
    auto DoIs_NewWorldReady() const -> bool;             // M2b: a new world (!= pre-travel) HasBegunPlay
    auto DoRun_Restore() -> FCk_Snapshot_LoadReport;     // M2b: Run_Restore into the post-travel world
    auto DoStamp_RespawnMarkers() -> int32;              // M2b-2a: stamp FTag_ActorRespawn_Pending on restored bridged entities
    auto DoIs_RespawnComplete() const -> bool;           // M2b-2a: true when no entity still carries the marker (processor drained them)
    auto DoTick_Load(float InDeltaSeconds) -> bool;      // FTSTicker callback; advances the machine
    auto DoFinish_Load(const FCk_Snapshot_LoadReport& InReport) -> void; // M2a: clear flag, fire delegate/signal, reset
    auto DoSet_ReconstitutionFlag(bool InInProgress) -> void; // M2b: set flag on the CURRENT world's EcsWorld subsystem

private:
    UPROPERTY(Transient)
    TMap<FGuid, FCk_Handle> _SaveKeyResolverMap;

    bool _SnapshotInProgress = false;
    int32 _LastPumpCount = 0;

    // M2a load state
    bool _LoadInProgress = false;
    ELoadPhase _LoadPhase = ELoadPhase::Idle;
    TArray<uint8> _PendingLoadBytes;
    FCk_Snapshot_Header _PendingLoadHeader;
    FCk_Delegate_OnLoadComplete _PendingLoadDelegate;
    TArray<FCk_Handle> _PendingTeardownRoots;
    FTSTicker::FDelegateHandle _LoadTickerHandle;
    int32 _LoadFrameCount = 0;

    // M2b travel state
    TWeakObjectPtr<UWorld> _PreTravelWorld;  // captured before OpenLevel; AwaitingWorld waits for a different world
    FString _TravelMapName;                  // resolved from the pre-travel world (RemovePIEPrefix)
    FCk_Snapshot_LoadReport _PendingRestoreReport; // stashed between Restoring and the final DoFinish_Load
    int32 _RespawnQuiescenceFramesRemaining = 0;   // post-respawn settle so deferred WithActor constructs abstain
    bool _RespawnQuiescenceStarted = false;        // sentinel: lazy-init the countdown once (a natural terminal 0 must not re-arm it)

    static constexpr int32 kLoad_TeardownFrameCap = 600; // ~10s @ 60fps; abort guard for a stuck/non-ticking world
    static constexpr int32 kLoad_TravelFrameCap = 600;          // abort if the post-travel world never comes up
    static constexpr int32 kLoad_RespawnQuiescenceFrames = 5;   // frames to keep the flag set after the respawn pass
    static constexpr int32 kLoad_RespawnFrameCap = 600;         // abort if the respawn processor never drains the markers
};

// --------------------------------------------------------------------------------------------------------------------

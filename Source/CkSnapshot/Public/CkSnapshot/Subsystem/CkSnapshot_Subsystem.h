#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Delegates.h"

#include <Subsystems/GameInstanceSubsystem.h>

#include "Containers/Ticker.h"

#include <StructUtils/InstancedStruct.h> // DoDeserialize_V3Blob return type

#include "CkSnapshot_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

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
    // synchronous _SnapshotInProgress save guard. The live signal the bridged-actor abstention reads at BeginPlay.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Load In Progress")
    bool
    Get_IsLoadInProgress() const;

protected:
    virtual auto Deinitialize() -> void override;

#if WITH_AUTOMATION_TESTS
public:
    // Test hook: the pump-pass count Request_Save observed on its last invocation.
    auto TestOnly_LastPumpCount() const -> int32 { return _LastPumpCount; }
#endif

private:
    auto DoGet_SnapshotSource() const -> FCk_Handle;

    // ---- v3 rebuild+hydrate load orchestration ------------------------------------------------------------------
    // v3 works WITH the freshly level-reloaded world: teardown + travel give a pristine world (GameMode default
    // pawn + level actors already re-created normally); then, under the load gate, overlay saved state — spawn
    // RuntimeSpawned from recipes, adopt EngineOwned/ConstructSpawned by identity, hydrate saved payloads,
    // reconcile absent children, settle, open the gate.
    // Hydrating is ATOMIC (one ticker callback): enqueue payloads + queue reconcile-destroys + open the gate, so no
    // gated world-tick ever sees pending payloads. That is load-bearing — the hydration dispatcher is load-kernel, so
    // draining it UNDER the gate (kernel-only pump, feature Setups frozen) would let a gate-open Setup stomp a
    // recompute-from-Params value (Velocity/Acceleration). Enqueuing then opening the gate in one callback forces
    // hydration to drain only in post-gate FULL passes, where each feature Setup precedes the late FGroup_Hydration
    // dispatch. Settling then lets the parked reconcile-destroys + residual requests drain over frames.
    enum class ELoadPhase : uint8 { Idle, TearingDown, AwaitingWorld, Rebuilding, Hydrating, Settling };

    auto DoInitiate_Teardown() -> void;                  // Request_DestroyEntity all gameplay roots; record them
    auto DoIs_TeardownComplete() const -> bool;          // all requested roots now invalid
    auto DoInitiate_Travel() -> void;                    // OpenLevel / seamless ServerTravel (once)
    auto DoIs_NewWorldReady() const -> bool;             // a new world (!= pre-travel) HasBegunPlay
    auto DoTick_Load(float InDeltaSeconds) -> bool;      // FTSTicker callback; advances the machine
    auto DoFinish_Load(const FCk_Snapshot_LoadReport& InReport) -> void; // clear flag, fire delegate/signal, reset

    // v3 load helpers
    auto DoGet_LoadWorldEcs() const -> class UCk_EcsWorld_Subsystem_UE*;   // post-travel world's EcsWorld subsystem
    auto DoRehydrate_SaveKeyResolver() -> void;          // repopulate _SaveKeyResolverMap from LIVE FFragment_SaveKey entities
    auto DoDeserialize_V3Blob(const TArray<uint8>& InBytes) const -> FInstancedStruct; // saved-id map-backed handle remap
    auto DoRebuild_Tick() -> bool;                       // resolve/spawn each entry into _SavedIdMap; true == complete
    auto DoHydrate_Enqueue() -> void;                    // write payloads -> FFragment_PendingHydration + tag (once)
    auto DoIs_HydrationComplete() const -> bool;         // no live FTag_Hydration_PendingApply remains
    auto DoReconcile_Queue() -> void;                    // subtractive Request_DestroyEntity of stray labeled children

private:
    UPROPERTY(Transient)
    TMap<FGuid, FCk_Handle> _SaveKeyResolverMap;

    bool _SnapshotInProgress = false;
    int32 _LastPumpCount = 0;

    // load state
    bool _LoadInProgress = false;
    ELoadPhase _LoadPhase = ELoadPhase::Idle;
    FCk_Delegate_OnLoadComplete _PendingLoadDelegate;
    TArray<FCk_Handle> _PendingTeardownRoots;
    FTSTicker::FDelegateHandle _LoadTickerHandle;
    int32 _LoadFrameCount = 0;

    // v3 rebuild+hydrate state (deserialized at Request_Load, consumed across the frame-spanning machine)
    FCk_Snapshot_V3_Tables _V3Tables;
    FCk_Snapshot_HeaderV3  _V3Header;
    TMap<uint32, FCk_Handle> _SavedIdMap;               // saved-id -> live handle (built during Rebuilding)
    TSet<uint32> _SpawnedRuntimeIds;                    // RuntimeSpawned entries we already issued a spawn for
    TSet<uint32> _PersistedIds;                         // every saved entity id — an owner NOT here is the world root/transient
    TSet<uint32> _SkippedIds;                           // entries the loader deliberately does NOT rebuild (boot-infra / unloadable)
    TMap<uint32, TWeakObjectPtr<AActor>> _PendingBridgeActors; // bridged saved-id -> spawned actor awaiting its bridge
    FCk_Snapshot_LoadReport _V3LoadReport;             // accumulates orphan/skip counts; DoFinish_Load reports it
    bool _HydrationEnqueued = false;                   // Hydrating enqueues payloads exactly once
    int32 _SettleFramesRemaining = 0;                  // post-gate frames to let parked destroys + Setups drain
    bool _SettleStarted = false;                       // sentinel: arm the settle countdown once
    int32 _RebuildLastMappedCount = 0;                 // progress tracking: mapped count at the previous rebuild tick
    int32 _RebuildStallTicks = 0;                      // consecutive rebuild ticks with no NEW mapping (early-exit gate)

    // travel state
    TWeakObjectPtr<UWorld> _PreTravelWorld;  // captured before OpenLevel; AwaitingWorld waits for a different world
    FString _TravelMapName;                  // resolved from the pre-travel world (RemovePIEPrefix)

    static constexpr int32 kLoad_TeardownFrameCap = 600; // ~10s @ 60fps; abort guard for a stuck/non-ticking world
    static constexpr int32 kLoad_TravelFrameCap   = 600; // abort if the post-travel world never comes up
    static constexpr int32 kLoad_RebuildFrameCap  = 600; // hard cap if an entry never resolves/spawns
    static constexpr int32 kLoad_RebuildStallTicks = 30; // proceed early once no NEW entity maps for this many ticks
    static constexpr int32 kLoad_HydrateFrameCap  = 600; // abort if the hydration dispatcher never drains
    static constexpr int32 kLoad_SettleFrames     = 3;   // post-gate frames: parked reconcile-destroys + gated Setups drain
};

// --------------------------------------------------------------------------------------------------------------------

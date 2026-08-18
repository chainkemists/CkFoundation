#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"
#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"
#include "CkSnapshot/Snapshot/CkSnapshot_SaveReport.h"
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

    // As Request_Save, plus a menu-facing sidecar (title / screenshot / game fields) written to the
    // same slot. Prefer this from any UI that lists slots — a plain Request_Save leaves the slot
    // with no sidecar, and it will list as untitled and thumbnail-less.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Save With Metadata",
              meta = (AutoCreateRefTerm = "InMetadata,InDelegate"))
    void
    Request_Save_WithMetadata(
        FName InSlotName,
        const FCk_Snapshot_SaveMetadata& InMetadata,
        const FCk_Delegate_OnSaveComplete& InDelegate);

    /**
     * As Request_Save_WithMetadata, but captures the thumbnail itself and saves once it arrives —
     * for saves triggered from GAMEPLAY, where the frame you want is the one on screen right now and
     * there is no menu-open moment to capture at.
     *
     * The capture is frame-deferred (a synchronous viewport read returns black in a packaged build —
     * see Request_CaptureViewportPng), so the SAVE is deferred with it: this returns immediately and
     * the world is captured on the next rendered frame, not this one. Two consequences worth knowing:
     * a UI element pushed between the call and the save does NOT appear in the thumbnail (the engine
     * reads the viewport before Slate composites UMG), and anything that would make the save illegal
     * in the intervening frame is not re-checked.
     *
     * A menu-driven save should NOT use this — it would photograph the menu. Capture at menu-open
     * with UCk_Utils_Snapshot_UE::Request_CaptureViewportThumbnail and pass the bytes through
     * InMetadata to Request_Save_WithMetadata instead.
     *
     * Any thumbnail already on InMetadata is respected and no capture is taken.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Save With Fresh Thumbnail",
              meta = (AutoCreateRefTerm = "InMetadata,InDelegate"))
    void
    Request_Save_WithFreshThumbnail(
        FName InSlotName,
        const FCk_Snapshot_SaveMetadata& InMetadata,
        const FCk_Delegate_OnSaveComplete& InDelegate,
        int32 InThumbnailMaxWidth = 480);

    // Deletes the snapshot AND its sidecar. Returns false when the snapshot slot did not exist or
    // the platform refused the delete; a missing sidecar is not a failure. Refused while a save or
    // load is in flight.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Delete Save Slot")
    bool
    Request_DeleteSaveSlot(
        FName InSlotName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Save Slot")
    bool
    Get_HasSaveSlot(
        FName InSlotName) const;

    // Every snapshot slot on disk, sidecar slots filtered out. Unordered — a menu that wants
    // most-recent-first sorts on Get_SaveSlotMeta's timestamp.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get All Save Slot Names")
    TArray<FName>
    Get_AllSaveSlotNames() const;

    // The slot's menu sidecar. Cheap — it never touches the world payload. Default-constructed
    // (Get_IsPopulated() false) when the slot has no sidecar.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Save Slot Meta")
    FCk_Snapshot_SlotMeta
    Get_SaveSlotMeta(
        FName InSlotName) const;

    // WARNING: this deserializes the ENTIRE world payload to return six fields. For menu listing use
    // Get_SaveSlotMeta instead; this remains for diagnostics that genuinely want the capture header.
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
    // Collision-safe publication: repeated publication by the same entity is
    // idempotent; explicitly shared rendezvous-group members retain the first
    // representative; every unique or mixed collision is diagnosed/rejected.
    auto TryPublish_SaveKey(FGuid InKey, FCk_Handle InHandle) -> bool;
    auto Consume_SaveKey(FGuid InKey) -> void;

public:
    // True from the start of a Request_Load until OnLoadComplete fires (spans real frames). Distinct from the
    // synchronous _SnapshotInProgress save guard. Consumers that must not act mid-load poll this.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Load In Progress")
    bool
    Get_IsLoadInProgress() const;

    // True only for the duration of the SYNCHRONOUS Request_Save call, so it never reads true from
    // a caller on the game thread that is not itself inside the save. A menu uses it to disable
    // slot interaction from the OnPreSave/OnSaveComplete signals, not by polling.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Save In Progress")
    bool
    Get_IsSaveInProgress() const;

    // The report of the most recently FINISHED load (success or abort). Default-constructed (Result=Failed_IO)
    // until the first load completes. The delegate/signal remain the push channel; this is the pull channel for
    // consumers that were not party to the Request_Load call (gates, diagnostics UI).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Last Load Report")
    FCk_Snapshot_LoadReport
    Get_LastLoadReport() const;

    // The report of the most recently attempted SAVE. Carries the result plus any durable value the capture
    // could not carry — today, a durable payload holding a handle to an entity the save did not persist. The
    // save's own return value stays a bare result, so no caller has to change to keep working.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Last Save Report")
    FCk_Snapshot_SaveReport
    Get_LastSaveReport() const;

protected:
    virtual auto Deinitialize() -> void override;

#if WITH_AUTOMATION_TESTS
public:
    auto TestOnly_LastPumpCount() const -> int32 { return _LastPumpCount; }
    auto TestOnly_TryPublish_SaveKeyWithoutDiagnostics(FGuid InKey, FCk_Handle InHandle) -> bool
    { return DoTryPublish_SaveKey(InKey, InHandle, false); }

    // The hydrate frame cap is the quarantine's bounded escape, and proving an escape fires means reaching it —
    // 600 frames of a load that is deliberately going nowhere. Shortening the fence does not weaken what the test
    // proves: the exit taken is the same one, reached the same way. Values <= 0 restore the shipping cap.
    auto TestOnly_Set_HydrateFrameCapOverride(int32 InFrameCap) -> void
    { _TestOnly_HydrateFrameCapOverride = InFrameCap; }
#endif

private:
    auto DoTryPublish_SaveKey(FGuid InKey, FCk_Handle InHandle, bool InDiagnoseCollision) -> bool;
    auto DoGet_SnapshotSource() const -> FCk_Handle;

    // Shared body of Request_Save / Request_Save_WithMetadata. InMetadata is written to the sidecar
    // only when InWriteSidecar is set, so the metadata-free entry point leaves no sidecar behind.
    auto DoRequest_Save(FName InSlotName, const FCk_Snapshot_SaveMetadata& InMetadata, bool InWriteSidecar,
                        const FCk_Delegate_OnSaveComplete& InDelegate) -> void;
    auto DoWrite_SlotMeta(FName InSlotName, const FCk_Snapshot_SaveMetadata& InMetadata,
                          const FCk_Snapshot_HeaderV3& InHeader) const -> void;

    // ---- v3 rebuild+hydrate load orchestration ------------------------------------------------------------------
    // Hydrating is ATOMIC (one ticker callback): enqueue payloads + queue reconcile-destroys + open the gate, so no
    // gated world-tick ever sees pending payloads and hydration can only drain in post-gate FULL passes. Why that is
    // load-bearing, and the whole phase walkthrough: CkSnapshot/CLAUDE.md § "The v3 load machine".
    enum class ELoadPhase : uint8 { Idle, TearingDown, AwaitingWorld, Rebuilding, Hydrating, Settling };

    // Why the hydration quarantine came off. Settled is the healthy path — the payload queue drained and the whole
    // mapped set was released together. The other two are the bounded escapes fail-closed must pair with, and each
    // names every entity it had to force rather than letting the release read as a normal one.
    enum class EQuarantineLift : uint8 { Settled, ForcedAtFrameCap, ForcedAtLoadFinish };

    auto DoInitiate_Teardown() -> void;                  // Request_DestroyEntity all gameplay roots; record them
    auto DoIs_TeardownComplete() const -> bool;          // all requested roots now invalid
    auto DoInitiate_Travel() -> void;                    // OpenLevel / seamless ServerTravel (once)
    auto DoIs_NewWorldReady() const -> bool;             // a new world (!= pre-travel) HasBegunPlay
    auto DoTick_Load(float InDeltaSeconds) -> bool;      // FTSTicker callback; advances the machine
    auto DoFinish_Load(const FCk_Snapshot_LoadReport& InReport) -> void; // clear flag, fire delegate/signal, reset

    auto DoGet_LoadWorldEcs() const -> class UCk_EcsWorld_Subsystem_UE*;   // post-travel world's EcsWorld subsystem
    // Repopulate _SaveKeyResolverMap from LIVE FFragment_SaveKey entities; returns the published count. Re-run every
    // rebuild tick: keys stamped after world-ready (channels are created on demand, ticks into the load) are otherwise
    // unreachable. Full Reset + rescan is the correct shape — the rebuild's own publish-back writes the fragment onto
    // the live entity, so a rescan re-finds every entry it added.
    auto DoRehydrate_SaveKeyResolver() -> int32;
    auto DoDeserialize_V3Blob(const TArray<uint8>& InBytes) const -> FInstancedStruct; // saved-id map-backed handle remap
    auto DoRebuild_Tick() -> bool;                       // resolve/spawn each entry into _SavedIdMap; true == complete
    UFUNCTION()
    void
    DoOnRuntimeEntityScriptConstructed(
        FCk_Handle_EntityScript InEntityScript);
    UFUNCTION()
    void
    DoOnRuntimeEntityScriptSpawnRequestCompleted(
        FCk_Handle InEntity,
        ECk_Request_OperationResult InResult);
    // The ONLY way an entry enters _SkippedIds: pairs the set membership with its reasoned per-entity record.
    auto DoRecord_Skip(const FCk_Snapshot_V3_EntityEntry& InEntry, ECk_Snapshot_SkipReason InReason) -> void;
    auto DoRestore_SavedOwnership() -> void;             // restore mapped lifetime/context links before hydration
    auto DoApply_SavedTransforms() -> void;              // restore each mapped entity's saved WORLD transform (G1)
    auto DoHydrate_Enqueue() -> void;                    // write payloads -> FFragment_PendingHydration + tag (once)

    // Two predicates, deliberately not one. The LIFT waits on the payload queue alone; FINISHING waits on the lift
    // having happened too. Collapsing them is circular — the lift would wait on a condition only the lift can make
    // true, and every load would burn the frame cap and report forced losses it never had.
    auto DoIs_PayloadDrainComplete() const -> bool;      // no live FTag_Hydration_PendingApply remains
    auto DoIs_HydrationComplete() const -> bool;         // the above AND the quarantine is empty

    auto DoStamp_HydrationQuarantine() -> void;                 // quarantine the whole mapped set, at enqueue
    auto DoLift_HydrationQuarantine(EQuarantineLift InReason) -> void; // release the whole set at once
    auto DoGet_HydrateFrameCap() const -> int32;                // kLoad_HydrateFrameCap, or a test's override

    auto DoReconcile_Queue() -> void;                    // subtractive Request_DestroyEntity of stray labeled children

private:
    UPROPERTY(Transient)
    TMap<FGuid, FCk_Handle> _SaveKeyResolverMap;

    bool _SnapshotInProgress = false;
    int32 _LastPumpCount = 0;

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
    TSet<FCk_Handle> _MappedLiveEntities;               // every _SavedIdMap value — a live entity one row already claimed
    TSet<uint32> _SpawnedRuntimeIds;                    // RuntimeSpawned entries we already issued a spawn for
    TSet<FCk_Handle> _RuntimeEntityScriptsAwaitingConstruction; // mapped identity; not yet safe as a definition-build owner
    TSet<uint32> _PersistedIds;                         // every saved entity id — an owner NOT here is the world root/transient
    TSet<uint32> _SkippedIds;                           // entries the loader deliberately does NOT rebuild
    TArray<FCk_Snapshot_SkipRecord> _SkipRecords;       // one reasoned record per _SkippedIds entry; copied into the report
    TMap<uint32, TWeakObjectPtr<AActor>> _PendingBridgeActors; // bridged saved-id -> spawned actor awaiting its bridge
    FCk_Snapshot_LoadReport _V3LoadReport;             // accumulates orphan/skip counts; DoFinish_Load reports it
    FCk_Snapshot_LoadReport _LastLoadReport;           // frozen copy of the last finished load's report (pull channel)
    FCk_Snapshot_SaveReport _LastSaveReport;           // same, for the last attempted save
    bool _HydrationEnqueued = false;                   // Hydrating enqueues payloads exactly once
    bool _QuarantineStamped = false;                   // the mapped set carries FTag_Hydration_Quarantine right now
#if WITH_AUTOMATION_TESTS
    int32 _TestOnly_HydrateFrameCapOverride = 0;       // <= 0 == use kLoad_HydrateFrameCap
#endif
    int32 _SettleFramesRemaining = 0;                  // post-gate frames to let parked destroys + Setups drain
    bool _SettleStarted = false;                       // sentinel: arm the settle countdown once
    int32 _RebuildLastMappedCount = 0;                 // progress tracking: mapped count at the previous rebuild tick
    int32 _RebuildStallTicks = 0;                      // consecutive rebuild ticks with no NEW mapping (early-exit gate)
    bool  _RebuildEscalated = false;                   // kernel quiesced with unresolved rows -> full-scope ticks (see Rebuilding)

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

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h" // FCk_Handle_PendingActorRelay — the client's release carrier

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Persistence/CkPersistenceHydration.h" // FCk_Delegate_Hydration_OnHydrated
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h" // ECk_EcsWorld_LoadHold — the hold this subsystem owns

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"
#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"
#include "CkSnapshot/Snapshot/CkSnapshot_SaveReport.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Delegates.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Signals.h" // FCk_Delegate_Snapshot_OnLoadComplete

#include <Subsystems/GameInstanceSubsystem.h>

#include "Containers/Ticker.h"

#include <StructUtils/InstancedStruct.h> // DoDeserialize_V3Blob return type

#include "CkSnapshot_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;
class UCk_LoadingProcess_Task_UE;

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

    // Existence, nothing more — true for ANY file occupying the slot, including one Request_Load
    // would refuse. The right question for "may I write/delete here"; the wrong one for occupancy.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Save Slot")
    bool
    Get_HasSaveSlot(
        FName InSlotName) const;

    // Loadability, not existence: true only when the slot holds a save Request_Load would accept
    // (CkSnapshot envelope at the current v3 format version, non-empty payload). A foreign file —
    // a legacy SPUD-era save, another game's USaveGame — reports FALSE here while Get_HasSaveSlot
    // reports true, so a menu keying occupancy off THIS renders such slots as empty instead of as
    // undead rows whose load click silently fails.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Compatible Save Slot")
    bool
    Get_HasCompatibleSaveSlot(
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
    /**
     * Queue a one-shot delegate for THIS load's completion. Call it through
     * UCk_Utils_Snapshot_UE::Promise_OnLoadComplete, which owns the no-load-in-progress half of the contract.
     *
     * Held here, on a GameInstance subsystem, rather than bound as an entity signal: every ECS registry is
     * world-scoped, so a bind made before a load's level travel died with the pre-travel world — silently, which
     * is the failure this replaces. A dynamic delegate is a weak UObject plus a function name, so a subscriber
     * the travel destroyed simply no-ops.
     */
    auto Request_AddLoadCompletePromise(const FCk_Delegate_Snapshot_OnLoadComplete& InDelegate) -> void;

    /**
     * Holds an OnHydrated promise made in this load's destination world while a load can still claim InHandle, but
     * before that claim is visible in _MappedLiveEntities. Returns false for the pre-travel/transition worlds, once
     * this load has lifted its quarantine, or when not loading, so the caller can satisfy the ordinary no-pending-work
     * half of the promise immediately. A mapped entry is rebound to the entity's normal one-shot signal; a never-mapped
     * entry fires at load completion with its original handle.
     */
    auto Request_AddHydrationPromise(
        const FCk_Handle& InHandle,
        const FCk_Delegate_Hydration_OnHydrated& InDelegate) -> bool;

public:
    // SaveKey resolver -- maps a stable FGuid (stored on the FFragment_SaveKey of a saved entity) to the
    // live FCk_Handle. Populated during load so post-load consumers can re-acquire entities by key.
    // Collision-safe publication: repeated publication by the same entity is
    // idempotent; explicitly shared rendezvous-group members retain the first
    // representative; every unique or mixed collision is diagnosed/rejected.
    auto TryPublish_SaveKey(FGuid InKey, FCk_Handle InHandle) -> bool;
    auto Consume_SaveKey(FGuid InKey) -> void;

    // A level-authored entity can either relocate its identity to a replacement or retire that identity
    // permanently. Both paths suppress the authored root; retirement cancellation is the atomic rollback path.
    auto Request_BeginSaveKeyRelocation(const FCk_Handle& InSource) -> FGuid;
    auto Request_CompleteSaveKeyRelocation(FCk_Handle& InDestination, const FGuid& InSaveKey) -> bool;
    auto
    Request_BeginSaveKeyRetirement(
        const FCk_Handle& InSource) -> FGuid;
    auto
    Request_CommitSaveKeyRetirement(
        const FGuid& InSaveKey) -> bool;
    auto
    Request_CancelSaveKeyRetirement(
        const FCk_Handle& InSource,
        const FGuid& InSaveKey) -> bool;

public:
    // True from the start of a Request_Load until OnLoadComplete fires (spans real frames). Distinct from the
    // synchronous _SnapshotInProgress save guard. Consumers that must not act mid-load poll this.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Load In Progress")
    bool
    Get_IsLoadInProgress() const;

    // True once THIS load has handed the world back: every payload applied, every request those applies issued
    // drained, physics stepped, overlaps converged, the hold released and game time running again. It is the
    // moment Promise_OnLoadComplete fires, and it is deliberately NOT the inverse of Get_IsLoadInProgress —
    // between them sits every phase in which the world exists but is not yet the player's.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Ready To Resume")
    bool
    Get_IsReadyToResume() const;

    // WHICH load this is, stamped into the travel URL and the replicated fact so a world coming up mid-travel
    // can say which load owns it. Zero until the first Request_Load, monotonic within a session, and OPAQUE
    // beyond that: it carries a session-unique salt so equality means "the same load" rather than "the same
    // ordinal". Do not derive a load count from it and do not persist it.
    auto Get_LoadEpoch() const -> int32;

    // The epoch's SHAPE, as a pure function of its two halves. Public because the property that matters —
    // two GameInstances at the same load COUNT mint different epochs — is a statement about this composition,
    // and asserting it any other way would need two instances that have each run the same number of loads.
    static auto Compose_LoadEpoch(int32 InSalt, int32 InCount) -> int32;

    // Answered at OnWorldBeginPlay for every world, BEFORE its processor graph exists, and SIDE-EFFECTING by
    // design — it applies the time freeze and (on a client) arms the client-side hold. Registered as CkEcs's
    // load-hold seed provider by this module's startup, so CkEcs never learns CkSnapshot exists.
    //
    // BOTH roles get Converging, and the value is measured rather than reasoned. Rebuilding was tried — it is the
    // one phase that suppresses Request_SpawnEntity, which looked like the right guard for a fresh world's
    // BeginPlay frames — and it BROKE the load: the level's own on-demand infrastructure spawns its entity and
    // stamps its SaveKey in exactly those frames, so suppressing them left three EngineOwned rows unresolvable
    // (savekey-miss) and cascaded 64 more into owner-orphaned, against zero on the reference. Measured on
    // PlayerQuickUseHeldItemAfterLoad: mapped 85/orphaned 67 under Rebuilding, mapped 152/orphaned 0 under
    // Converging.
    //
    // The window Rebuilding was meant to guard is real, but it is the PRODUCER's to guard: Get_IsRebuildInProgress
    // is true in every phase, including this one, which is what a level-triggered seeder must ask. The framework
    // cannot close it by refusing spawns here without also refusing the rebuild its own rows depend on.
    // None means do not seed.
    auto DoGet_ShouldHoldWorldAtBoot(UWorld& InWorld) -> ECk_EcsWorld_LoadHold;

    // True while THIS mapped entity's restored state is still the load's to write: a load is in flight, the load
    // mapped this entity, and the global quarantine lift has not run yet. False alone does not prove completion in
    // the destination world's pre-map window; Promise_OnHydrated resolves that ambiguity through the subsystem's
    // pending-promise queue. Membership covers the window between row mapping and the quarantine stamp, where the
    // entity is already the load's but carries no tag yet.
    auto Get_IsHydrationPending(const FCk_Handle& InHandle) const -> bool;

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
    // could not carry — a durable payload holding a handle to an entity the save did not persist, or to one whose
    // saved row cannot rebuild. The save's own return value stays a bare result, so no caller has to change to keep
    // working.
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
    auto TestOnly_Get_IsSaveKeySuppressed(const FGuid& InKey) const -> bool
    { return _SuppressedSaveKeys.Contains(InKey); }

    // The hydrate frame cap is the quarantine's bounded escape, and proving an escape fires means reaching it —
    // 600 frames of a load that is deliberately going nowhere. Shortening the fence does not weaken what the test
    // proves: the exit taken is the same one, reached the same way. Values <= 0 restore the shipping cap.
    auto TestOnly_Set_HydrateFrameCapOverride(int32 InFrameCap) -> void
    { _TestOnly_HydrateFrameCapOverride = InFrameCap; }

    // Same reasoning for the convergence phase's bounded escape: proving it fires means reaching it, and the
    // shipping bound is 180 frames of a phase that is deliberately going nowhere. Shortening the fence does not
    // weaken what a test proves — the exit taken is the same one, reached the same way. Values <= 0 restore the
    // shipping cap.
    auto TestOnly_Set_ConvergenceFrameCapOverride(int32 InFrameCap) -> void
    { _TestOnly_ConvergenceFrameCapOverride = InFrameCap; }

    // WHY the last client hold let go. A test that asserts only "the hold is off" passes identically whether the
    // designed conjunct arrived or the bounded escape gave up on it — and the escape is exactly the outcome such
    // a test exists to rule out. False until a client hold has released at least once this session.
    auto TestOnly_Get_ClientHoldReleasedByCap() const -> bool
    { return _TestOnly_ClientHoldReleasedByCap; }

    // Whether the loader has already said, for THIS load, that it observed a paused world under the hold. A test
    // that pauses mid-load reads this to know the observation landed before unpausing — otherwise it is racing
    // the loader's own ticker to produce the very line it is asserting on.
    auto TestOnly_Get_PauseObservedUnderHold() const -> bool
    { return _PauseObservedUnderHold; }

    // The freeze's single-valued bookkeeping is a contract about TWO worlds, and a test cannot state it without
    // driving two applies directly — there is no load shape that produces a second LIVE world on demand. These
    // forward to the private pair rather than widening it, so the shipping surface is unchanged.
    auto TestOnly_Apply_TimeFreeze(UWorld& InWorld) -> void
    { DoApply_TimeFreeze(InWorld); }

    auto TestOnly_Restore_TimeFreeze(UWorld& InWorld) -> void
    { DoRestore_TimeFreeze(InWorld); }
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
    //
    // Draining is what used to be called Settling: the payload drain, the deferred requests those applies issue,
    // and the parked reconcile-destroys. Converging is new, and is the phase that makes "ready to resume" mean
    // anything — the payload queue being empty is not the world being coherent.
    enum class ELoadPhase : uint8 { Idle, TearingDown, AwaitingWorld, Rebuilding, Hydrating, Draining, Converging };

    // Why the hydration quarantine came off. Settled is the healthy path — the payload queue drained and the whole
    // mapped set was released together. The other two are the bounded escapes fail-closed must pair with, and each
    // names every entity it had to force rather than letting the release read as a normal one.
    enum class EQuarantineLift : uint8 { Settled, ForcedAtFrameCap, ForcedAtLoadFinish };

    auto DoInitiate_Teardown() -> void;                  // Request_DestroyEntity all gameplay roots; record them
    auto DoIs_TeardownComplete() const -> bool;          // all requested roots now invalid
    auto DoInitiate_Travel() -> void;                    // OpenLevel / seamless ServerTravel (once)

    // ONE world-identity rule, shared by the readiness poll and the boot seed so they cannot fork. A world that
    // answers true here is the world THIS load travelled to — not the pre-travel one, and not the seamless
    // transition map, whose package name differs.
    auto DoIs_WorldOwnedByThisLoad(const UWorld& InWorld) const -> bool;
    auto DoIs_NewWorldReady() const -> bool;             // the above AND HasBegunPlay
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
    auto DoBind_PendingHydrationPromises(const FCk_Handle& InMappedHandle) -> void;
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

    // Destroys every entity the rebuild had to construct from a class default because nothing could resolve its
    // archetype, and NAMES each one on the report. The husk is built rather than dropped so its container hydrates
    // intact (the inventory handlers are all-or-nothing); this is the other half of that trade - without it the
    // player keeps an inventory slot and a grid cell that look occupied and can never be used again.
    auto DoReap_UnresolvedArchetypeHusks(FCk_Snapshot_LoadReport& InOutReport) -> void;
    auto DoRestore_SavedOwnership() -> void;             // restore mapped lifetime/context links before hydration
    auto DoApply_SavedTransforms() -> void;              // restore each mapped entity's saved WORLD transform (G1)
    auto DoHydrate_Enqueue() -> void;                    // write payloads -> FFragment_PendingHydration + tag (once)

    // Two predicates, deliberately not one. The LIFT waits on the payload queue alone; FINISHING waits on the lift
    // having happened too. Collapsing them is circular — the lift would wait on a condition only the lift can make
    // true, and every load would burn the frame cap and report forced losses it never had.
    auto DoIs_PayloadDrainComplete() const -> bool;      // no live FTag_Hydration_PendingApply remains
    auto DoIs_HydrationComplete() const -> bool;         // the above AND the quarantine is empty

    auto DoStamp_HydrationQuarantine() -> void;                 // quarantine the whole mapped set, at enqueue
    // Releases the whole set at once, recording anything it had to force INTO the report that is about to be
    // frozen — the abort paths finish with a locally-built report, so writing to the member here would name
    // the loss in a copy nobody reads.
    auto DoLift_HydrationQuarantine(EQuarantineLift InReason, FCk_Snapshot_LoadReport& InOutReport) -> void;
    auto DoGet_HydrateFrameCap() const -> int32;                // kLoad_HydrateFrameCap, or a test's override
    auto DoGet_ConvergenceFrameCap() const -> int32;            // kLoad_ConvergenceFrameCap, or a test's override

    // Reads ck::FCtx_HydrationOutcomes ONCE and sweeps whatever is still queued, so the report says what became
    // of every payload rather than how many reached the queue. Called from DoFinish_Load, never twice per load.
    auto DoFold_HydrationOutcomes(FCk_Snapshot_LoadReport& InOutReport) const -> void;

    // The load's verdict, computed LAST from the folded buckets — a completed load that lost named payloads is
    // Succeeded_WithLoss, never Success. It only ever downgrades: a Failed_* result stands.
    auto DoCompute_LoadResult(FCk_Snapshot_LoadReport& InOutReport) const -> void;

    // Game time does not advance while the load owns the world, and the mechanism is the engine's own: global
    // time dilation, held at the world's floor. UWorld::TimeSeconds stops, every actor tick's DeltaSeconds
    // collapses with it, and therefore every CkTimer, every processor cadence, every catch-up replay and every
    // gameplay deadline expressed in world seconds stops too — for every reader, with no per-reader edit.
    // RealTimeSeconds keeps accruing undilated, which is exactly why the small named set of watchdogs that must
    // outlive the hold reads wall time instead.
    //
    // Scoped to ONE WORLD at a time, never to the load. AWorldSettings::TimeDilation is transient on a per-level
    // actor whose constructor sets 1.0, so the freeze does NOT survive travel: the post-travel world must be
    // frozen again, and the prior value is captured from — and restored to — the world it was applied to. A load
    // therefore applies twice, and the second apply must not be mistaken for a redundant one.
    auto DoApply_TimeFreeze(UWorld& InWorld) -> void;
    auto DoRestore_TimeFreeze(UWorld& InWorld) -> void;

    // A paused world does not tick, and every phase of a load is driven BY world ticks — the rebuild's kernel
    // passes, the payload drain, the convergence pump. So a pause taken while the hold is on does not pause the
    // load; it stops it, and the load then spends its whole frame budget going nowhere before escaping at the
    // caps and reporting Succeeded_WithLoss for facts that were never given a chance to converge.
    //
    // The framework does not refuse the pause: pausing is a game-side decision, taken by a game-side menu, and a
    // world subsystem is the wrong place to veto it (the sanctioned guard is the pause UI declining while
    // Get_IsLoadInProgress). What it does is refuse to be SILENT about it — ONE Warning per load, naming the
    // world and the phase, so the resulting cap-escape is explained rather than mysterious.
    auto DoObserve_PauseUnderHold(const UWorld& InWorld, const TCHAR* InSide, int32 InEpoch) -> void;

    // The ?CkLoad= option is a ONE-SHOT: it tells the world coming up right now which load owns it, and it has
    // no business surviving into the next travel. It would, though — every relative travel inherits the whole
    // option array from FWorldContext::LastURL, and FWorldContext::LastRemoteURL is replayed verbatim by
    // `reconnect` — so a finished load's epoch would re-arm a client hold on a later map change or a reconnect,
    // and would ride the join URL to anyone connecting through it. Struck from BOTH as it is consumed, which is
    // where and how the engine strikes its own one-shots (Listen, failed, closed). UWorld::URL is deliberately
    // left intact: that is the record of how THIS world came up, and the arm gate reads it.
    auto DoConsume_LoadEpochOption(UWorld& InWorld) const -> void;

    auto DoReconcile_Queue() -> void;                    // subtractive Request_DestroyEntity of stray labeled children

    // ---- The hold ------------------------------------------------------------------------------------------------
    // Every write of the world's load hold goes through here, so a phase transition and the hold that expresses it
    // can never disagree. A world that has gone away is not an error — a load's own travel is exactly that.
    auto DoSet_LoadHold(ECk_EcsWorld_LoadHold InHold) -> void;

    // The load's terminal transition, and the only place Ready_ToResume becomes true: the hold comes off, game
    // time restarts, the loading screen this load was holding is released, and the fact is published to clients.
    // Then DoFinish_Load closes the report and fires the promises — so every callback runs on a normal world.
    auto DoEnter_ReadyToResume() -> void;

    // Once per Converging frame: run the registered advances (the physics grant is one), pump, and record what
    // the pump found into ck::FCtx_LoadConvergence. Deliberately an ACTION, called from one place, so the
    // predicates that read the result can stay pure.
    auto DoDrive_Convergence() -> void;

    // Says how the convergence phase is going, at Display, in bounded form: one line the frame a fact flips to
    // satisfied, one summary when the phase ends, and — if it ends at the cap — the trailing pump/skip series
    // beside the per-name Errors, so the question "what kept it alive" is answerable from the log alone rather
    // than from a repro. Per-frame detail stays at Verbose.
    auto DoReport_ConvergenceProgress(const TArray<FName>& InPending) -> void;

    // A convergence that is still pending well past the point a healthy load has finished is the one case where
    // the pump-count series is not enough: it says HOW MUCH work there is, never WHOSE. Per-processor attribution
    // is behind ck.Scheduler.DebugTiming, which is off in a normal run for cost reasons — so the loader turns it
    // on for itself, only once the phase is visibly stuck, and puts it back exactly as it found it on the way out.
    auto DoArm_ConvergenceDebugTiming() -> void;
    auto DoRestore_ConvergenceDebugTiming() -> void;

    // ONE Display block at the cap, naming the processors that kept the world awake and the entities the destroy
    // queue is still holding. Bounded: top 16 processors per tick group, at most 8 entities.
    auto DoReport_ConvergenceStall() const -> void;
    auto DoGet_ConvergenceSeriesText(const TArray<int32>& InSeries) const -> FString;
    auto DoGet_ConvergenceGrantedSteps() const -> int32;

    // Every remaining Pending row, recorded as a named loss with the frames it waited. The bounded escape's
    // report half.
    auto DoRecord_ConvergenceUnmet(FCk_Snapshot_LoadReport& InOutReport) const -> void;

    // ---- The loading screen --------------------------------------------------------------------------------------
    // Held for the WHOLE load, released on every route out of one. No watchdog: a load runs a blocking LoadMap,
    // package loads and PSO warm-up, so a wall-clock timeout would fire on a healthy slow load and drop the screen
    // over a half-rebuilt world. The loader's own frame caps are the bound.
    auto DoCreate_LoadScreenHold(UObject* InWorldContext, const FString& InReason,
                                 TObjectPtr<UCk_LoadingProcess_Task_UE>& InOutHold) const -> void;
    auto DoRelease_LoadScreenHold(TObjectPtr<UCk_LoadingProcess_Task_UE>& InOutHold) const -> void;

    // ---- The client half -----------------------------------------------------------------------------------------
    // A client has no load, no report and no completion — but on a listen-server reload it travels and rebuilds
    // too, so its world needs the same hold for the same span. Armed from the travel URL (the server cannot reach
    // it any earlier than that) and released on the server's own ready-to-resume fact.
    auto DoAcquire_LoadStateChannel(UWorld& InWorld) -> void;
    auto DoPublish_LoadState(bool InReadyToResume) -> void;
    auto DoBegin_ClientHold(UWorld& InWorld, int32 InEpoch) -> void;
    auto DoTick_ClientHold(float InDeltaSeconds) -> bool;
    auto DoRelease_ClientHold(const TCHAR* InReason) -> void;


private:
    UPROPERTY(Transient)
    TMap<FGuid, FCk_Handle> _SaveKeyResolverMap;
    TSet<FGuid> _SuppressedSaveKeys;
    TSet<FGuid> _PendingSaveKeyRetirements;
    TSet<FCk_Handle> _SuppressedSaveKeyDestroyQueued;

    bool _SnapshotInProgress = false;
    int32 _LastPumpCount = 0;

    bool _LoadInProgress = false;
    ELoadPhase _LoadPhase = ELoadPhase::Idle;
    FCk_Delegate_OnLoadComplete _PendingLoadDelegate;
    // One-shot promises queued through Promise_OnLoadComplete. Distinct from _PendingLoadDelegate, which is the
    // ONE delegate the Request_Load caller passed: different delegate type (it carries no handle), one slot by
    // definition, and already travel-safe for the same reason this list is — both live on the subsystem.
    TArray<FCk_Delegate_Snapshot_OnLoadComplete> _PendingLoadCompletePromises;
    struct FPendingHydrationPromise
    {
        FCk_Handle _Handle;
        FCk_Delegate_Hydration_OnHydrated _Delegate;
    };
    // Pre-map only: entities already in _MappedLiveEntities bind directly to their signal. This stays on the
    // GameInstance because the destination entity has no hydration signal edge until its saved row maps to it.
    TArray<FPendingHydrationPromise> _PendingHydrationPromises;
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
    bool _QuarantineLifted  = false;                   // this load's global lift has run (by settle or either escape)
#if WITH_AUTOMATION_TESTS
    int32 _TestOnly_HydrateFrameCapOverride = 0;       // <= 0 == use kLoad_HydrateFrameCap
    int32 _TestOnly_ConvergenceFrameCapOverride = 0;   // <= 0 == use kLoad_ConvergenceFrameCap
    bool  _TestOnly_ClientHoldReleasedByCap = false;   // the last client-hold release was the bounded escape
#endif
    int32 _SettleFramesRemaining = 0;                  // Draining floor: frames to let parked destroys + Setups drain
    bool _SettleStarted = false;                       // sentinel: arm the settle countdown once
    int32 _ConvergenceFramesSatisfied = 0;             // consecutive Converging frames with nothing Pending

    // Convergence diagnostics. A phase that either converges invisibly or burns its cap and names a row is one a
    // reader cannot reconstruct from the outcome alone: the questions that matter afterwards are WHICH facts were
    // slow, and what the pump was still finding while they were. These three are what makes that answerable from
    // any log, and they are bounded — a rolling window, never a per-frame transcript.
    // Guarded with the only pair that reads them: both halves of arm/restore are non-shipping, so a shipping
    // build has nothing that would reference these.
#if !UE_BUILD_SHIPPING
    bool _ConvergenceDebugTimingArmed = false;         // this load turned ck.Scheduler.DebugTiming on
    bool _ConvergenceDebugTimingPrior = false;          // ...and this is what it was before, restored on exit
#endif
    TSet<FName> _ConvergencePendingLastFrame;          // to spot the frame a row flips Pending -> Satisfied
    TArray<int32> _ConvergencePumpSeries;              // trailing window of per-frame pump counts
    TArray<int32> _ConvergenceSkippedSeries;           // trailing window of per-frame skipped-tick-group counts
    static constexpr int32 kConvergenceSeriesWindow = 16;
    int32 _RebuildLastMappedCount = 0;                 // progress tracking: mapped count at the previous rebuild tick
    int32 _RebuildStallTicks = 0;                      // consecutive rebuild ticks with no NEW mapping (early-exit gate)
    bool  _RebuildEscalated = false;                   // kernel quiesced with unresolved rows -> full-scope ticks (see Rebuilding)

    TWeakObjectPtr<UWorld> _PreTravelWorld;  // captured before OpenLevel; AwaitingWorld waits for a different world
    FString _TravelMapName;                  // resolved from the pre-travel world (RemovePIEPrefix)

    // The world the game-time freeze is applied to RIGHT NOW (unset == none), and that world's dilation before
    // it. Per world rather than per load, because the freeze does not survive travel — see DoApply_TimeFreeze.
    TWeakObjectPtr<UWorld> _TimeFreezeWorld;
    float _PriorTimeDilation = 1.0f;

    // One Warning per load, never one per frame — see DoObserve_PauseUnderHold. Cleared when a load latches and
    // when a client hold arms, so a pause in a later load is reported again rather than absorbed by the first.
    bool _PauseObservedUnderHold = false;

    // How many loads this GameInstance has run. The LOW half of the epoch below; nothing outside this class
    // reads it.
    int32 _LoadCount = 0;

    // The HIGH half, drawn once per GameInstance. Without it the epoch is a bare per-instance COUNT, so a
    // machine that has hosted N loads and then joins a host running its own load N reads that load as its own
    // (`Epoch == _LoadEpoch`) and refuses to arm its client hold — the one decision the epoch exists to make.
    // Salted, equality of epochs implies identity of loads, which is the only claim any consumer makes on it.
    int32 _LoadEpochSalt = 0;

    // WHICH load this is. It rides the travel URL and the replicated fact, and it is what stops a client
    // releasing on a fact left standing by the previous load — or arming for one that belongs to somebody else.
    // OPAQUE: monotonic within a session, but not a count, and never persisted.
    int32 _LoadEpoch = 0;
    bool _IsReadyToResume = false;

    // The loading screen this load holds up for its whole duration, and the channel carrying the ready-to-resume
    // fact to clients. Both are per load and both are released on every route out of one.
    UPROPERTY(Transient)
    TObjectPtr<UCk_LoadingProcess_Task_UE> _LoadScreenHold;

    FCk_Handle_PendingActorRelay _PendingLoadStateChannel;
    FCk_Handle _LoadStateChannelEntity;

    // ---- The client half: no load of its own, the same hold ----
    bool _ClientHoldActive = false;
    int32 _ClientHoldEpoch = 0;
    int32 _ClientHoldFrameCount = 0;
    FTSTicker::FDelegateHandle _ClientHoldTickerHandle;
    TWeakObjectPtr<UWorld> _ClientHoldWorld;

    UPROPERTY(Transient)
    TObjectPtr<UCk_LoadingProcess_Task_UE> _ClientLoadScreenHold;


    static constexpr int32 kLoad_TeardownFrameCap = 600; // ~10s @ 60fps; abort guard for a stuck/non-ticking world
    static constexpr int32 kLoad_TravelFrameCap   = 600; // abort if the post-travel world never comes up
    static constexpr int32 kLoad_RebuildFrameCap  = 600; // hard cap if an entry never resolves/spawns
    static constexpr int32 kLoad_RebuildStallTicks = 30; // proceed early once no NEW entity maps for this many ticks
    static constexpr int32 kLoad_HydrateFrameCap  = 600; // abort if the hydration dispatcher never drains
    static constexpr int32 kLoad_SettleFrames     = 3;   // Draining floor: parked reconcile-destroys + freed Setups drain

    // Convergence is bounded like every other phase, and for the same reason: fail-closed without an escape is a
    // permanent wedge. 180 frames is generous against a phase whose work is a handful of physics steps and a pump.
    static constexpr int32 kLoad_ConvergenceFrameCap = 180;
    // Well past a healthy convergence (measured: 2-3 frames) and well short of the cap, so a stuck phase pays for
    // per-processor timing only when it is already going to cost 180 frames anyway.
    static constexpr int32 kLoad_ConvergenceDebugArmFrame = 30;
    // Consecutive frames every registered fact must report converged before the world is handed back. One frame
    // can be a fact that has not started rather than one that has finished.
    static constexpr int32 kLoad_ConvergenceQuiescentFrames = 2;

    // The client waits on the SERVER's whole load, so its budget is the sum of what the server may legitimately
    // spend — not one phase of it. At 600 it was sized like a single phase (it was equal to the hydrate cap),
    // which means a server that spent even its teardown and travel budgets had already outlived the client
    // watching it: the client failed open into a world still being rebuilt and reported the fact as NEVER
    // ARRIVED, which is a healthy slow load misreported as a broken one. Written as the SUM rather than as a
    // number so a phase whose cap moves carries this with it instead of silently re-opening the gap.
    //
    // Rebuild counts TWICE, and that is not padding: an escalation re-arms the phase against a fresh budget
    // (DoTick_Load's Rebuilding case sets _RebuildEscalated, zeroes _LoadFrameCount and STAYS in Rebuilding), and
    // escalating is a first-class reported outcome rather than a fault — it exists because a staged construction
    // can only be finished by a game processor. So the load shape most likely to be slow is exactly the one a
    // single-rebuild sum would have failed open on. The two error directions are not symmetric: failing open
    // early hands the player a half-rebuilt world AND misreports a correct load, while failing open later costs
    // seconds of a loading screen on a load that was already broken.
    static constexpr int32 kLoad_ClientHoldFrameCap =
        kLoad_TeardownFrameCap + kLoad_TravelFrameCap +
        (2 * kLoad_RebuildFrameCap) +
        kLoad_HydrateFrameCap + kLoad_ConvergenceFrameCap;

    // The epoch's two halves. 16 low bits of count, 15 high bits of salt, sign bit left clear so the value
    // survives FCString::Atoi and the `Epoch > 0` validation on the way back in. The count wraps at 65536 loads
    // in one session — a bound no session reaches, and in any case a collision WITHIN one instance, where the
    // salt is identical and the count was never the discriminator. The salt is what makes two INSTANCES
    // distinguishable, which is the case that actually bit.
    //
    // ACCEPTED RESIDUAL: 15 bits means two GameInstances draw the same salt with p = 1/32768, and when they are
    // also at the same load count the pre-salt bug returns verbatim for that session. That is ~4 orders of
    // magnitude better than the bare count it replaces, and only the sign bit is structurally load-bearing, so
    // more salt bits are available cheaply if the residual ever stops being acceptable. Not widened here.
    static constexpr int32 kLoadEpoch_CountBits = 16;
    static constexpr int32 kLoadEpoch_CountMask = (1 << kLoadEpoch_CountBits) - 1;
    static constexpr int32 kLoadEpoch_SaltMask  = 0x7FFF;
};

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Signals.h" // FCk_Delegate_Snapshot_OnLoadComplete

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkSnapshot_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UTexture2D;

// --------------------------------------------------------------------------------------------------------------------

// Blueprint/AngelScript-facing queries over the save/load lifecycle markers. The markers themselves live in CkEcs
// (CkSnapshot_RestoreMarker.h) / CkEcsExt (CkActorRebind_Utils.h) as plain C++ ECS tags so restore-redrive
// processors can read them without a CkSnapshot dependency — but plain tags have no reflected surface, so script
// code cannot see them at all without this library. The marker queries are read-only: their lifecycles are owned
// by the load, and game-side consumers that need a clear-side contract (e.g. a rebound-handled guard) own that
// policy themselves. The SaveKey surface is the one mutator here — see its contract below.
UCLASS(NotBlueprintable)
class CKSNAPSHOT_API UCk_Utils_Snapshot_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Snapshot_UE);

    // True while the entity carries the just-restored marker — stamped by a v3 load only on entities it mapped
    // from a saved id. This is the restored-vs-replaced discriminator: presence/count checks cannot tell a
    // restored entity from a fresh construction standing where it used to be; this can.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Was Just Restored")
    static bool
    Get_WasJustRestored(
        const FCk_Handle& InHandle);

    // True while the entity carries FTag_ActorJustRebound — stamped by the respawn pass when a fresh bridged
    // actor is re-bound to this restored entity. The re-bridge skips WithActor::Construct, so actor-side wiring
    // is dead until a consumer reattaches; consumers own their done-guard.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Was Actor Just Rebound")
    static bool
    Get_WasActorJustRebound(
        const FCk_Handle& InHandle);

    // True while a CkSnapshot load is reconstituting the world the entity lives in. WORLD-scoped, unlike the
    // per-entity markers above — the handle is only used to resolve the world. The load gate freezes feature
    // processors, but entity-script construction replays during rebuild BY DESIGN — so a construction script
    // that unconditionally SEEDS a separately-persisted entity must gate that seeding on this returning false,
    // or it creates a second copy alongside the restored one. (Children composed UNDER the script are fine:
    // the replayed construction re-creating them is exactly how the rebuild works.)
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Load In Progress")
    static bool
    Get_IsLoadInProgress(
        const FCk_Handle& InHandle);

    // Runs the delegate once the world is COHERENT: immediately when no load is in progress, else one-shot on
    // this load's OnLoadComplete (post-settle — hydrated values are readable). The consumer-side twin of the
    // load gate: feature processors are frozen during a load, but signal/promise CALLBACKS are not — a callback
    // delivered mid-load that reads world population (occupancy rosters, tag scans, counts) must route that
    // read through this instead of acting on the half-rebuilt world. The immediate path passes a
    // default-constructed report (there was no load to report on).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Promise On Load Complete",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Promise_OnLoadComplete(
        const FCk_Handle& InAnyWorldHandle,
        const FCk_Delegate_Snapshot_OnLoadComplete& InDelegate);

    /**
     * Give the entity a stable save identity, derived deterministically from InStableIdentity.
     *
     * A keyed entity is captured under rule 2 (`EngineOwned`): the loader does NOT rebuild it, it RENDEZVOUSES —
     * the saved entity's payloads land on whichever entity in the fresh world carries the same key. That is the
     * correct shape for anything the fresh world creates on its own (boot-spawned singletons, level-owned infra,
     * on-demand shared infrastructure): without a key, the saved copy rebuilds ALONGSIDE the freshly-created one
     * and the world ends up with two.
     *
     * The caller owns identity STABILITY: the same logical entity must produce byte-identical InStableIdentity on
     * every boot, and two distinct logical entities must never produce the same one (they would consolidate onto
     * whichever the resolver published last). Derive it from something structural — a group tag, a level-unique
     * name, a configured id — never from spawn order, pointer values, or a runtime counter.
     *
     * Assignment is immediate (a fragment add, not a queued request) and idempotent — re-stamping the same
     * identity leaves the same key. Keys stamped long after BeginPlay are fine: the load re-sweeps the resolver
     * every rebuild tick.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Assign Save Key")
    static void
    Request_AssignSaveKey(
        UPARAM(ref) FCk_Handle& InHandle,
        const FString& InStableIdentity);

    /**
     * Registers a historical SaveKey identity for load rendezvous while keeping
     * the canonical key written by the next save unchanged. The entity must
     * already carry its canonical SaveKey.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Assign Save Key Alias")
    static void
    Request_AssignSaveKeyAlias(
        UPARAM(ref) FCk_Handle& InHandle,
        const FString& InHistoricalIdentity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Save Key")
    static bool
    Get_HasSaveKey(
        const FCk_Handle& InHandle);

    /**
     * Decode a slot's thumbnail into a transient texture, or null when the slot recorded none.
     * The texture is NOT rooted — store it in a UPROPERTY (a widget field) or the next GC collects
     * it. Decoding is not free; decode once per slot row, not per paint.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Slot Screenshot Texture")
    static UTexture2D*
    Get_SlotScreenshotTexture(
        const FCk_Snapshot_SlotMeta& InMeta);

    /**
     * True when the slot has a sidecar on disk, as opposed to the default Get_SaveSlotMeta returns
     * for a slot without one. Script-facing twin of FCk_Snapshot_SlotMeta::Get_IsPopulated — a
     * plain USTRUCT method has no reflected surface, so only CK_PROPERTY accessors reach script.
     */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Slot Meta Populated")
    static bool
    Get_IsSlotMetaPopulated(
        const FCk_Snapshot_SlotMeta& InMeta);

    /** One game-defined summary field off a slot, or InFallback when the slot never recorded it. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Slot Custom Field")
    static FString
    Get_SlotCustomField(
        const FCk_Snapshot_SlotMeta& InMeta,
        FName InKey,
        const FString& InFallback);

    /**
     * PNG thumbnail of the current viewport, for handing to Request_Save_WithMetadata.
     *
     * Call it BEFORE pushing a save/pause menu: the read takes the back buffer as-is, menus
     * included, so capturing during the save itself thumbnails the save screen. Empty (not an
     * error) with no viewport — headless, -nullrhi and dedicated servers all take that path.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Capture Viewport Thumbnail",
              meta = (WorldContext = "InWorldContextObject"))
    static TArray<uint8>
    Capture_ViewportThumbnail(
        const UObject* InWorldContextObject,
        int32 InMaxWidth = 480);
};

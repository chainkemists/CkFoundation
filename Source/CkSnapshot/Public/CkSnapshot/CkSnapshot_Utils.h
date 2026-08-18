#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkPersistenceHydration.h" // FCk_Delegate_Hydration_OnHydrated

#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Signals.h" // FCk_Delegate_Snapshot_OnLoadComplete

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkSnapshot_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UTexture2D;

// --------------------------------------------------------------------------------------------------------------------

// Carries the PNG bytes of an asynchronous viewport thumbnail capture. Empty when there was no
// viewport to read or the capture did not arrive — see Request_CaptureViewportThumbnail.
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_Snapshot_OnThumbnailCaptured,
    const TArray<uint8>&, InThumbnailPng);

// --------------------------------------------------------------------------------------------------------------------

// What Promise_OnLoadComplete DID, returned synchronously, so a caller that cares can branch without a second
// query. The delegate runs exactly once either way.
UENUM(BlueprintType)
enum class ECk_Snapshot_PromiseResult : uint8
{
    // A load is in flight; the delegate is queued and will run when it finishes.
    Bound,
    // There was no load to wait for, so the delegate has ALREADY run, with Result = NoLoadInProgress.
    NoLoadInProgress,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_PromiseResult);

// --------------------------------------------------------------------------------------------------------------------

// Blueprint/AngelScript-facing queries over the save/load lifecycle markers. The markers themselves live in CkEcs
// (CkSnapshot_RestoreMarker.h) as plain C++ ECS tags so restore-redrive processors can read them without a
// CkSnapshot dependency — but plain tags have no reflected surface, so script code cannot see them at all without
// this library. The marker queries are read-only: their lifecycles are owned by the load. The SaveKey surface is
// the one mutator here — see its contract below.
UCLASS(NotBlueprintable)
class CKSNAPSHOT_API UCk_Utils_Snapshot_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Snapshot_UE);

    // True once the entity has been hydrated by a load this session (ck::FTag_Hydration_WasHydratedThisLoad,
    // stamped only on entities the load mapped from a saved id). Genuinely useful as the restored-vs-replaced
    // discriminator — presence and count checks cannot tell a restored entity from a fresh construction standing
    // where it used to be, and this can.
    //
    // But it is PERMANENT: it never clears, so it answers "was this ever restored", never "is my restored state
    // ready". Reading it to decide when a value may be read means polling from a processor that happens to tick
    // in the right window, and pairing it with a hand-rolled once-per-feature dedup. Promise_OnHydrated is that
    // question asked properly — one-shot, per entity, pushed. This stays until its callers are swept.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Was Just Restored")
    static bool
    Get_WasJustRestored(
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

    // True while a load is REBUILDING the world the entity lives in — the load gate is held, whether the world is
    // ticking the load kernel or an escalated full-scope pass. It exists for exactly one caller: something whose
    // construction SEEDS or SPAWNS a separately-persisted entity, which must not do that while the loader is the
    // sole legitimate creator of world population, or the world ends up with the loader's copy and its own.
    //
    // It is NOT the predicate for deciding whether a value may be read. Nothing needs that predicate any more: a
    // load holds a restored entity out of every non-kernel processor's view until its payloads have applied, so a
    // Setup processor reading a Durable fragment is already in the clear, and a consumer outside that path binds
    // Promise_OnHydrated. Code that polls this to time a READ is describing a race that no longer exists, and is
    // usually one edit away from having polled it in the wrong window instead.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Is Rebuild In Progress")
    static bool
    Get_IsRebuildInProgress(
        const FCk_Handle& InHandle);

    // Runs the delegate once the world is COHERENT: immediately when no load is in progress, else one-shot on
    // this load's OnLoadComplete (post-settle — hydrated values are readable). The consumer-side twin of the
    // load gate: feature processors are frozen during a load, but signal/promise CALLBACKS are not — a callback
    // delivered mid-load that reads world population (occupancy rosters, tag scans, counts) must route that
    // read through this instead of acting on the half-rebuilt world.
    //
    // The immediate path reports Result = NoLoadInProgress, and the call RETURNS that too, so a caller that
    // cares can branch without asking a second question. It is not a failure: nothing was loading.
    //
    // The promise is held by the snapshot subsystem, which is GameInstance-scoped, so it SURVIVES the level
    // travel a load performs — a bind made before the travel still fires afterwards. That is the whole reason
    // it is not an entity signal: every ECS registry is world-scoped, so a bind on the pre-travel world's
    // transient entity died with that world and nothing said so.
    //
    // For anything scoped to ONE entity, prefer Promise_OnHydrated below: this answers the strictly later and
    // coarser question of whether the whole world is coherent, and it is meant for consumers whose lifetime is
    // DECOUPLED from the entities they read — widgets, subsystems, cross-entity tasks.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Promise On Load Complete",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static ECk_Snapshot_PromiseResult
    Promise_OnLoadComplete(
        const FCk_Handle& InAnyWorldHandle,
        const FCk_Delegate_Snapshot_OnLoadComplete& InDelegate);

    /**
     * Runs the delegate once THIS entity's restored state is final, exactly once, and always.
     *
     * The load-path twin of Promise_OnReplicationComplete, and the answer to "where do I read a restored value?"
     * — construction is too early by contract (DoConstruct and DoBeginPlay observe construct defaults), and a
     * feature's own Setup processor, which is the other correct place, is only available to a feature that HAS a
     * Setup processor. This is the hook for everyone else: a widget, a subsystem, a consumer on another entity.
     *
     * It fires on the global quarantine lift — after every mapped entity's payloads have applied, not just this
     * one's — so a callback may read its own entity AND its siblings. If nothing is pending for this handle
     * (a fresh spawn, a client, no load in flight, or a load whose lift already happened) it fires IMMEDIATELY
     * and synchronously, because hydration is then as complete as it will ever be. A promise that stayed silent
     * on an un-restored entity would push every consumer back into "poll a marker and hope", which is the
     * failure mode this replaces.
     *
     * It fires for a forced-release entity too — with that entity's loss already recorded in the load report —
     * so a consumer is never left waiting on an edge that will not arrive.
     *
     * Prefer this over Promise_OnLoadComplete for anything scoped to ONE entity: OnLoadComplete answers "is the
     * world coherent", which is a strictly later and coarser question.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Promise On Hydrated",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Promise_OnHydrated(
        const FCk_Handle& InHandle,
        const FCk_Delegate_Hydration_OnHydrated& InDelegate);

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
     * PNG thumbnail of the current viewport, delivered on the NEXT rendered frame — the capture that
     * works in a packaged build. Hand the bytes to Request_Save_WithMetadata when they arrive.
     *
     * Request it as a save/pause menu OPENS rather than when Save is clicked: the underlying read
     * happens before Slate composites UMG, so the frame it captures is the gameplay behind the menu.
     *
     * InDelegate always runs exactly once — with the bytes, or with an empty array when there is no
     * viewport (headless / -nullrhi / dedicated server) or the capture did not arrive in time.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Capture Viewport Thumbnail",
              meta = (WorldContext = "InWorldContextObject", AutoCreateRefTerm = "InDelegate"))
    static void
    Request_CaptureViewportThumbnail(
        const UObject* InWorldContextObject,
        const FCk_Delegate_Snapshot_OnThumbnailCaptured& InDelegate,
        int32 InMaxWidth = 480);
};

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkSnapshot_Utils.generated.h"

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

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Save Key")
    static bool
    Get_HasSaveKey(
        const FCk_Handle& InHandle);
};

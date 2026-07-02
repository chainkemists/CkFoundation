#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkActorRebind_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Stamped by Request_RebindActor on the restored entity the moment its fresh actor is re-bridged (M2b2a).
    // The re-bridge deliberately does NOT re-run WithActor::Construct, so any ACTOR-SIDE wiring the original
    // construction did (cached entity handles on the actor, NewObject components, camera directors, ...) is dead on
    // the respawned actor. Game code keys a processor on this tag to run its own idempotent reattach against the
    // re-bridged actor (resolve via UCk_Utils_OwningActor_UE), then REMOVES the tag as its done-guard.
    // TRANSIENT: one-shot post-restore bookkeeping — must never be captured into a save.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_ActorJustRebound);
}

// --------------------------------------------------------------------------------------------------------------------

// Re-establishes the actor<->entity bridge that a snapshot restore severs: Run_Restore's clear() drops the
// non-snapshotable OwningActor + Transform-root links, so a restored bridged entity comes back with no live actor.
// After the snapshot respawn pass spawns a fresh actor (of FFragment_ActorSpawnIntent's class), this re-links the
// two WITHOUT re-running WithActor::Construct (the gameplay fragments already round-tripped).
UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_ActorRebind_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorRebind_UE);

public:
    // Re-bridge InActor to the already-restored InEntity. Re-adds the actor->entity reverse-lookup component
    // (SetupActorEntityLink), the entity->actor OwningActor fragment (OwningActor::Add), and re-creates the
    // Transform fragment bound to the actor's root component (it is non-snapshotable, so absent post-restore).
    // Does NOT re-add gameplay fragments (those round-tripped via the snapshot).
    static auto
    Request_RebindActor(
        FCk_Handle& InEntity,
        AActor* InActor) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

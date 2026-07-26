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
    // Stamped by Request_RebindActor. The re-bridge deliberately does NOT re-run WithActor::Construct, so any
    // ACTOR-SIDE wiring the original construction did is dead on the respawned actor: game code keys a processor
    // on this tag to run its own idempotent reattach, then REMOVES the tag as its done-guard.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_ActorJustRebound);
}

// --------------------------------------------------------------------------------------------------------------------

// Re-establishes the actor<->entity bridge that a snapshot restore severs, WITHOUT re-running
// WithActor::Construct. See CkEcsExt/CLAUDE.md § "Actor rebind after a snapshot restore".
UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_ActorRebind_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorRebind_UE);

public:
    // Re-bridge InActor to the already-restored InEntity. Does NOT re-add gameplay fragments (those
    // round-tripped via the snapshot).
    static auto
    Request_RebindActor(
        FCk_Handle& InEntity,
        AActor* InActor) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

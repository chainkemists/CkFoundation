#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkActorRebind_Utils.generated.h"

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

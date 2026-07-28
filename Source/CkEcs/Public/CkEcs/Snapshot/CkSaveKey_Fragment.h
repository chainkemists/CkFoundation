#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Misc/Guid.h>

#include "CkSaveKey_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FFragment_SaveKey
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_SaveKey);

private:
    // UPROPERTY(SaveGame) sets the real CPF_SaveGame flag so Tier-A reflection serializes _Key under an
    // ArIsSaveGame archive. The former UPROPERTY(meta=(SaveGame)) only stuffed "SaveGame" into the metadata
    // map (not the flag), so the GUID round-tripped empty — see CkEcsExt's FFragment_ActorSpawnIntent.
    UPROPERTY(SaveGame)
    FGuid _Key;

public:
    CK_PROPERTY_GET(_Key);
    CK_DEFINE_CONSTRUCTORS(FFragment_SaveKey, _Key);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::save_key
{
    /**
     * The one derivation of a SaveKey from a stable identity string — every stamping site routes through here, so no
     * two of them can disagree on the hash. `UCk_Utils_Snapshot_UE::Request_AssignSaveKey` is the BP/AS entry point
     * and forwards here; this lives in CkEcs so the modules BELOW the save system (the actor bridge, the level
     * spawner, the relay channels) can key the entities they create without depending on it.
     *
     * The caller owns identity STABILITY: the same logical entity must produce a byte-identical identity on every
     * boot, and two distinct logical entities must never produce the same one — they would consolidate onto
     * whichever the load's resolver published last.
     */
    CKECS_API auto
    Assign(
        FCk_Handle& InEntity,
        const FString& InStableIdentity) -> void;

    /**
     * True when the actor came from LOADING (or PIE-duplicating) its level rather than from a runtime SpawnActor.
     * Such an actor is re-created by the level on every boot, so a save must RENDEZVOUS its state onto that fresh
     * copy; respawning it from the save would leave the world with two.
     */
    CKECS_API auto
    Get_IsLevelPlaced(
        const AActor* InActor) -> bool;

    /**
     * Rendezvous identity of a level-placed actor: its level's package name (PIE prefix stripped) followed by the
     * actor's object name. Both halves are authored data that survives a reboot, and the object name is unique
     * within its level, so the pair is stable and collision-free without being tied to spawn order.
     *
     * EMPTY when the actor is not level-placed — the caller's signal that no rendezvous identity exists for it.
     */
    CKECS_API auto
    Get_LevelPlacedIdentity(
        const AActor* InActor) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------

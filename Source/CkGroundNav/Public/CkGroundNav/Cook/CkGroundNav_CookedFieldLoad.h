#pragma once

#include "CkGroundNav/Cook/CkGroundNav_CookedTile.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// A cooked field back into a field, and where to find the index that names one.
//
// The load is a PURE function of an index asset, the params the field is being loaded for, and the
// fingerprint those params currently reduce to. It reaches no world, no registry and no physics, so
// every one of its refusals is assertable headless against assets built in a transient package - which
// is what the refusals are for. Finding the index is the one half that needs a world, and it is a
// separate call for that reason.
//
// Nothing here ensures. A cook older than the code reading it, a level nobody cooked, an asset whose
// tiles have been deleted: all of them are ordinary states of a shipped game whose answer is to bake at
// runtime, and an ensure would turn the ordinary case into a development-build crash while telling a
// shipping build nothing.
// --------------------------------------------------------------------------------------------------------------------

class UCk_GroundNav_CookedFieldIndex_UE;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * The lattice a field of these params belongs to, as the values a cooked asset carries.
     *
     * The ONE place the two representations are put side by side. A cooked tile is loadable on its own
     * and carries tile-local indices and nothing else, so comparing lattices is what turns a tile read
     * into the wrong field from a corrupt field into a refusal - and a second derivation of the key
     * would be a second answer to which lattice the params name.
     */
    CKGROUNDNAV_API auto
    Get_CookedLatticeKey(
        const FCk_GroundNav_FieldParams& InParams) -> FCk_GroundNav_CookedLatticeKey;

    /**
     * The package key a cook filed this world's fields under, or None for a world that has no level.
     *
     * The ONE derivation of it, so the lookup that finds an index and the check that judges the index
     * it found cannot arrive at two different answers. It goes through Get_PackageLookupKey, because
     * PIE renames every level package and the cook only ever ran on the unprefixed one.
     */
    CKGROUNDNAV_API auto
    Get_LevelPackageKey(
        UWorld* InWorld) -> FName;

    /**
     * The index asset for {the world's persistent level package, InCookKey}, or null.
     *
     * NULL IS THE ORDINARY ANSWER: a level opts into cooked ground, and one that never cooked simply
     * has nothing at the convention path. A None key is not a key at all - such a volume is
     * runtime-only by definition - and answers null without a lookup.
     */
    CKGROUNDNAV_API auto
    Find_CookedFieldIndex(
        UWorld* InWorld,
        FName   InCookKey) -> const UCk_GroundNav_CookedFieldIndex_UE*;

    /**
     * The cooked field the index names, composed into OutField.
     *
     * Answers Cooked on success and StaleCook on every refusal - an index whose recorded level package
     * or cook key is not the one being asked for, an index speaking a format version this reader does
     * not, a fingerprint naming inputs that have since moved, a lattice that is not the one these
     * params describe, a tile reference that resolves to nothing, a tile asset whose own format,
     * lattice, coord or fingerprint disagrees with the index that lists it, or a tile blob the
     * serializer will not read. StaleCook rather than a per-cause vocabulary because the caller's
     * answer is the same for all of them: bake at runtime.
     *
     * InLevelPackage and InCookKey are the identity the caller ASKED FOR, and they are checked against
     * the index's own: the path is the only reference a cooked asset has, so an asset that ended up at
     * the convention path while describing another level or another volume would otherwise be read as
     * this volume's ground.
     *
     * OutField IS NOT TOUCHED UNLESS THE LOAD SUCCEEDS, on the serializer's own terms - a caller
     * falling back needs something to fall back TO. The field is composed ONCE, after every tile has
     * been read, through the serializer's own composition, so every derived array is re-derived from
     * the tiles that actually loaded rather than read out of a blob.
     *
     * _OpenBodies COMES BACK EMPTY, always. The per-tile form carries none - the closure diagnostics
     * belong to the run that read the meshes, and the cook is the only thing that ever did - so a
     * cooked field reports no open body rather than a stale list of the ones the cooker found.
     *
     * MissingCook is not among the answers: an index that does not exist cannot be passed here, and
     * Find_CookedFieldIndex answering null is what that status names.
     */
    CKGROUNDNAV_API auto
    Try_LoadCookedField(
        const UCk_GroundNav_CookedFieldIndex_UE& InIndex,
        FName                                    InLevelPackage,
        FName                                    InCookKey,
        const FCk_GroundNav_FieldParams&         InParams,
        uint64                                   InInputFingerprint,
        FCk_GroundNav_Field&                     OutField) -> ECk_GroundNav_CookStatus;
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * A content fingerprint of everything one bake's output depends on. Equal fingerprints mean a rebuild
     * would reproduce the same field, so the previous result may be reused; any difference forces a bake.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_ContentFingerprint
    {
    public:
        uint64 _Value = 0;

    public:
        auto operator==(const FCk_GroundNav_ContentFingerprint&) const -> bool = default;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * THE FROZEN HASH INPUT ENUMERATION.
     *
     * Every input below changes the baked field, and therefore must change the fingerprint. Anything a
     * bake reads that is NOT on this list is a latent staleness bug: the field would silently keep a
     * result computed under different inputs. Adding a bake input means adding it HERE and adding its
     * perturbation test in the same change - never one without the other.
     *
     *   1. Geometry      - every triangle's three world-space vertices, and their winding.
     *   2. Region        - the world bounds the bake covers.
     *   3. Bake config   - cell size, cell height, tile size, max columns per tile.
     *   4. Agent profile - max slope, max slope change, step height, ledge sensitivity, rough perch
     *                      tolerance.
     *   5. Markup       - every ENABLED record's id, shape type and dimensions, world transform, area
     *                      tag, kind and cost multiplier. A disabled record decides nothing about the
     *                      field, so it must not force a rebuild by being present.
     *   6. Links        - every ENABLED record's id, both endpoints, direction, the two cost
     *                      multipliers, clearance, area and user-type tags, and the projection mode and
     *                      extents its endpoints resolve under. A disabled link decides nothing either,
     *                      by the same rule as a disabled markup record.
     *
     * ORDER INDEPENDENCE is a contract, not an accident. Triangles are combined with a commutative
     * operation, so the same world submitted in a different order fingerprints identically - otherwise
     * every unrelated change to collection order would force a full rebake of an unchanged world.
     * Addition rather than XOR, deliberately: under XOR a pair of identical triangles would cancel to
     * zero, and a doubled surface is not the same world as no surface at all.
     *
     * Markup reaches the same property by a different route: the records are hashed SEQUENTIALLY in
     * ascending _Id, which is a canonical order the submitter cannot perturb. Sequential and not
     * commutative because two records differing only in which volume carries which tag are two
     * different worlds, and a commutative combine would call them one.
     *
     * Links are hashed sequentially in the order the list carries them, which is already ascending _Id:
     * ids are handed out monotonically and never reused, so the list a submitter can hand over IS the
     * canonical order and there is nothing left to sort. Neither record carries its submission epoch
     * into the hash - a stamp saying WHEN an input arrived is not part of what the input decides, and
     * folding one in would fingerprint two identical worlds differently.
     *
     * The area tag is hashed through its NAME, never through GetTypeHash: an FName's hash is an index
     * into a per-process table and is not the same number in the next run, which is precisely the
     * property a fingerprint compared across sessions cannot have.
     */
    CKGROUNDNAV_API auto
    Get_ContentFingerprint(
        const FCk_GroundNav_GeometryBatch&          InGeometry,
        const FBox&                                 InRegion,
        const FCk_GroundNav_BakeConfig&             InConfig,
        const FCk_GroundNav_AgentProfile&           InProfile,
        TConstArrayView<FCk_GroundNav_MarkupRecord> InMarkups = {},
        TConstArrayView<FCk_GroundNav_LinkRecord>   InLinks = {}) -> FCk_GroundNav_ContentFingerprint;
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"

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
     *
     * ORDER INDEPENDENCE is a contract, not an accident. Triangles are combined with a commutative
     * operation, so the same world submitted in a different order fingerprints identically - otherwise
     * every unrelated change to collection order would force a full rebake of an unchanged world.
     * Addition rather than XOR, deliberately: under XOR a pair of identical triangles would cancel to
     * zero, and a doubled surface is not the same world as no surface at all.
     */
    CKGROUNDNAV_API auto
    Get_ContentFingerprint(
        const FCk_GroundNav_GeometryBatch& InGeometry,
        const FBox&                        InRegion,
        const FCk_GroundNav_BakeConfig&    InConfig,
        const FCk_GroundNav_AgentProfile&  InProfile) -> FCk_GroundNav_ContentFingerprint;
}

// --------------------------------------------------------------------------------------------------------------------

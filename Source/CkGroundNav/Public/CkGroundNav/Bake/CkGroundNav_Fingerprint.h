#pragma once

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"

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
     *   7. Merge        - the plane-fit tolerance and the normal cone _MergeTunables merges cells
     *                      into plates under. Two bakes merged under different tolerances describe
     *                      the same ground with different plates, and the funnel that walks one is
     *                      not walking the other.
     *   8. Clearance cap - _MaxClearanceUu, the ceiling the clearance transform saturates at - which
     *                      also sizes the halo every tile bakes with. Move it and every cell's
     *                      clearance number moves with it, and that number is what admits an agent.
     *   9. Variants     - every profile variant's tag NAME and its profile, in authored order. A
     *                      variant is a whole second field baked out of the same geometry, so one
     *                      added, edited or dropped changes what the volume publishes even though
     *                      item 4 - the untagged default's profile - did not move.
     *
     * ITEM 1 IS THE ONLY ONE Get_InputFingerprint LEAVES OUT. Items 2 through 9 are the AUTHORED
     * inputs and are what that function answers; Get_ContentFingerprint answers all nine, and is
     * implemented by handing item 1 to the same enumeration so the two cannot drift apart.
     *
     * THE TRAILING INPUTS CARRY DEFAULTS, and a default is a VALUE like any other: an omitted markup
     * list fingerprints as no markup, an omitted merge tolerance as the tunables' own defaults, an
     * omitted clearance cap as zero - which no bake runs at. Two fingerprints are therefore comparable
     * only where both were taken over the same set of supplied inputs. A caller that bakes under a
     * value passes it.
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
        const FCk_GroundNav_GeometryBatch&                       InGeometry,
        const FBox&                                              InRegion,
        const FCk_GroundNav_BakeConfig&                          InConfig,
        const FCk_GroundNav_AgentProfile&                        InProfile,
        TConstArrayView<FCk_GroundNav_MarkupRecord>              InMarkups = {},
        TConstArrayView<FCk_GroundNav_LinkRecord>                InLinks = {},
        const FCk_GroundNav_MergeTunables&                       InMergeTunables = {},
        float                                                    InMaxClearanceUu = 0.0f,
        TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants = {})
        -> FCk_GroundNav_ContentFingerprint;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Items 2 through 9 of the frozen list: everything a bake reads that was AUTHORED, with the
     * geometry left out.
     *
     * The half a volume can answer without a physics world, and the half that changes when somebody
     * paints, links, retunes or adds a profile variant. Geometry moving is a different question with a
     * different answer - the backend's world revision - and folding the two into one number would make
     * "the records moved" indistinguishable from "the world moved".
     *
     * The variants are taken as tag NAMES beside their profiles rather than as the authored variant
     * type: that type lives with the volume, and the bake layer holds no volume concepts. The name is
     * what is hashed, for the reason every other tag here is hashed through its name.
     */
    CKGROUNDNAV_API auto
    Get_InputFingerprint(
        const FBox&                                              InRegion,
        const FCk_GroundNav_BakeConfig&                          InConfig,
        const FCk_GroundNav_AgentProfile&                        InProfile,
        TConstArrayView<FCk_GroundNav_MarkupRecord>              InMarkups = {},
        TConstArrayView<FCk_GroundNav_LinkRecord>                InLinks = {},
        const FCk_GroundNav_MergeTunables&                       InMergeTunables = {},
        float                                                    InMaxClearanceUu = 0.0f,
        TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants = {})
        -> FCk_GroundNav_ContentFingerprint;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Item 1 of the frozen list on its own: one geometry collection reduced to a single 64-bit value,
     * order-independently and winding-sensitively, with its triangle count folded in.
     *
     * Exposed because not every bake has a whole-region batch to hand over. A tiled build collects one
     * tile's halo per slice and never holds the region's triangles at once, so it cannot produce this
     * and takes the overload below instead.
     */
    CKGROUNDNAV_API auto
    Get_GeometryHash(
        const FCk_GroundNav_GeometryBatch& InGeometry) -> uint64;

    /**
     * The same fingerprint over geometry ALREADY reduced to one 64-bit value.
     *
     * The primitive: the batch-taking form above is exactly this with item 1 computed by
     * Get_GeometryHash. Every other item of the frozen list is decided here, and item 1 is delegated -
     * a caller handing over a value is asserting that equal values mean equal geometry, which is the
     * contract ICk_GroundNav_GeometryBackend::Get_WorldRevision already holds over the world a sliced
     * build reads. A caller that hands over something weaker gets a fingerprint exactly as trustworthy
     * as what it handed over.
     */
    CKGROUNDNAV_API auto
    Get_ContentFingerprint(
        uint64                                                   InGeometryHash,
        const FBox&                                              InRegion,
        const FCk_GroundNav_BakeConfig&                          InConfig,
        const FCk_GroundNav_AgentProfile&                        InProfile,
        TConstArrayView<FCk_GroundNav_MarkupRecord>              InMarkups = {},
        TConstArrayView<FCk_GroundNav_LinkRecord>                InLinks = {},
        const FCk_GroundNav_MergeTunables&                       InMergeTunables = {},
        float                                                    InMaxClearanceUu = 0.0f,
        TConstArrayView<TPair<FName, FCk_GroundNav_AgentProfile>> InVariants = {})
        -> FCk_GroundNav_ContentFingerprint;
}

// --------------------------------------------------------------------------------------------------------------------

#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_LinkTypes.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Project both ends of every authored link the field's params carry onto the field itself,
     * replacing whatever the last composition resolved them to.
     *
     * Runs AFTER the seam portals and BEFORE the reachability labels. The plate set a link's ends
     * address is only whole once the seams have composed, and a link resolved after the labels were
     * numbered would join two components the numbering had already been told were apart.
     *
     * Wholesale rather than patched, exactly like the seam portals: every index a resolution carries -
     * tile, cell, plate - is valid only against the field it was answered from, so an entry carried
     * across a publish would name plates the new numbering had given to other ground.
     *
     * AN END THAT FINDS NO GROUND IS HELD, NEVER DROPPED. The entry stays, its per-end status says
     * whether that ground is missing or merely unbaked, and _UnresolvedLinkCount counts the link. The
     * only consequence is that the link contributes no crossing and no label until a publish resolves
     * it, so an author who places a link over ground nobody has baked yet loses nothing.
     *
     * Reads the field's own published cells and nothing else: no world, no registry, no physics and
     * no geometry probe.
     */
    CKGROUNDNAV_API auto
    DoResolve_Links(
        FCk_GroundNav_Field& InOutField) -> void;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * A new field that differs from the one given only in what its links resolve to, and in whatever
     * its reachability labels say as a result.
     *
     * The published field is never touched. Its value is copied, the copy re-resolves the whole record
     * list and re-labels, and the caller swaps the pointer - the same contract a rebuild answers under,
     * and the reason a reader mid-query cannot see a half-changed world.
     *
     * SPENDS NO PROBE AND RE-BAKES NOTHING. A link is authored as two world points, and finding what
     * they stand on is a projection over cells that are already published; no span, no clearance and
     * no plate of any tile can move under it. It does read cells, so this is not the cost derive's
     * pinned zero-read claim - it is a bounded number of reads per end and no contact with the backend.
     *
     * EVERY RECORD IS RE-RESOLVED FROM THE WHOLE LIST, which is what makes the answer depend on the
     * links that EXIST rather than on which of them happened to change. A record deleted outright
     * names no tile any more, so a pass that only re-resolved the listed records could never retire
     * the connectivity the deleted one contributed.
     *
     * A TILE TAKES InEpoch WHEN A LINK WHOSE RESOLVED ENTRY CHANGED HAS AN END ON IT - added, removed,
     * switched on or off, or landed on a different plate or a different status - counting the ends the
     * entry had BEFORE the change as well as the ones it has after, because a link that moved off a
     * plate changed that plate's reachability exactly as much as the plate it moved onto. The field
     * takes InEpoch when at least one tile did. A link that resolved to what it already resolved to
     * moves nothing, so a reader diffing epochs still learns exactly which ground has news.
     *
     * THE DERIVED FIELD'S PARAMS CARRY THE RECORDS ITS ENTRIES WERE RESOLVED FROM, so a published
     * field always accounts for its own links rather than for whatever the last build baked with.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    Get_FieldWithLinks(
        const FCk_GroundNav_Field&              InField,
        const TArray<FCk_GroundNav_LinkRecord>& InLinks,
        const FCk_GroundNav_Epoch&              InEpoch)
        -> TPair<FCk_GroundNav_FieldPtr, FCk_GroundNav_BakeStageResult>;
}

// --------------------------------------------------------------------------------------------------------------------

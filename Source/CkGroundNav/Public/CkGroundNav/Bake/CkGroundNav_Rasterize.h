#pragma once

#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_GeometryBatch.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Rasterize world triangles into per-column ordered spans.
     *
     * Each triangle is clipped to every column it covers and contributes that column's Z interval, so a
     * triangle narrower than a cell still registers — point-sampling the cell centre would drop it, and
     * a dropped floor is invisible until an agent falls through it.
     *
     * Spans within InProfile's step height of one another MERGE, which is what turns the six faces of a
     * floor box into one span rather than three stacked ones. The merged span keeps the normal of its
     * HIGHEST contributing surface: that is the face an agent actually stands on.
     *
     * Degenerate and non-finite triangles are DROPPED AND COUNTED in the result's dropped-input count,
     * never rasterized. A silent drop and a legitimately empty region are indistinguishable downstream.
     *
     * A column count over InConfig's per-tile ceiling fails with LimitExceeded and publishes nothing.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoRasterizeSpans(
        const FCk_GroundNav_GeometryBatch& InGeometry,
        const FBox&                        InRegion,
        const FCk_GroundNav_BakeConfig&    InConfig,
        const FCk_GroundNav_AgentProfile&  InProfile,
        FCk_GroundNav_SpanField&           OutSpans) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------

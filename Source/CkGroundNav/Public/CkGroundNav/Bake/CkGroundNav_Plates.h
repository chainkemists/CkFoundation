#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_SpanField.h"

#include <CoreMinimal.h>

#include "CkGroundNav_Plates.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * What makes two neighbouring cells part of the same plate.
 *
 * There are exactly two knobs because there are exactly three merge criteria, and the third —
 * policy equality — is not a tolerance: cells either carry the same traversal policy or they do not.
 *
 * An over-merged plate reports a floor where there is a step, and the funnel that later walks it has
 * no way to notice. Both defaults are therefore chosen with margin toward splitting.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_MergeTunables
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNav_MergeTunables);

private:
    // How far a cell's surface may sit from the plate's plane and still join it.
    //
    // THIS MUST STAY BELOW THE SHALLOWEST STEP THE CONTENT NEEDS PRESERVED. A tolerance at or above a
    // riser height merges the treads either side of it into one plate, and the step stops existing as
    // far as everything downstream is concerned. The default is one cell height, which preserves any
    // riser taller than itself; content with shallower steps than that must lower it.
    //
    // The floor is not zero. Surface normals are stored quantized, so the plane fitted through a long
    // ramp drifts about a unit from the ramp's true plane over a few dozen cells; a tolerance under
    // roughly 1 uu fragments a ramp that is genuinely flat.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _PlaneFitToleranceUu = 10.0f;

    // How far a cell's surface normal may turn from the plate's and still join it.
    //
    // In practice this rarely binds: normals that differ also make heights diverge, so the plane-fit
    // tolerance usually rejects a merge first. What it does own is the narrow end — below about 3
    // degrees it starts fragmenting nominally-flat ground on its own, because rasterized normals of a
    // gently curved surface vary by that much between neighbouring cells. The default sits well clear
    // of that floor.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _NormalConeDegrees = 10.0f;

public:
    CK_PROPERTY_GET(_PlaneFitToleranceUu);
    CK_PROPERTY_GET(_NormalConeDegrees);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_MergeTunables, _PlaneFitToleranceUu, _NormalConeDegrees);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * One merged rectangle of cells on one layer, with the two numbers that say how honest the merge
     * was.
     *
     * Bounds are INCLUSIVE. The rectangle is the unit everything above the cell grid addresses, so
     * per-plate data replaces per-cell data for everything except height and clearance.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Plate
    {
    public:
        static constexpr int32 kNoPlate = -1;

    public:
        int32 _LayerIndex = 0;

        int32 _MinX = 0;
        int32 _MinY = 0;
        int32 _MaxX = 0;
        int32 _MaxY = 0;

        // Farthest any of this plate's cells sits from the plane fitted through it.
        float _MaxPlaneResidualUu = 0.0f;

        // Total spread between the plate's lowest and highest cell. Unlike the residual, this is not
        // bounded by the tolerance the merge ran under, so it is what exposes an over-merge: a plate
        // spanning two stair treads has a range of one riser however well a plane fits it.
        float _HeightRangeUu = 0.0f;

        // The least room any cell of the plate offers, under the field's clearance ceiling. A body no
        // wider than this fits anywhere on the plate, so a move that stays inside the rectangle needs
        // no cell stepped to be admitted. Filled when a tile is published; a plate straight out of
        // decomposition carries zero.
        float _MinClearanceUu = 0.0f;

    public:
        auto Get_Width() const -> int32 { return (_MaxX - _MinX) + 1; }
        auto Get_Depth() const -> int32 { return (_MaxY - _MinY) + 1; }
        auto Get_CellCount() const -> int32 { return Get_Width() * Get_Depth(); }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Every plate of every layer, plus the cell-to-plate map that addresses them. */
    struct CKGROUNDNAV_API FCk_GroundNav_PlateField
    {
    public:
        int32 _SizeX = 0;
        int32 _SizeY = 0;
        int32 _LayerCount = 0;

        TArray<FCk_GroundNav_Plate> _Plates;

        // _LayerCount planes of _SizeX * _SizeY, layer-major; kNoPlate where nothing is walkable.
        TArray<int32> _CellToPlate;

    public:
        auto Get_PlateIndexAt(int32 InX, int32 InY, int32 InLayer) const -> int32;

        auto Get_MaxPlaneResidualUu() const -> float;
        auto Get_MaxHeightRangeUu() const -> float;

        /** Cells covered divided by plates emitted — how much the decomposition actually bought. */
        auto Get_CollapseRatio() const -> float;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The surface a walkable cell presents on one layer. Fails when the cell has no span there. */
    CKGROUNDNAV_API auto
    Get_CellSurface(
        const FCk_GroundNav_SpanField&  InSpans,
        const FCk_GroundNav_LayerField& InLayers,
        int32                           InX,
        int32                           InY,
        int32                           InLayer,
        float&                          OutTopZ,
        FVector&                        OutNormal) -> bool;

    /**
     * Collapse each layer's walkable cells into merged rectangles.
     *
     * Greedy, seeded in row-major scan order and grown along X before Y, so the same input always
     * produces the same plates in the same order — a decomposition that varied run to run would make
     * every plate id downstream unstable.
     *
     * A region that cannot merge degenerates to one plate per cell. That is correct, just larger:
     * refusing to merge costs memory, while merging cells that should not be merged reports a floor
     * where there is a step.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoDecompose_Plates(
        const FCk_GroundNav_SpanField&     InSpans,
        const FCk_GroundNav_LayerField&    InLayers,
        const FCk_GroundNav_MergeTunables& InTunables,
        FCk_GroundNav_PlateField&          OutPlates) -> FCk_GroundNav_BakeStageResult;
}

// --------------------------------------------------------------------------------------------------------------------

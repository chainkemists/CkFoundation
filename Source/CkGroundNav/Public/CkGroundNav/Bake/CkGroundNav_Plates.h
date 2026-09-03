#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupTypes.h"
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

        // Which entry of the plate field's _AreaPolicies this plate's traversal policy is, or
        // INDEX_NONE for none. An INDEX and not a container: the plate stays a plain value addressable
        // by integer id, which is the property that lets a tile be copied, compared and serialized.
        int32 _AreaPolicyIndex = INDEX_NONE;

        // What crossing this plate costs relative to plain ground. The identity where no cost markup
        // covers it, so a consumer multiplies through it without a branch.
        float _CostMultiplier = 1.0f;

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

        // Every distinct traversal-policy container the plates name, deduplicated by equality. Interned
        // here rather than carried per plate because a policy is authored once and covers many plates,
        // and because a plate that held a container would stop being a value an integer id addresses.
        TArray<FGameplayTagContainer> _AreaPolicies;

    public:
        auto Get_PlateIndexAt(int32 InX, int32 InY, int32 InLayer) const -> int32;

        /** The container an index names, or an empty one for INDEX_NONE and for anything out of range. */
        auto Get_AreaPolicy(int32 InIndex) const -> const FGameplayTagContainer&;

        auto Get_MaxPlaneResidualUu() const -> float;
        auto Get_MaxHeightRangeUu() const -> float;

        /** Cells covered divided by plates emitted — how much the decomposition actually bought. */
        auto Get_CollapseRatio() const -> float;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The published lattice a plate's cells are read against: where cell (0,0) starts, how big a cell
     * is, and the surface height of every cell.
     *
     * A VIEW over the heights rather than a copy — the caller already holds them, and a stage that
     * copied a whole tile's surfaces to read a handful of plates would cost more than the work it does.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PlateLattice
    {
    public:
        FVector2D _OriginXY = FVector2D::ZeroVector;

        float _CellSizeUu = 0.0f;

        int32 _SizeX = 0;
        int32 _SizeY = 0;
        int32 _LayerCount = 0;

        // Layer-major, as a tile stores it.
        TConstArrayView<float> _SurfaceZ;

    public:
        auto Get_IsValid() const -> bool
        {
            return _CellSizeUu > 0.0f && _SizeX > 0 && _SizeY > 0 && _LayerCount > 0 &&
                   _SurfaceZ.Num() == (_SizeX * _SizeY * _LayerCount);
        }

        auto Get_CellMinXY(int32 InX, int32 InY) const -> FVector2D
        {
            return FVector2D{
                _OriginXY.X + (static_cast<double>(InX) * _CellSizeUu),
                _OriginXY.Y + (static_cast<double>(InY) * _CellSizeUu)};
        }

        auto Get_SurfaceZ(int32 InX, int32 InY, int32 InLayer) const -> float
        {
            return _SurfaceZ[(InLayer * _SizeX * _SizeY) + (InY * _SizeX) + InX];
        }
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
     * InCellPolicy is the THIRD merge criterion, and the only one that is not a tolerance: two cells
     * join only where their entries are EQUAL. It is layer-major over the layer field's cells, exactly
     * as _CellToPlate is, and an empty view means no cell carries a policy — under which the criterion
     * admits every merge the first two do and the decomposition is unchanged. A view of any other size
     * is InvalidInput rather than a silent partial application.
     *
     * The entries are compared and never interpreted: they are the caller's key and index nothing here.
     * What a plate carries in _AreaPolicyIndex is Stamp_PlateCostPolicies' answer, not this one — a
     * criterion that also wrote the plate's policy would make the split and the label two accounts of
     * the same thing, and two accounts drift.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    DoDecompose_Plates(
        const FCk_GroundNav_SpanField&     InSpans,
        const FCk_GroundNav_LayerField&    InLayers,
        const FCk_GroundNav_MergeTunables& InTunables,
        FCk_GroundNav_PlateField&          OutPlates,
        TConstArrayView<int32>             InCellPolicy = {}) -> FCk_GroundNav_BakeStageResult;

    /**
     * Give every plate the traversal policy and cost multiplier the Cost-kind markup over it implies.
     *
     * ANY OVERLAP WINS AND THE PLATE DOES NOT SPLIT. A markup covering one cell of a plate prices the
     * whole rectangle, because splitting would renumber plates a tile has already published — seam
     * stubs, portals and reachability labels all hold a _PlateIndex, and none of them could be told. A
     * region that genuinely needs sub-plate resolution is priced by the per-query cost table instead.
     *
     * Coverage is decided per CELL at that cell's own surface height, so a volume painted on an upper
     * storey does not price the floor sharing its column. Overlapping records union their tags and the
     * LARGEST multiplier wins; a plate no record covers keeps INDEX_NONE and the identity multiplier.
     *
     * The SOLE writer of _AreaPolicyIndex, _CostMultiplier and _AreaPolicies, and it recomputes all
     * three from the records it is given rather than adding to what is already there. That is what lets
     * the same call restamp an already-stamped field: switching a record off removes exactly its
     * contribution, where a pass that accumulated would leave it behind forever.
     *
     * Bills NO probes. It reads no span, no geometry, and no cell the plate rectangles do not already
     * name, which is the property that lets a cost-only derive answer on an empty budget.
     *
     * Pure: no world, no registry, no physics.
     */
    CKGROUNDNAV_API auto
    Stamp_PlateCostPolicies(
        const FCk_GroundNav_PlateLattice&           InLattice,
        TConstArrayView<FCk_GroundNav_MarkupRecord> InMarkups,
        FCk_GroundNav_PlateField&                   InOutPlates) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

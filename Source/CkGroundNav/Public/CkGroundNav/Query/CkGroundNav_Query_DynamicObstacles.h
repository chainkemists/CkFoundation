#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Immutable, query-local dynamic geometry. It deliberately carries values only: CkGroundNav must not
// retain Crowd entities or read a changing world while a sliced search is in flight.
// --------------------------------------------------------------------------------------------------------------------

class FCkTest_GroundNav_DynamicObstacleBroadPhase;

namespace ck::groundnav
{
    enum class ECk_GroundNav_DynamicUnionEdge : uint8;

    struct CKGROUNDNAV_API FCk_GroundNav_DynamicObstacleDisc
    {
        FVector _Centre = FVector::ZeroVector;
        float _RadiusUu = 0.0f;
        float _VerticalHalfExtentUu = 0.0f;
    };

    struct CKGROUNDNAV_API FCk_GroundNav_DynamicObstacleObb
    {
        // Canonical yaw-only, unit-scale transform. Try_MakeDynamicObstacleSnapshot rejects every
        // other transform so the predicates have one coordinate-system contract.
        FTransform _YawTransform = FTransform::Identity;
        FVector _WorldHalfExtents = FVector::ZeroVector;
    };

    class CKGROUNDNAV_API FCk_GroundNav_DynamicObstacleSnapshot final
    {
    public:
        FCk_GroundNav_DynamicObstacleSnapshot() = default;

        // Default construction is the valid empty snapshot. Non-empty snapshots are only created
        // by the validating factory, and the private arrays are never mutable through this API.
        auto Get_IsEmpty() const -> bool { return _Discs.IsEmpty() && _Obbs.IsEmpty(); }
        auto Get_Discs() const -> TConstArrayView<FCk_GroundNav_DynamicObstacleDisc> { return _Discs; }
        auto Get_Obbs() const -> TConstArrayView<FCk_GroundNav_DynamicObstacleObb> { return _Obbs; }

    private:
        friend CKGROUNDNAV_API auto Try_MakeDynamicObstacleSnapshot(
            TConstArrayView<FCk_GroundNav_DynamicObstacleDisc>,
            TConstArrayView<FCk_GroundNav_DynamicObstacleObb>)
            -> TOptional<FCk_GroundNav_DynamicObstacleSnapshot>;
        friend CKGROUNDNAV_API auto Get_IsDynamicObstacleCoveringCell(
            const FCk_GroundNav_DynamicObstacleSnapshot&, const FVector2D&, float, float) -> TOptional<bool>;
        friend CKGROUNDNAV_API auto Get_DynamicUnionEdge(
            const FCk_GroundNav_DynamicObstacleSnapshot&, const FVector&, const FVector&)
            -> TOptional<ECk_GroundNav_DynamicUnionEdge>;
        friend class ::FCkTest_GroundNav_DynamicObstacleBroadPhase;

        struct FDiscBroadPhaseNode
        {
            double _MinimumX = 0.0;
            double _MinimumY = 0.0;
            double _MinimumZ = 0.0;
            double _MaximumX = 0.0;
            double _MaximumY = 0.0;
            double _MaximumZ = 0.0;
            int32  _First = 0;
            int32  _Count = 0;
            int32  _Left = INDEX_NONE;
            int32  _Right = INDEX_NONE;
        };

        FCk_GroundNav_DynamicObstacleSnapshot(
            TArray<FCk_GroundNav_DynamicObstacleDisc>&& InDiscs,
            TArray<FCk_GroundNav_DynamicObstacleObb>&&  InObbs)
            : _Discs(MoveTemp(InDiscs))
            , _Obbs(MoveTemp(InObbs))
        {}

        TArray<FCk_GroundNav_DynamicObstacleDisc> _Discs;
        TArray<FCk_GroundNav_DynamicObstacleObb> _Obbs;
        TArray<FDiscBroadPhaseNode> _DiscBroadPhaseNodes;
        TArray<int32> _DiscBroadPhaseIndices;
        bool _HasDiscBroadPhase = false;
    };

    /** Returns unset when any supplied record is malformed; never returns a partial subset. */
    CKGROUNDNAV_API auto Try_MakeDynamicObstacleSnapshot(
        TConstArrayView<FCk_GroundNav_DynamicObstacleDisc> InDiscs,
        TConstArrayView<FCk_GroundNav_DynamicObstacleObb>  InObbs)
        -> TOptional<FCk_GroundNav_DynamicObstacleSnapshot>;

    /** The exact monotonic-union answer an inside-start cell edge needs. */
    enum class ECk_GroundNav_DynamicUnionEdge : uint8
    {
        Clear,
        InsideAll,
        ExitsOnce,
        NonMonotonic
    };

    /**
     * Whether any obstacle covers the closed cell square at this exact surface Z. Unset rejects
     * malformed query inputs rather than treating them as clear.
     */
    CKGROUNDNAV_API auto Get_IsDynamicObstacleCoveringCell(
        const FCk_GroundNav_DynamicObstacleSnapshot& InSnapshot,
        const FVector2D&                             InCellMinXY,
        float                                        InCellSizeUu,
        float                                        InSurfaceZUu) -> TOptional<bool>;

    /**
     * Classifies exact closed obstacle intervals along one segment. Unset rejects malformed query
     * inputs. Separate intervals remain separate: no epsilon bridges two distinct obstacles.
     */
    CKGROUNDNAV_API auto Get_DynamicUnionEdge(
        const FCk_GroundNav_DynamicObstacleSnapshot& InSnapshot,
        const FVector&                                InStart,
        const FVector&                                InEnd) -> TOptional<ECk_GroundNav_DynamicUnionEdge>;
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkGroundNav_Query_SurfaceWalk.h"

#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"
#include "CkGroundNav/Query/CkGroundNav_Query_CellStep.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace surfacewalk_private
    {
        // A motion component below this crosses no boundary over the whole segment, so its axis never
        // steps and its parametric distances stay out of reach.
        constexpr auto kAxisMotionEpsilon = 1e-9;

        // Motion shorter than this is nothing left to walk.
        constexpr auto kResidualMotionUu = 1e-6;

        // A cell costs the length walked inside it. The bake carries no traversal policy yet, so this is
        // where a plate's own multiplier will be read from once it does.
        constexpr auto kCostMultiplier = 1.0;

        // ------------------------------------------------------------------------------------------------------------

        /** The field-wide cell a surface sits in: its tile's corner in cells, plus its tile-local index. */
        auto Get_FieldCellOf(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_SurfaceRef& InSurface) -> FIntPoint
        {
            if (NOT InField._Tiles.IsValidIndex(InSurface._TileIndex))
            { return FIntPoint{INDEX_NONE, INDEX_NONE}; }

            const auto CellsPerTile = Get_CellsPerTile(InField._Params);
            const auto& Tile = InField._Tiles[InSurface._TileIndex];

            return FIntPoint{
                (Tile._Coord._X * CellsPerTile) + InSurface._CellX,
                (Tile._Coord._Y * CellsPerTile) + InSurface._CellY};
        }

        auto Get_EdgeNormal(
            int32 InDirection) -> FVector
        {
            const auto Offset = Get_DirectionOffset(InDirection);

            return FVector{-static_cast<double>(Offset.X), -static_cast<double>(Offset.Y), 0.0};
        }

        // ------------------------------------------------------------------------------------------------------------

        struct FPlateEarlyOut
        {
            bool _Applies = false;

            FCk_GroundNav_SurfaceRef _Surface;

            float _SurfaceZUu = 0.0f;
        };

        /**
         * Whether the target lies on the start's own plate with room for the body, and the surface it
         * lands on. One cell read, and only once the rectangle and the plate's clearance both allow it.
         */
        auto Get_PlateEarlyOut(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_SurfaceRef& InStart,
            const FVector2D&                InTargetXY,
            const FCk_GroundNav_QueryAgent& InAgent,
            FCk_GroundNav_QueryCost&        InOutCost) -> FPlateEarlyOut
        {
            auto EarlyOut = FPlateEarlyOut{};

            const auto TargetAddress = Get_CellAddress(InField, Get_FieldCellAt(InField._Params, InTargetXY));

            if (NOT TargetAddress.Get_IsValid() || TargetAddress._TileIndex != InStart._TileIndex)
            { return EarlyOut; }

            const auto& Tile = InField._Tiles[InStart._TileIndex];

            if (NOT Tile._Plates._Plates.IsValidIndex(InStart._PlateIndex))
            { return EarlyOut; }

            const auto& Plate = Tile._Plates._Plates[InStart._PlateIndex];

            const auto TargetIsOnThePlate =
                TargetAddress._CellX >= Plate._MinX && TargetAddress._CellX <= Plate._MaxX &&
                TargetAddress._CellY >= Plate._MinY && TargetAddress._CellY <= Plate._MaxY;

            if (NOT TargetIsOnThePlate || NOT Get_IsAdmitted(Plate._MinClearanceUu, InAgent))
            { return EarlyOut; }

            auto Surface = FCk_GroundNav_SurfaceRef{};
            auto SurfaceZUu = 0.0f;
            auto ClearanceUu = 0.0f;

            ++InOutCost._CellsRead;

            if (NOT Get_SurfaceAt(InField, TargetAddress, InStart._LayerIndex, Surface, SurfaceZUu, ClearanceUu))
            { return EarlyOut; }

            if (Surface._PlateIndex != InStart._PlateIndex)
            { return EarlyOut; }

            EarlyOut._Applies = true;
            EarlyOut._Surface = Surface;
            EarlyOut._SurfaceZUu = SurfaceZUu;

            return EarlyOut;
        }

        // ------------------------------------------------------------------------------------------------------------

        /**
         * The grid line traversal both queries walk: a segment in XY stepped cell by cell through
         * Get_StepAcross, stopping at the target or at the first step the ground refuses.
         *
         * The walker's point is always _Origin + _Delta * _T, which is what keeps it inside the current
         * cell's closed square: _T only ever advances to a boundary whose step across was admitted.
         */
        struct FTraversal
        {
            const FCk_GroundNav_Field& _Field;

            FCk_GroundNav_QueryAgent _Agent;
            FCk_GroundNav_QueryCost _Cost;

            FCk_GroundNav_SurfaceRef _Current;

            // Field-wide, unlike the surface's own tile-local index.
            FIntPoint _Cell = FIntPoint::ZeroValue;

            float _SurfaceZUu = 0.0f;

            FVector2D _Origin = FVector2D::ZeroVector;
            FVector2D _Position = FVector2D::ZeroVector;
            FVector2D _Target = FVector2D::ZeroVector;
            FVector2D _Delta = FVector2D::ZeroVector;

            double _TotalLengthUu = 0.0;
            double _T = 0.0;

            double _TMax[2] = {0.0, 0.0};
            double _TDelta[2] = {0.0, 0.0};
            int32 _Step[2] = {0, 0};

            bool _Blocked = false;
            int32 _BlockedAxis = INDEX_NONE;
            int32 _BlockedDirection = INDEX_NONE;

            int32 _CellsStepped = 0;
            int32 _PortalCrossings = 0;
            int32 _SeamCrossings = 0;

            auto DoInitialise(
                const FVector2D& InFrom,
                const FVector2D& InTarget) -> void
            {
                _Origin = InFrom;
                _Position = InFrom;
                _Target = InTarget;
                _Delta = InTarget - InFrom;
                _TotalLengthUu = _Delta.Size();
                _T = 0.0;

                const auto CellSizeUu = static_cast<double>(_Field._Params._Config.Get_CellSizeUu());
                const auto CellMinXY = Get_CellMinXY(_Field._Params, _Cell);

                for (auto Axis = 0; Axis < 2; ++Axis)
                {
                    const auto DeltaOnAxis = Axis == 0 ? _Delta.X : _Delta.Y;

                    if (FMath::Abs(DeltaOnAxis) < kAxisMotionEpsilon)
                    {
                        _Step[Axis] = 0;
                        _TDelta[Axis] = TNumericLimits<double>::Max();
                        _TMax[Axis] = TNumericLimits<double>::Max();

                        continue;
                    }

                    const auto PositionOnAxis = Axis == 0 ? _Position.X : _Position.Y;
                    const auto CellMinOnAxis = Axis == 0 ? CellMinXY.X : CellMinXY.Y;

                    _Step[Axis] = DeltaOnAxis > 0.0 ? 1 : -1;
                    _TDelta[Axis] = CellSizeUu / FMath::Abs(DeltaOnAxis);

                    const auto BoundaryOnAxis = _Step[Axis] > 0 ? CellMinOnAxis + CellSizeUu : CellMinOnAxis;

                    _TMax[Axis] = (BoundaryOnAxis - PositionOnAxis) / DeltaOnAxis;
                }
            }

            /**
             * Whether the target lies in the current cell's CLOSED square. Decided on the target's own
             * coordinates rather than on the parametric distance of the next boundary: a target exactly
             * on a cell line has that boundary at a distance that rounds to either side of one, and a
             * traversal that stepped across the line at its very end would answer differently from the
             * one coming the other way.
             */
            auto Get_IsTargetInCurrentCell() const -> bool
            {
                const auto CellSizeUu = static_cast<double>(_Field._Params._Config.Get_CellSizeUu());
                const auto CellMinXY = Get_CellMinXY(_Field._Params, _Cell);

                return _Target.X >= CellMinXY.X && _Target.X <= CellMinXY.X + CellSizeUu &&
                       _Target.Y >= CellMinXY.Y && _Target.Y <= CellMinXY.Y + CellSizeUu;
            }

            /** Cross the next boundary. False when the target is reached or the ground refused the step. */
            auto DoAdvance() -> bool
            {
                if (Get_IsTargetInCurrentCell())
                {
                    _T = 1.0;
                    _Position = _Target;

                    return false;
                }

                const auto Axis = _TMax[0] <= _TMax[1] ? 0 : 1;

                const auto Direction = Axis == 0
                    ? (_Step[0] > 0 ? 0 : 2)
                    : (_Step[1] > 0 ? 1 : 3);

                auto NextSurface = FCk_GroundNav_SurfaceRef{};
                auto SurfaceZUu = 0.0f;
                auto ClearanceUu = 0.0f;

                const auto Verdict = Get_StepAcross(
                    _Field, _Current, Direction, _Agent, NextSurface, SurfaceZUu, ClearanceUu, _Cost);

                _T = _TMax[Axis];
                _Position = _Origin + (_Delta * _T);

                if (Verdict != ECk_GroundNav_StepVerdict::Admitted)
                {
                    _Blocked = true;
                    _BlockedAxis = Axis;
                    _BlockedDirection = Direction;

                    return false;
                }

                const auto CrossedATile = NextSurface._TileIndex != _Current._TileIndex;
                const auto CrossedAPlate = NextSurface._PlateIndex != _Current._PlateIndex;

                _Current = NextSurface;
                _Cell += Get_DirectionOffset(Direction);
                _SurfaceZUu = SurfaceZUu;
                _TMax[Axis] += _TDelta[Axis];

                ++_CellsStepped;

                if (CrossedATile)
                {
                    ++_SeamCrossings;
                    ++_Cost._TilesTouched;
                }
                else if (CrossedAPlate)
                { ++_PortalCrossings; }

                return true;
            }

            /** The walker's point, held inside the current cell's closed square against rounding. */
            auto Get_ContainedPosition() const -> FVector2D
            {
                return Get_ClosestPointInCellXY(_Field._Params, _Cell, _Position);
            }

            /** The blocked point, moved exactly onto the edge the refused step would have crossed. */
            auto Get_SlidPosition() const -> FVector2D
            {
                const auto CellSizeUu = static_cast<double>(_Field._Params._Config.Get_CellSizeUu());
                const auto CellMinXY = Get_CellMinXY(_Field._Params, _Cell);
                const auto CellMinOnAxis = _BlockedAxis == 0 ? CellMinXY.X : CellMinXY.Y;
                const auto EdgeOnAxis = _Step[_BlockedAxis] > 0 ? CellMinOnAxis + CellSizeUu : CellMinOnAxis;

                auto Slid = _Position;

                if (_BlockedAxis == 0)
                { Slid.X = EdgeOnAxis; }
                else
                { Slid.Y = EdgeOnAxis; }

                return Get_ClosestPointInCellXY(_Field._Params, _Cell, Slid);
            }

            /** The parametric distance of the next point the traversal stops at. */
            auto Get_NextT() const -> double
            {
                return FMath::Min(FMath::Min(_TMax[0], _TMax[1]), 1.0);
            }
        };

        // ------------------------------------------------------------------------------------------------------------


        // ------------------------------------------------------------------------------------------------------------

        /**
         * The surface a traversal starts on.
         *
         * A start on a cell line belongs to the cells on both sides, and the choice between them is
         * not arbitrary: the body prefers a cell it is admitted on — it may already be standing in a
         * tight spot, which is why an unadmitted cell is still acceptable when nothing else is — and
         * between equals it prefers the cell on the side its motion enters, so a traversal never
         * begins by stepping across the very line it started on.
         */
        auto Get_TraversalStart(
            const FCk_GroundNav_Field&      InField,
            const FVector&                  InStart,
            float                           InVerticalToleranceUu,
            const FCk_GroundNav_QueryAgent& InAgent,
            const FVector2D&                InMotionXY) -> FCk_GroundNav_IsNavigableResult
        {
            auto Result = FCk_GroundNav_IsNavigableResult{};

            const auto StartXY = FVector2D{InStart.X, InStart.Y};

            auto Candidates = TArray<FCk_GroundNav_CellAddress, TInlineAllocator<4>>{};
            Get_CellAddressesAt(InField, StartXY, Candidates);

            if (Candidates.IsEmpty())
            {
                Result._Status = ECk_NavSurface_QueryStatus::NoSurface;

                return Result;
            }

            const auto FloorCell = Get_FieldCellAt(InField._Params, StartXY);
            const auto ToleranceUu = static_cast<double>(InVerticalToleranceUu);

            auto TouchedTiles = TArray<int32, TInlineAllocator<4>>{};

            auto Found = false;
            auto BestAdmitted = false;
            auto BestVerticalUu = 0.0;
            auto BestSidePreference = -1;

            for (const auto& Address : Candidates)
            {
                TouchedTiles.AddUnique(Address._TileIndex);

                if (Get_TileStatus(InField, Address._TileIndex) != ECk_GroundNav_BuildStatus::Built)
                {
                    Result._Cost._TouchedUnbuiltTile = true;
                    continue;
                }

                const auto& Tile = InField._Tiles[Address._TileIndex];
                const auto CellsPerTile = Get_CellsPerTile(InField._Params);
                const auto FieldCellX = (Tile._Coord._X * CellsPerTile) + Address._CellX;
                const auto FieldCellY = (Tile._Coord._Y * CellsPerTile) + Address._CellY;

                // Only meaningful where two candidates differ on an axis, which is exactly the on-line
                // case; everywhere else every candidate scores the same.
                const auto OnMotionSideX = InMotionXY.X > 0.0
                    ? FieldCellX == FloorCell.X
                    : InMotionXY.X < 0.0 ? FieldCellX == FloorCell.X - 1 : true;
                const auto OnMotionSideY = InMotionXY.Y > 0.0
                    ? FieldCellY == FloorCell.Y
                    : InMotionXY.Y < 0.0 ? FieldCellY == FloorCell.Y - 1 : true;
                const auto SidePreference = (OnMotionSideX ? 1 : 0) + (OnMotionSideY ? 1 : 0);

                for (auto Layer = 0; Layer < Tile._LayerCount; ++Layer)
                {
                    ++Result._Cost._CellsRead;

                    auto Surface = FCk_GroundNav_SurfaceRef{};
                    auto SurfaceZUu = 0.0f;
                    auto ClearanceUu = 0.0f;

                    if (NOT Get_SurfaceAt(InField, Address, Layer, Surface, SurfaceZUu, ClearanceUu))
                    { continue; }

                    const auto VerticalUu = FMath::Abs(static_cast<double>(SurfaceZUu) - InStart.Z);

                    if (VerticalUu > ToleranceUu)
                    { continue; }

                    const auto Admitted = Get_IsAdmitted(ClearanceUu, InAgent);

                    const auto IsBetter =
                        NOT Found ||
                        (Admitted != BestAdmitted ? Admitted :
                         VerticalUu != BestVerticalUu ? VerticalUu < BestVerticalUu :
                         SidePreference > BestSidePreference);

                    if (NOT IsBetter)
                    { continue; }

                    Found = true;
                    BestAdmitted = Admitted;
                    BestVerticalUu = VerticalUu;
                    BestSidePreference = SidePreference;

                    Result._Surface = Surface;
                    Result._SurfaceZUu = SurfaceZUu;
                    Result._ClearanceUu = ClearanceUu;
                }
            }

            Result._Cost._TilesTouched = TouchedTiles.Num();

            if (Found)
            {
                Result._Status = ECk_NavSurface_QueryStatus::Success;

                return Result;
            }

            Result._Status = Result._Cost._TouchedUnbuiltTile
                ? ECk_NavSurface_QueryStatus::Unbuilt
                : ECk_NavSurface_QueryStatus::NoSurface;

            return Result;
        }

        auto DoBegin_Traversal(
            FTraversal&                            InOutTraversal,
            const FCk_GroundNav_IsNavigableResult& InStart,
            const FCk_GroundNav_QueryAgent&        InAgent,
            const FCk_GroundNav_QueryCost&         InCostSoFar,
            const FVector2D&                       InStartXY,
            const FVector2D&                       InTargetXY) -> void
        {
            InOutTraversal._Agent = InAgent;
            InOutTraversal._Cost = InCostSoFar;
            InOutTraversal._Current = InStart._Surface;
            InOutTraversal._Cell = Get_FieldCellOf(InOutTraversal._Field, InStart._Surface);
            InOutTraversal._SurfaceZUu = InStart._SurfaceZUu;

            InOutTraversal.DoInitialise(InStartXY, InTargetXY);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MoveAlongSurface(
            const FCk_GroundNav_Field&            InField,
            const FCk_GroundNav_SurfaceWalkQuery& InQuery,
            FCk_GroundNav_SurfaceWalkDiagnostics& OutDiagnostics)
        -> FCk_GroundNav_SurfaceWalkResult
    {
        using namespace surfacewalk_private;

        OutDiagnostics = {};

        auto Result = FCk_GroundNav_SurfaceWalkResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        const auto StartXY = FVector2D{InQuery._Start.X, InQuery._Start.Y};
        const auto TargetXY = FVector2D{InQuery._Target.X, InQuery._Target.Y};

        const auto Start = Get_TraversalStart(
            InField, InQuery._Start, InQuery._StartVerticalToleranceUu, InQuery._Agent, TargetXY - StartXY);

        Result._Cost = Start._Cost;

        if (NOT Start.Get_IsSuccess())
        {
            Result._Status = Start._Status;

            return Result;
        }

        const auto EarlyOut = Get_PlateEarlyOut(InField, Start._Surface, TargetXY, InQuery._Agent, Result._Cost);

        if (EarlyOut._Applies)
        {
            Result._Status = ECk_NavSurface_QueryStatus::Success;
            Result._Location = FVector{TargetXY.X, TargetXY.Y, static_cast<double>(EarlyOut._SurfaceZUu)};
            Result._Surface = EarlyOut._Surface;
            Result._ReachedTarget = true;

            OutDiagnostics._TookPlateEarlyOut = true;

            return Result;
        }

        auto Traversal = FTraversal{InField};
        DoBegin_Traversal(Traversal, Start, InQuery._Agent, Result._Cost, StartXY, TargetXY);

        const auto CellSizeUu = static_cast<double>(InField._Params._Config.Get_CellSizeUu());
        const auto SpannedCells =
            (FMath::Abs(TargetXY.X - StartXY.X) + FMath::Abs(TargetXY.Y - StartXY.Y)) / CellSizeUu;
        const auto MaxIterations = FMath::CeilToInt32(4.0 * (2.0 + SpannedCells + 4.0)) + 16;

        auto Iterations = 0;

        while (true)
        {
            if (Iterations >= MaxIterations)
            {
                OutDiagnostics._HitIterationBound = true;

                break;
            }

            ++Iterations;

            if (Traversal.DoAdvance())
            { continue; }

            if (NOT Traversal._Blocked)
            { break; }

            ++OutDiagnostics._BlockedSteps;
            ++OutDiagnostics._SlideCount;

            const auto SlidPosition = Traversal.Get_SlidPosition();

            auto SlidTarget = Traversal._Target;

            if (Traversal._BlockedAxis == 0)
            { SlidTarget.X = SlidPosition.X; }
            else
            { SlidTarget.Y = SlidPosition.Y; }

            Traversal._Blocked = false;
            Traversal._Position = SlidPosition;

            if ((SlidTarget - SlidPosition).Size() < kResidualMotionUu)
            { break; }

            Traversal.DoInitialise(SlidPosition, SlidTarget);
        }

        const auto FinalXY = Traversal.Get_ContainedPosition();

        Result._Status = ECk_NavSurface_QueryStatus::Success;
        Result._Location = FVector{FinalXY.X, FinalXY.Y, static_cast<double>(Traversal._SurfaceZUu)};
        Result._Surface = Traversal._Current;
        Result._ReachedTarget = FVector2D::Distance(FinalXY, TargetXY) <= kResidualMotionUu;
        Result._Cost = Traversal._Cost;

        OutDiagnostics._CellsStepped = Traversal._CellsStepped;
        OutDiagnostics._PortalCrossings = Traversal._PortalCrossings;
        OutDiagnostics._SeamCrossings = Traversal._SeamCrossings;

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_SurfaceRaycast(
            const FCk_GroundNav_Field&        InField,
            const FCk_GroundNav_RaycastQuery& InQuery)
        -> FCk_GroundNav_RaycastResult
    {
        using namespace surfacewalk_private;

        auto Result = FCk_GroundNav_RaycastResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;

            return Result;
        }

        const auto StartXY = FVector2D{InQuery._Start.X, InQuery._Start.Y};
        const auto EndXY = FVector2D{InQuery._End.X, InQuery._End.Y};

        const auto Start = Get_TraversalStart(
            InField, InQuery._Start, InQuery._StartVerticalToleranceUu, InQuery._Agent, EndXY - StartXY);

        Result._Cost = Start._Cost;

        if (NOT Start.Get_IsSuccess())
        {
            Result._Status = Start._Status;

            return Result;
        }

        const auto SegmentLengthUu = FVector2D::Distance(StartXY, EndXY);
        const auto CapIsActive = InQuery._MaxCost > 0.0f;
        const auto MaxCost = static_cast<double>(InQuery._MaxCost);

        // A segment the cap would stop inside is stepped instead, so the ray ends where the accumulation
        // reached the cap rather than at the plate's far end.
        const auto CapAdmitsTheWholeSegment = NOT CapIsActive || (kCostMultiplier * SegmentLengthUu) <= MaxCost;

        if (CapAdmitsTheWholeSegment)
        {
            const auto EarlyOut = Get_PlateEarlyOut(InField, Start._Surface, EndXY, InQuery._Agent, Result._Cost);

            if (EarlyOut._Applies)
            {
                Result._Status = ECk_NavSurface_QueryStatus::Success;
                Result._HitLocation = FVector{EndXY.X, EndXY.Y, static_cast<double>(EarlyOut._SurfaceZUu)};
                Result._LastSurface = EarlyOut._Surface;
                Result._AccumulatedCost = static_cast<float>(kCostMultiplier * SegmentLengthUu);

                return Result;
            }
        }

        auto Traversal = FTraversal{InField};
        DoBegin_Traversal(Traversal, Start, InQuery._Agent, Result._Cost, StartXY, EndXY);

        auto AccumulatedCost = 0.0;

        while (true)
        {
            const auto LengthInCellUu = (Traversal.Get_NextT() - Traversal._T) * Traversal._TotalLengthUu;
            const auto SegmentCost = kCostMultiplier * LengthInCellUu;

            if (CapIsActive && (AccumulatedCost + SegmentCost) > MaxCost)
            {
                const auto RemainingLengthUu = (MaxCost - AccumulatedCost) / kCostMultiplier;
                const auto CapT = Traversal._TotalLengthUu > 0.0
                    ? Traversal._T + (RemainingLengthUu / Traversal._TotalLengthUu)
                    : Traversal._T;

                Traversal._Position = Traversal._Origin + (Traversal._Delta * CapT);

                const auto CapXY = Traversal.Get_ContainedPosition();

                Result._Status = ECk_NavSurface_QueryStatus::Blocked;
                Result._HitLocation = FVector{CapXY.X, CapXY.Y, static_cast<double>(Traversal._SurfaceZUu)};
                Result._LastSurface = Traversal._Current;
                Result._AccumulatedCost = static_cast<float>(MaxCost);
                Result._StoppedOnCost = true;
                Result._Cost = Traversal._Cost;

                return Result;
            }

            AccumulatedCost += SegmentCost;

            if (NOT Traversal.DoAdvance())
            { break; }
        }

        const auto FinalXY = Traversal.Get_ContainedPosition();

        Result._HitLocation = FVector{FinalXY.X, FinalXY.Y, static_cast<double>(Traversal._SurfaceZUu)};
        Result._LastSurface = Traversal._Current;
        Result._AccumulatedCost = static_cast<float>(AccumulatedCost);
        Result._Cost = Traversal._Cost;

        if (Traversal._Blocked)
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;
            Result._HitNormal = Get_EdgeNormal(Traversal._BlockedDirection);

            return Result;
        }

        Result._Status = ECk_NavSurface_QueryStatus::Success;

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

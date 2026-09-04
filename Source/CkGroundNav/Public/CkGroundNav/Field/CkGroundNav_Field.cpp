#include "CkGroundNav_Field.h"

#include "CkGroundNav/Bake/CkGroundNav_MeshClosure.h"
#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Field/CkGroundNav_FieldLinks.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        FCk_GroundNav_FieldParams::
        Get_IsValid() const
        -> bool
    {
        return _Config.Get_IsValid() &&
               _Divisions.X > 0 && _Divisions.Y > 0 &&
               _MaxZUu > _MinZUu &&
               _MaxClearanceUu >= 0.0f;
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_TileSpanUu() const
        -> double
    {
        const auto CellSize = static_cast<double>(_Config.Get_CellSizeUu());

        if (CellSize <= 0.0)
        { return 0.0; }

        const auto Cells = FMath::Max(1, FMath::CeilToInt32(_Config.Get_TileSizeUu() / CellSize));

        return static_cast<double>(Cells) * CellSize;
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_Bounds() const
        -> FBox
    {
        const auto SpanUu = Get_TileSpanUu();

        return FBox{
            FVector{_OriginXY.X, _OriginXY.Y, static_cast<double>(_MinZUu)},
            FVector{
                _OriginXY.X + (SpanUu * _Divisions.X),
                _OriginXY.Y + (SpanUu * _Divisions.Y),
                static_cast<double>(_MaxZUu)}};
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_TileBakeParams(
            const FCk_GroundNav_TileCoord& InCoord,
            const FCk_GroundNav_Epoch&     InEpoch) const
        -> FCk_GroundNav_TileBakeParams
    {
        auto Params = FCk_GroundNav_TileBakeParams{};

        Params._Coord = InCoord;
        Params._Epoch = InEpoch;
        Params._FieldOriginXY = _OriginXY;
        Params._MinZUu = _MinZUu;
        Params._MaxZUu = _MaxZUu;
        Params._Config = _Config;
        Params._Profile = _Profile;
        Params._MergeTunables = _MergeTunables;
        Params._MarkupRecords = _MarkupRecords;
        Params._MaxClearanceUu = _MaxClearanceUu;

        return Params;
    }

    auto
        FCk_GroundNav_FieldParams::
        Get_TileCoordAt(
            const FVector& InWorldPosition) const
        -> FCk_GroundNav_TileCoord
    {
        const auto SpanUu = Get_TileSpanUu();

        if (SpanUu <= 0.0)
        { return FCk_GroundNav_TileCoord{INDEX_NONE, INDEX_NONE}; }

        return FCk_GroundNav_TileCoord{
            FMath::FloorToInt32((InWorldPosition.X - _OriginXY.X) / SpanUu),
            FMath::FloorToInt32((InWorldPosition.Y - _OriginXY.Y) / SpanUu)};
    }

    auto
        FCk_GroundNav_SeamPortal::
        Get_Endpoints(
            const FCk_GroundNav_Tile& InTileA,
            FVector&                  OutMinEnd,
            FVector&                  OutMaxEnd) const
        -> void
    {
        const auto Cell = static_cast<double>(InTileA._CellSizeUu);

        if (_Direction == 0)
        {
            const auto EdgeX = InTileA._Origin.X + (static_cast<double>(InTileA._SizeX) * Cell);

            OutMinEnd = FVector{EdgeX, InTileA._Origin.Y + (static_cast<double>(_AlongMin) * Cell),
                                static_cast<double>(_MinEndZUu)};
            OutMaxEnd = FVector{EdgeX, InTileA._Origin.Y + (static_cast<double>(_AlongMax + 1) * Cell),
                                static_cast<double>(_MaxEndZUu)};

            return;
        }

        const auto EdgeY = InTileA._Origin.Y + (static_cast<double>(InTileA._SizeY) * Cell);

        OutMinEnd = FVector{InTileA._Origin.X + (static_cast<double>(_AlongMin) * Cell), EdgeY,
                            static_cast<double>(_MinEndZUu)};
        OutMaxEnd = FVector{InTileA._Origin.X + (static_cast<double>(_AlongMax + 1) * Cell), EdgeY,
                            static_cast<double>(_MaxEndZUu)};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_Field::
        Get_Tile(
            const FCk_GroundNav_TileCoord& InCoord) const
        -> const FCk_GroundNav_Tile*
    {
        const auto TileIndex = Get_TileIndex(_Params._Divisions, InCoord);

        return _Tiles.IsValidIndex(TileIndex) ? &_Tiles[TileIndex] : nullptr;
    }

    auto
        FCk_GroundNav_Field::
        Get_TileAt(
            const FVector& InWorldPosition) const
        -> const FCk_GroundNav_Tile*
    {
        return Get_Tile(_Params.Get_TileCoordAt(InWorldPosition));
    }

    auto
        FCk_GroundNav_Field::
        Get_BuiltTileCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& Tile : _Tiles)
        {
            if (Tile.Get_IsBuilt())
            { ++Count; }
        }

        return Count;
    }

    auto
        FCk_GroundNav_Field::
        Get_ReachabilityLabel(
            int32 InTileIndex,
            int32 InPlateIndex) const
        -> int32
    {
        // _TilePlateOffsets holds TileCount + 1 entries, so IsValidIndex(InTileIndex) alone would
        // admit InTileIndex == TileCount and the [InTileIndex + 1] read below would run off the end.
        if (InPlateIndex < 0 || InTileIndex < 0 || NOT _TilePlateOffsets.IsValidIndex(InTileIndex + 1))
        { return INDEX_NONE; }

        const auto Flat = _TilePlateOffsets[InTileIndex] + InPlateIndex;

        if (Flat >= _TilePlateOffsets[InTileIndex + 1])
        { return INDEX_NONE; }

        return _ReachabilityLabels.IsValidIndex(Flat) ? _ReachabilityLabels[Flat] : INDEX_NONE;
    }

    auto
        FCk_GroundNav_Field::
        Get_AreProvablyDisconnected(
            int32 InTileIndexA,
            int32 InPlateIndexA,
            int32 InTileIndexB,
            int32 InPlateIndexB) const
        -> bool
    {
        const auto LabelA = Get_ReachabilityLabel(InTileIndexA, InPlateIndexA);
        const auto LabelB = Get_ReachabilityLabel(InTileIndexB, InPlateIndexB);

        // An unlabelled plate is not PROVEN unreachable, only unknown, and the difference matters:
        // this answer is used to refuse work, and refusing on an unknown would refuse on a hole in the
        // data rather than on a fact about the world.
        if (LabelA == INDEX_NONE || LabelB == INDEX_NONE)
        { return false; }

        // Different labels prove nothing while either component still borders ground that has not
        // been baked: the crossing that would join them may exist in a tile nobody has reached. This
        // is the same refusal as the unknown-label one above, for the same reason — a hole in the
        // data is not a fact about the world.
        if (Get_IsComponentOpen(LabelA) || Get_IsComponentOpen(LabelB))
        { return false; }

        return LabelA != LabelB;
    }

    auto
        FCk_GroundNav_Field::
        Get_IsComponentOpen(
            int32 InLabel) const
        -> bool
    {
        return _ComponentIsOpen.IsValidIndex(InLabel) ? _ComponentIsOpen[InLabel] : true;
    }

    auto
        FCk_GroundNav_Field::
        Get_ReachabilityComponentCount() const
        -> int32
    {
        auto Highest = int32{INDEX_NONE};

        for (const auto Label : _ReachabilityLabels)
        { Highest = FMath::Max(Highest, Label); }

        return Highest + 1;
    }

    auto
        FCk_GroundNav_Field::
        Get_AggregatedTileEpochSum() const
        -> int64
    {
        auto Sum = int64{0};

        for (const auto& Tile : _Tiles)
        { Sum += Tile._Epoch._Value; }

        return Sum;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_FieldPublisher::
        Request_Publish(
            const TSharedRef<const FCk_GroundNav_Field>& InField)
        -> void
    {
        _Published = InField;
        _Epoch = InField->_Epoch;
        _Status = ECk_GroundNav_BuildStatus::Built;
    }

    auto
        FCk_GroundNav_FieldPublisher::
        Request_RecordFailure()
        -> void
    {
        _Status = ECk_GroundNav_BuildStatus::Failed;
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace field_private
    {
        // Only the two positive directions are composed. Every seam is shared by exactly one +X or +Y
        // step, so walking all four would emit each crossing twice under two names.
        constexpr int32 kPositiveDirectionCount = 2;

        /** One matched pair of stubs, before neighbouring pairs are run together. */
        struct FSeamCrossing
        {
            int32 _TileIndexA = INDEX_NONE;
            int32 _TileIndexB = INDEX_NONE;
            int32 _PlateA = FCk_GroundNav_Plate::kNoPlate;
            int32 _PlateB = FCk_GroundNav_Plate::kNoPlate;
            int32 _Direction = 0;
            int32 _Along = 0;

            float _ClearanceUu = 0.0f;
            float _MidZUu = 0.0f;
        };

        auto Get_IsSameRun(
            const FSeamCrossing& InPrevious,
            const FSeamCrossing& InCurrent) -> bool
        {
            return InCurrent._TileIndexA == InPrevious._TileIndexA &&
                   InCurrent._TileIndexB == InPrevious._TileIndexB &&
                   InCurrent._PlateA     == InPrevious._PlateA     &&
                   InCurrent._PlateB     == InPrevious._PlateB     &&
                   InCurrent._Direction  == InPrevious._Direction  &&
                   InCurrent._Along      == InPrevious._Along + 1;
        }

        /**
         * The neighbour's account of the same crossing, or nothing.
         *
         * Matching on BOTH surfaces is what makes this exact rather than plausible: this tile's far
         * surface must be the one the neighbour stands on, and the neighbour's far surface must be the
         * one this tile stands on. Two floors stacked over the same seam cell would otherwise be
         * indistinguishable.
         */
        auto Get_MatchingStub(
            const FCk_GroundNav_Tile&     InNeighbour,
            const FCk_GroundNav_SeamStub& InStub,
            int32                         InOppositeDirection) -> const FCk_GroundNav_SeamStub*
        {
            for (const auto& Candidate : InNeighbour._SeamStubs)
            {
                if (Candidate._Direction != InOppositeDirection ||
                    Candidate._AlongIndex != InStub._AlongIndex)
                { continue; }

                if (Candidate._NearSurfaceZUu != InStub._FarSurfaceZUu ||
                    Candidate._FarSurfaceZUu != InStub._NearSurfaceZUu)
                { continue; }

                return &Candidate;
            }

            return nullptr;
        }
    }

    namespace reachability_private
    {
        /** Union-find with path compression. Small enough to keep local; the merge is not the subject. */
        auto Get_Root(TArray<int32>& InOutParents, int32 InIndex) -> int32
        {
            auto Root = InIndex;

            while (InOutParents[Root] != Root)
            { Root = InOutParents[Root]; }

            while (InOutParents[InIndex] != Root)
            {
                const auto Next = InOutParents[InIndex];
                InOutParents[InIndex] = Root;
                InIndex = Next;
            }

            return Root;
        }

        auto Do_Union(TArray<int32>& InOutParents, int32 InLeft, int32 InRight) -> void
        {
            const auto RootLeft = Get_Root(InOutParents, InLeft);
            const auto RootRight = Get_Root(InOutParents, InRight);

            if (RootLeft == RootRight)
            { return; }

            // Toward the lower index deliberately: it makes the forest depend only on which plates are
            // connected, never on the order the merges arrived in.
            InOutParents[FMath::Max(RootLeft, RootRight)] = FMath::Min(RootLeft, RootRight);
        }

        /**
         * Whether any of a tile's four orthogonal neighbours exists in the lattice and is not built.
         *
         * A neighbour OUTSIDE the lattice is deliberately not counted: the field border is a real
         * edge by design, not an admission of ignorance, and counting it would make every
         * component of a single-tile field permanently unprovable.
         */
        auto Get_HasUnbuiltNeighbour(
            const FCk_GroundNav_Field& InField,
            int32                      InTileIndex) -> bool
        {
            const auto Divisions = InField._Params._Divisions;
            const auto Coord = Get_TileCoord(Divisions, InTileIndex);

            constexpr int32 OffsetsX[] = {1, -1, 0, 0};
            constexpr int32 OffsetsY[] = {0, 0, 1, -1};

            for (auto Side = 0; Side < 4; ++Side)
            {
                const auto NeighbourCoord = FCk_GroundNav_TileCoord{
                    Coord._X + OffsetsX[Side], Coord._Y + OffsetsY[Side]};

                if (NeighbourCoord._X < 0 || NeighbourCoord._X >= Divisions.X ||
                    NeighbourCoord._Y < 0 || NeighbourCoord._Y >= Divisions.Y)
                { continue; }

                const auto NeighbourIndex = Get_TileIndex(Divisions, NeighbourCoord);

                if (InField._Tiles.IsValidIndex(NeighbourIndex) &&
                    NOT InField._Tiles[NeighbourIndex].Get_IsBuilt())
                { return true; }
            }

            return false;
        }
    }

    auto
        DoDerive_PlateOffsets(
            FCk_GroundNav_Field& InOutField)
        -> void
    {
        InOutField._TilePlateOffsets.Reset();
        InOutField._TilePlateOffsets.Reserve(InOutField._Tiles.Num() + 1);

        auto Running = 0;

        for (const auto& Tile : InOutField._Tiles)
        {
            InOutField._TilePlateOffsets.Emplace(Running);
            Running += Tile._Plates._Plates.Num();
        }

        InOutField._TilePlateOffsets.Emplace(Running);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        DoLabel_Reachability(
            FCk_GroundNav_Field& InOutField)
        -> void
    {
        using namespace reachability_private;

        DoDerive_PlateOffsets(InOutField);
        InOutField._ReachabilityLabels.Reset();

        const auto Running = InOutField._TilePlateOffsets.Last();

        auto Parents = TArray<int32>{};
        Parents.Reserve(Running);

        for (auto Index = 0; Index < Running; ++Index)
        { Parents.Emplace(Index); }

        for (auto TileIndex = 0; TileIndex < InOutField._Tiles.Num(); ++TileIndex)
        {
            const auto Offset = InOutField._TilePlateOffsets[TileIndex];

            for (const auto& Portal : InOutField._Tiles[TileIndex]._Portals._Portals)
            { Do_Union(Parents, Offset + Portal._PlateA, Offset + Portal._PlateB); }
        }

        for (const auto& Portal : InOutField._SeamPortals)
        {
            const auto FlatA = InOutField._TilePlateOffsets[Portal._TileIndexA] + Portal._PlateA;
            const auto FlatB = InOutField._TilePlateOffsets[Portal._TileIndexB] + Portal._PlateB;

            Do_Union(Parents, FlatA, FlatB);
        }

        for (const auto& Link : InOutField._ResolvedLinks)
        {
            if (NOT Link.Get_IsTraversable())
            { continue; }

            if (NOT Parents.IsValidIndex(Link._StartFlatPlate) || NOT Parents.IsValidIndex(Link._EndFlatPlate))
            { continue; }

            // A one-directional link unions exactly as a bidirectional one does: a label only ever
            // promises that a DIFFERENT label is provably unreachable, and a one-way route leaves that
            // proof unavailable in both directions.
            Do_Union(Parents, Link._StartFlatPlate, Link._EndFlatPlate);
        }

        // Numbered in scan order AFTER the merging, so the labels say what is connected to what and
        // nothing about the schedule that discovered it.
        InOutField._ReachabilityLabels.Init(INDEX_NONE, Running);

        auto RootToLabel = TMap<int32, int32>{};

        for (auto Flat = 0; Flat < Running; ++Flat)
        {
            const auto Root = Get_Root(Parents, Flat);

            if (const auto* Existing = RootToLabel.Find(Root))
            {
                InOutField._ReachabilityLabels[Flat] = *Existing;
                continue;
            }

            const auto Label = RootToLabel.Num();
            RootToLabel.Add(Root, Label);
            InOutField._ReachabilityLabels[Flat] = Label;
        }

        // A component holding a plate in a tile that borders unbuilt ground is unprovable, and says
        // so rather than letting a caller read "disconnected" off a gap in the bake.
        InOutField._ComponentIsOpen.Init(false, RootToLabel.Num());

        for (auto TileIndex = 0; TileIndex < InOutField._Tiles.Num(); ++TileIndex)
        {
            if (NOT Get_HasUnbuiltNeighbour(InOutField, TileIndex))
            { continue; }

            const auto Begin = InOutField._TilePlateOffsets[TileIndex];
            const auto End = InOutField._TilePlateOffsets[TileIndex + 1];

            for (auto Flat = Begin; Flat < End; ++Flat)
            {
                const auto Label = InOutField._ReachabilityLabels[Flat];

                if (InOutField._ComponentIsOpen.IsValidIndex(Label))
                { InOutField._ComponentIsOpen[Label] = true; }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace field_boundary_private
    {
        // Which seam portals cross a plate on one rim side of one tile: the tile is the A side of a
        // seam leaving through its east or north face, and the B side of one arriving through its
        // west or south face.
        auto Get_IsCoveredBySeam(
            const FCk_GroundNav_SeamPortal& InSeam,
            int32                           InTileIndex,
            int32                           InPlateIndex,
            int32                           InSide,
            int32                           InAlong) -> bool
        {
            const auto Axis = InSide % 2;

            if (InSeam._Direction != Axis || InAlong < InSeam._AlongMin || InAlong > InSeam._AlongMax)
            { return false; }

            const auto IsPositiveSide = InSide == 0 || InSide == 1;

            return IsPositiveSide
                ? InSeam._TileIndexA == InTileIndex && InSeam._PlateA == InPlateIndex
                : InSeam._TileIndexB == InTileIndex && InSeam._PlateB == InPlateIndex;
        }

        auto Get_Along(
            int32            InSide,
            const FIntPoint& InCell) -> int32
        {
            return (InSide == 0 || InSide == 2) ? InCell.Y : InCell.X;
        }

        /**
         * Split every rim candidate of every built tile by the seam portals that cross it, and keep
         * what nothing crosses. A rim beside an unbuilt neighbour has no seam portals at all and is
         * therefore wholly a boundary: nothing is known past it, and a body kept off it is kept safe.
         */
        auto DoDerive_TileEdgeBoundary(
            FCk_GroundNav_Field& InOutField) -> void
        {
            InOutField._TileEdgeBoundary.Reset();
            InOutField._TileEdgeBoundary.SetNum(InOutField._Tiles.Num());

            for (auto TileIndex = 0; TileIndex < InOutField._Tiles.Num(); ++TileIndex)
            {
                const auto& Tile = InOutField._Tiles[TileIndex];

                if (NOT Tile.Get_IsBuilt())
                { continue; }

                auto Lattice = FCk_GroundNav_BoundaryLattice{};
                Lattice._Origin = Tile._Origin;
                Lattice._CellSizeUu = Tile._CellSizeUu;
                Lattice._SizeX = Tile._SizeX;
                Lattice._SizeY = Tile._SizeY;
                Lattice._LayerCount = Tile._LayerCount;
                Lattice._SurfaceZ = &Tile._SurfaceZ;

                auto& EdgeBoundary = InOutField._TileEdgeBoundary[TileIndex];

                for (const auto& Candidate : Tile._Boundary._EdgeCandidates)
                {
                    const auto StepX = FMath::Sign(Candidate._ToCell.X - Candidate._FromCell.X);
                    const auto StepY = FMath::Sign(Candidate._ToCell.Y - Candidate._FromCell.Y);
                    const auto Count = Candidate.Get_CellCount();

                    int32 RunStart = INDEX_NONE;

                    const auto Do_CloseRun = [&](int32 InEndExclusive) -> void
                    {
                        if (RunStart == INDEX_NONE)
                        { return; }

                        const auto From = FIntPoint{
                            Candidate._FromCell.X + (StepX * RunStart), Candidate._FromCell.Y + (StepY * RunStart)};
                        const auto To = FIntPoint{
                            Candidate._FromCell.X + (StepX * (InEndExclusive - 1)),
                            Candidate._FromCell.Y + (StepY * (InEndExclusive - 1))};

                        EdgeBoundary.Add(Make_BoundarySegment(
                            Lattice, Candidate._PlateIndex, Candidate._LayerIndex, Candidate._Side, From, To));

                        RunStart = INDEX_NONE;
                    };

                    for (auto Step = 0; Step < Count; ++Step)
                    {
                        const auto Cell = FIntPoint{
                            Candidate._FromCell.X + (StepX * Step), Candidate._FromCell.Y + (StepY * Step)};
                        const auto Along = Get_Along(Candidate._Side, Cell);

                        auto Covered = false;

                        for (const auto& Seam : InOutField._SeamPortals)
                        {
                            if (Get_IsCoveredBySeam(Seam, TileIndex, Candidate._PlateIndex, Candidate._Side, Along))
                            {
                                Covered = true;
                                break;
                            }
                        }

                        if (Covered)
                        {
                            Do_CloseRun(Step);
                            continue;
                        }

                        if (RunStart == INDEX_NONE)
                        { RunStart = Step; }
                    }

                    Do_CloseRun(Count);
                }
            }
        }
    }

    auto
        DoDerive_SeamPortals(
            FCk_GroundNav_Field& InOutField)
        -> void
    {
        using namespace field_private;

        InOutField._SeamPortals.Reset();
        InOutField._UnmatchedSeamStubCount = 0;

        auto Crossings = TArray<FSeamCrossing>{};

        for (auto TileIndexA = 0; TileIndexA < InOutField._Tiles.Num(); ++TileIndexA)
        {
            const auto& TileA = InOutField._Tiles[TileIndexA];

            if (NOT TileA.Get_IsBuilt())
            { continue; }

            for (auto Direction = 0; Direction < kPositiveDirectionCount; ++Direction)
            {
                const auto Offset = Get_DirectionOffset(Direction);
                const auto NeighbourCoord = FCk_GroundNav_TileCoord{
                    TileA._Coord._X + Offset.X, TileA._Coord._Y + Offset.Y};

                const auto TileIndexB = Get_TileIndex(InOutField._Params._Divisions, NeighbourCoord);

                if (NOT InOutField._Tiles.IsValidIndex(TileIndexB))
                { continue; }

                const auto& TileB = InOutField._Tiles[TileIndexB];

                // An unbuilt neighbour is a place nothing is known about, not a wall. No portal is
                // emitted and the boundary reads as unbuilt to whatever tries to cross it.
                if (NOT TileB.Get_IsBuilt())
                { continue; }

                const auto Opposite = Get_OppositeDirection(Direction);

                for (const auto& Stub : TileA._SeamStubs)
                {
                    if (Stub._Direction != Direction)
                    { continue; }

                    const auto* Match = Get_MatchingStub(TileB, Stub, Opposite);

                    if (Match == nullptr)
                    {
                        // Two BUILT tiles that disagree about a crossing they share. Counted rather than
                        // repaired: there is no third account to arbitrate between them, and the crossing
                        // simply does not exist for anything that comes after.
                        ++InOutField._UnmatchedSeamStubCount;
                        continue;
                    }

                    auto Crossing = FSeamCrossing{};

                    Crossing._TileIndexA = TileIndexA;
                    Crossing._TileIndexB = TileIndexB;
                    Crossing._PlateA = Stub._PlateIndex;
                    Crossing._PlateB = Match->_PlateIndex;
                    Crossing._Direction = Direction;
                    Crossing._Along = Stub._AlongIndex;
                    Crossing._ClearanceUu = FMath::Min(Stub._ClearanceUu, Match->_ClearanceUu);
                    Crossing._MidZUu = 0.5f * (Stub._NearSurfaceZUu + Stub._FarSurfaceZUu);

                    Crossings.Emplace(Crossing);
                }
            }
        }

        Crossings.Sort([](const FSeamCrossing& InLeft, const FSeamCrossing& InRight) -> bool
        {
            if (InLeft._TileIndexA != InRight._TileIndexA) { return InLeft._TileIndexA < InRight._TileIndexA; }
            if (InLeft._TileIndexB != InRight._TileIndexB) { return InLeft._TileIndexB < InRight._TileIndexB; }
            if (InLeft._Direction  != InRight._Direction)  { return InLeft._Direction  < InRight._Direction; }
            if (InLeft._PlateA     != InRight._PlateA)     { return InLeft._PlateA     < InRight._PlateA; }
            if (InLeft._PlateB     != InRight._PlateB)     { return InLeft._PlateB     < InRight._PlateB; }

            return InLeft._Along < InRight._Along;
        });

        for (auto Index = 0; Index < Crossings.Num(); ++Index)
        {
            const auto& Crossing = Crossings[Index];

            if (Index > 0 && Get_IsSameRun(Crossings[Index - 1], Crossing))
            {
                auto& Portal = InOutField._SeamPortals.Last();

                Portal._AlongMax = Crossing._Along;
                Portal._MaxEndZUu = Crossing._MidZUu;
                Portal._TraversalClearanceUu = FMath::Max(
                    Portal._TraversalClearanceUu, Crossing._ClearanceUu);

                continue;
            }

            auto Portal = FCk_GroundNav_SeamPortal{};

            Portal._TileIndexA = Crossing._TileIndexA;
            Portal._TileIndexB = Crossing._TileIndexB;
            Portal._PlateA = Crossing._PlateA;
            Portal._PlateB = Crossing._PlateB;
            Portal._Direction = Crossing._Direction;
            Portal._AlongMin = Crossing._Along;
            Portal._AlongMax = Crossing._Along;
            Portal._MinEndZUu = Crossing._MidZUu;
            Portal._MaxEndZUu = Crossing._MidZUu;
            Portal._TraversalClearanceUu = Crossing._ClearanceUu;

            InOutField._SeamPortals.Emplace(Portal);
        }

        field_boundary_private::DoDerive_TileEdgeBoundary(InOutField);

        // ONE line for the whole derivation, not one per stub: a field baked against a world that moved
        // mid-build produces these by the hundred, and the number is the signal.
        if (InOutField._UnmatchedSeamStubCount > 0)
        {
            ck::groundnav::Warning(
                TEXT("GroundNav field derived seam portals with [{}] unmatched seam stub(s). Two adjacent ")
                TEXT("BUILT tiles disagree about a crossing they share, which means they were baked against ")
                TEXT("different geometry. Every such crossing is absent from the field and reads as ")
                TEXT("impassable to every query afterwards."),
                InOutField._UnmatchedSeamStubCount);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoCheck_GeometryClosure(
            const ICk_GroundNav_GeometryBackend&  InBackend,
            const TArray<FCk_GroundNav_BodyRef>&  InBodies,
            TSet<uint64>&                         InOutCheckedBodies,
            TArray<FCk_GroundNav_OpenBody>&       InOutOpenBodies,
            int32&                                InOutProbes)
        -> void
    {
        auto Geometry = FCk_GroundNav_GeometryBatch{};

        for (const auto& Body : InBodies)
        {
            if (InOutCheckedBodies.Contains(Body._Value))
            { continue; }

            InOutCheckedBodies.Add(Body._Value);

            // A heightfield is open by construction and legitimately so: it has no interior to describe,
            // and holding it to the closure contract would report every terrain in the level.
            if (InBackend.Get_BodyKind(Body) == ECk_GroundNav_BodyKind::Surface)
            { continue; }

            Geometry.Reset();

            const auto TriangleCount = InBackend.Get_BodyTriangles(Body, Geometry);

            // A body the backend no longer holds yields nothing, and a mesh with no triangles is not an
            // open one — it is a body with nothing to say about any column at all.
            if (TriangleCount <= 0)
            { continue; }

            const auto Closure = Get_MeshClosure(
                Geometry, 0, TriangleCount, FCk_GroundNav_OpenBody::kMaxRecordedEdges, InOutProbes);

            if (Closure.Get_IsClosed())
            { continue; }

            auto OpenBody = FCk_GroundNav_OpenBody{};

            OpenBody._Body = Body;
            OpenBody._Description = InBackend.Get_BodyDescription(Body);
            OpenBody._Bounds = InBackend.Get_BodyBounds(Body);
            OpenBody._TriangleCount = Closure._TriangleCount;
            OpenBody._OpenEdgeCount = Closure._OpenEdgeCount;
            OpenBody._OpenEdgePoints = Closure._OpenEdgePoints;

            InOutOpenBodies.Emplace(MoveTemp(OpenBody));
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoReport_OpenBodies(
            const TArray<FCk_GroundNav_OpenBody>& InOpenBodies)
        -> void
    {
        if (InOpenBodies.IsEmpty())
        { return; }

        auto Bodies = FString{};

        for (const auto& OpenBody : InOpenBodies)
        {
            Bodies += FString::Printf(
                TEXT("\n  - %s: %d open edge(s) of %d triangle(s)"),
                *OpenBody._Description, OpenBody._OpenEdgeCount, OpenBody._TriangleCount);
        }

        const auto Plural = InOpenBodies.Num() == 1 ? FString{TEXT("y")} : FString{TEXT("ies")};

        ck::groundnav::Warning(
            TEXT("GroundNav bake found [{}] static bod{} with OPEN collision. The bake sees faces only ")
            TEXT("— a solid with no underside, a fence plane, a wall whose bottom was never modelled — ")
            TEXT("presents nothing in the columns beneath it and BAKES AS OPEN GROUND: agents will path ")
            TEXT("straight through it. Every Solid body must be a closed mesh (simple/convex collision, ")
            TEXT("or a closed collision mesh); a heightfield is exempt. The ground under these bodies is ")
            TEXT("not trustworthy until they are fixed:{}"),
            InOpenBodies.Num(), Plural, Bodies);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoBake_Field(
            const ICk_GroundNav_GeometryBackend& InBackend,
            const FCk_GroundNav_FieldParams&     InParams,
            const FCk_GroundNav_Epoch&           InEpoch,
            FCk_GroundNav_Field&                 OutField)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        OutField = FCk_GroundNav_Field{};

        if (NOT InParams.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        if (NOT InBackend.Get_IsValid())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::BackendUnavailable);
            return Result;
        }

        OutField._Params = InParams;
        OutField._Epoch = InEpoch;
        OutField._Tiles.SetNum(InParams.Get_TileCount());

        auto ProbesSpent = 0;
        auto DroppedInputCount = 0;

        auto Geometry = FCk_GroundNav_GeometryBatch{};
        auto Bodies = TArray<FCk_GroundNav_BodyRef>{};

        // Spans the whole bake, not one tile: a body straddling four tiles' halos is fetched and judged
        // once, so the closure cost is a property of the world rather than of how it was divided.
        auto CheckedBodies = TSet<uint64>{};

        for (auto TileIndex = 0; TileIndex < OutField._Tiles.Num(); ++TileIndex)
        {
            const auto Coord = Get_TileCoord(InParams._Divisions, TileIndex);
            const auto TileParams = InParams.Get_TileBakeParams(Coord, InEpoch);

            // The HALO bounds, not the tile's own. A tile handed only its own geometry reads short at
            // every edge, and the assembled field claims a pinch at every seam.
            const auto HaloBounds = Get_TileHaloBounds(TileParams);

            Geometry.Reset();
            InBackend.Get_TrianglesInBounds(HaloBounds, Geometry);

            InBackend.Get_StaticBodiesInBounds(HaloBounds, Bodies);
            DoCheck_GeometryClosure(InBackend, Bodies, CheckedBodies, OutField._OpenBodies, ProbesSpent);

            const auto TileResult = DoBake_Tile(Geometry, TileParams, OutField._Tiles[TileIndex]);

            ProbesSpent += TileResult.Get_ProbesSpent();
            DroppedInputCount += TileResult.Get_DroppedInputCount();
        }

        DoDerive_SeamPortals(OutField);
        DoResolve_Links(OutField);
        DoLabel_Reachability(OutField);

        DoReport_OpenBodies(OutField._OpenBodies);

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);
        Result.Set_DroppedInputCount(DroppedInputCount);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

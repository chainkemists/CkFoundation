#include "CkGroundNav_Query_Reachability.h"

#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"
#include "CkGroundNav/Query/CkGroundNav_Funnel.h"
#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"

#include <Algo/Reverse.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace reachability_query_private
    {
        /** One entry of the flood's frontier: a candidate crossing keyed by the distance to reach it. */
        struct FFrontierEntry
        {
            double _DistanceUu = 0.0;

            // Into the flood's candidate pool, NOT into the result's settled crossings.
            int32 _CandidateIndex = INDEX_NONE;

            auto
            operator<(
                const FFrontierEntry& InOther) const
                -> bool
            {
                if (_DistanceUu != InOther._DistanceUu)
                { return _DistanceUu < InOther._DistanceUu; }

                return _CandidateIndex < InOther._CandidateIndex;
            }
        };

        // ------------------------------------------------------------------------------------------------------------

        /** A crossing the frontier holds but has not settled: the settled shape minus its place in the result. */
        struct FFloodCandidate
        {
            FCk_GroundNav_Crossing _Crossing;

            FVector _EntryPoint = FVector::ZeroVector;

            double _DistanceUu = 0.0;

            // Into the RESULT's settled crossings, INDEX_NONE for one left from the source plate.
            int32 _Predecessor = INDEX_NONE;
        };

        // ------------------------------------------------------------------------------------------------------------

        /** The point resolved onto the surface it stands on. Radius zero: the body is already there. */
        auto Get_Resolved(
            const FCk_GroundNav_Field& InField,
            const FVector&             InLocation,
            float                      InVerticalToleranceUu) -> FCk_GroundNav_IsNavigableResult
        {
            auto Query = FCk_GroundNav_IsNavigableQuery{};
            Query._Location = InLocation;
            Query._VerticalToleranceUu = InVerticalToleranceUu;

            return Get_IsNavigable(InField, Query);
        }

        auto DoAccumulate_Cost(
            FCk_GroundNav_QueryCost&       InOutCost,
            const FCk_GroundNav_QueryCost& InAdded) -> void
        {
            InOutCost._CellsRead += InAdded._CellsRead;
            InOutCost._TilesTouched += InAdded._TilesTouched;
            InOutCost._TouchedUnbuiltTile = InOutCost._TouchedUnbuiltTile || InAdded._TouchedUnbuiltTile;
        }

        /**
         * One crossing, with its interval turned into the left and right a body walking it sees.
         *
         * Left is the end the outward direction has on its left hand. In this frame a body facing +X
         * has +Y on its RIGHT, so the left end is the one whose offset from the other has a negative
         * cross product with that direction. Deriving it rather than tabulating it is what keeps the
         * four directions from disagreeing with each other, and with the funnel.
         */
        auto Make_Crossing(
            int32          InFromFlatPlate,
            int32          InToFlatPlate,
            int32          InDirection,
            const FVector& InMinEnd,
            const FVector& InMaxEnd,
            float          InClearanceUu) -> FCk_GroundNav_Crossing
        {
            const auto Offset = Get_DirectionOffset(InDirection);

            const auto AlongX = InMaxEnd.X - InMinEnd.X;
            const auto AlongY = InMaxEnd.Y - InMinEnd.Y;

            const auto Cross = (static_cast<double>(Offset.X) * AlongY) - (static_cast<double>(Offset.Y) * AlongX);
            const auto MaxEndIsLeft = Cross < 0.0;

            auto Crossing = FCk_GroundNav_Crossing{};
            Crossing._FromFlatPlate = InFromFlatPlate;
            Crossing._ToFlatPlate = InToFlatPlate;
            Crossing._Direction = InDirection;
            Crossing._Left = MaxEndIsLeft ? InMaxEnd : InMinEnd;
            Crossing._Right = MaxEndIsLeft ? InMinEnd : InMaxEnd;
            Crossing._ClearanceUu = InClearanceUu;

            return Crossing;
        }

        /**
         * One crossing for an authored link, built WITHOUT Make_Crossing's left/right derivation.
         *
         * That derivation orients an interval against the lattice direction it is crossed in, and a
         * link has neither: both sides stand on the entry endpoint the record authored, so there is
         * nothing to orient and no lattice direction to orient it by.
         */
        auto Make_LinkCrossing(
            int32                             InFromFlatPlate,
            int32                             InToFlatPlate,
            const FCk_GroundNav_ResolvedLink& InLink,
            const FVector&                    InEntryPoint,
            int32                             InLinkIndex) -> FCk_GroundNav_Crossing
        {
            auto Crossing = FCk_GroundNav_Crossing{};
            Crossing._FromFlatPlate = InFromFlatPlate;
            Crossing._ToFlatPlate = InToFlatPlate;
            Crossing._Direction = INDEX_NONE;
            Crossing._Left = InEntryPoint;
            Crossing._Right = InEntryPoint;
            Crossing._ClearanceUu = InLink._ClearanceUu;
            Crossing._LinkIndex = InLinkIndex;

            return Crossing;
        }

        auto Get_IsTraversedForward(
            const FCk_GroundNav_ResolvedLink& InLink) -> bool
        {
            return InLink._Direction == ECk_GroundNav_LinkDirection::Bidirectional ||
                   InLink._Direction == ECk_GroundNav_LinkDirection::Forward;
        }

        auto Get_IsTraversedBackward(
            const FCk_GroundNav_ResolvedLink& InLink) -> bool
        {
            return InLink._Direction == ECk_GroundNav_LinkDirection::Bidirectional ||
                   InLink._Direction == ECk_GroundNav_LinkDirection::Backward;
        }

        auto Make_FunnelPortal(
            const FCk_GroundNav_Crossing& InCrossing) -> FCk_GroundNav_FunnelPortal
        {
            auto Portal = FCk_GroundNav_FunnelPortal{};
            Portal._Left = InCrossing._Left;
            Portal._Right = InCrossing._Right;

            return Portal;
        }

        /** The portals from the source up to and INCLUDING the given settled crossing, in walk order. */
        auto DoBuild_PortalChain(
            const FCk_GroundNav_FloodResult&    InFlood,
            int32                               InCrossingIndex,
            TArray<FCk_GroundNav_FunnelPortal>& OutChain) -> void
        {
            OutChain.Reset();

            auto Index = InCrossingIndex;

            while (InFlood._Crossings.IsValidIndex(Index))
            {
                const auto& Settled = InFlood._Crossings[Index];

                OutChain.Add(Make_FunnelPortal(Settled._Crossing));

                Index = Settled._Predecessor;
            }

            Algo::Reverse(OutChain);
        }

        /**
         * Whether this exact crossing is already settled.
         *
         * A plate is legitimately entered through several crossings and every one of them is settled;
         * only the same interval from the same plate is a duplicate. The endpoints compare exactly
         * because both came from the same arithmetic over the same portal.
         */
        auto Get_IsCrossingSettled(
            const FCk_GroundNav_FloodResult& InFlood,
            const FCk_GroundNav_Crossing&    InCrossing) -> bool
        {
            if (NOT InFlood._PlateEntries.IsValidIndex(InCrossing._ToFlatPlate))
            { return false; }

            for (const auto EntryIndex : InFlood._PlateEntries[InCrossing._ToFlatPlate])
            {
                if (NOT InFlood._Crossings.IsValidIndex(EntryIndex))
                { continue; }

                const auto& Settled = InFlood._Crossings[EntryIndex]._Crossing;

                if (Settled._FromFlatPlate == InCrossing._FromFlatPlate &&
                    Settled._Direction == InCrossing._Direction &&
                    Settled._LinkIndex == InCrossing._LinkIndex &&
                    Settled._Left == InCrossing._Left &&
                    Settled._Right == InCrossing._Right)
                { return true; }
            }

            return false;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_FlatPlateIndex(
            const FCk_GroundNav_Field& InField,
            int32                      InTileIndex,
            int32                      InPlateIndex)
        -> int32
    {
        // _TilePlateOffsets holds TileCount + 1 entries, so IsValidIndex(InTileIndex) alone would admit
        // InTileIndex == TileCount and the [InTileIndex + 1] read below would run off the end.
        if (InTileIndex < 0 || InPlateIndex < 0 || NOT InField._TilePlateOffsets.IsValidIndex(InTileIndex + 1))
        { return INDEX_NONE; }

        const auto Flat = InField._TilePlateOffsets[InTileIndex] + InPlateIndex;

        return Flat < InField._TilePlateOffsets[InTileIndex + 1] ? Flat : INDEX_NONE;
    }

    auto
        Get_TileAndPlate(
            const FCk_GroundNav_Field& InField,
            int32                      InFlatPlate,
            int32&                     OutTileIndex,
            int32&                     OutPlateIndex)
        -> bool
    {
        const auto& Offsets = InField._TilePlateOffsets;

        if (InFlatPlate < 0 || Offsets.Num() < 2 || InFlatPlate >= Offsets.Last())
        { return false; }

        // The tile whose range holds the index is the LAST one whose offset does not pass it. Offsets
        // are non-decreasing, so a tile with no plates of its own can never be that one.
        auto Low = int32{0};
        auto High = Offsets.Num() - 2;
        auto Found = int32{INDEX_NONE};

        while (Low <= High)
        {
            const auto Mid = Low + ((High - Low) / 2);

            if (Offsets[Mid] <= InFlatPlate)
            {
                Found = Mid;
                Low = Mid + 1;
            }
            else
            {
                High = Mid - 1;
            }
        }

        if (Found == INDEX_NONE)
        { return false; }

        OutTileIndex = Found;
        OutPlateIndex = InFlatPlate - Offsets[Found];

        return true;
    }

    auto
        Get_FlatPlateCount(
            const FCk_GroundNav_Field& InField)
        -> int32
    {
        return InField._TilePlateOffsets.IsEmpty() ? 0 : InField._TilePlateOffsets.Last();
    }

    auto
        Get_CrossingsFrom(
            const FCk_GroundNav_Field&      InField,
            int32                           InFlatPlate,
            TArray<FCk_GroundNav_Crossing>& OutCrossings,
            FCk_GroundNav_QueryCost&        InOutCost)
        -> void
    {
        using namespace reachability_query_private;

        auto TileIndex = int32{INDEX_NONE};
        auto PlateIndex = int32{INDEX_NONE};

        if (NOT Get_TileAndPlate(InField, InFlatPlate, TileIndex, PlateIndex))
        { return; }

        if (NOT InField._Tiles.IsValidIndex(TileIndex))
        { return; }

        const auto& Tile = InField._Tiles[TileIndex];

        if (NOT Tile.Get_IsBuilt())
        { return; }

        for (const auto PortalIndex : Tile._Portals.Get_PortalsForPlate(PlateIndex))
        {
            ++InOutCost._CellsRead;

            if (NOT Tile._Portals._Portals.IsValidIndex(PortalIndex))
            { continue; }

            const auto& Portal = Tile._Portals._Portals[PortalIndex];

            const auto LeavesFromA = Portal._PlateA == PlateIndex;

            if (NOT LeavesFromA && Portal._PlateB != PlateIndex)
            { continue; }

            const auto ToFlatPlate = Get_FlatPlateIndex(
                InField, TileIndex, LeavesFromA ? Portal._PlateB : Portal._PlateA);

            if (ToFlatPlate == INDEX_NONE)
            { continue; }

            auto MinEnd = FVector::ZeroVector;
            auto MaxEnd = FVector::ZeroVector;

            Portal.Get_Endpoints(Tile._Origin, Tile._CellSizeUu, MinEnd, MaxEnd);

            OutCrossings.Add(Make_Crossing(
                InFlatPlate,
                ToFlatPlate,
                LeavesFromA ? Portal._Direction : Get_OppositeDirection(Portal._Direction),
                MinEnd,
                MaxEnd,
                Portal._TraversalClearanceUu));
        }

        for (const auto& Seam : InField._SeamPortals)
        {
            ++InOutCost._CellsRead;

            const auto LeavesFromA = Seam._TileIndexA == TileIndex && Seam._PlateA == PlateIndex;
            const auto LeavesFromB = Seam._TileIndexB == TileIndex && Seam._PlateB == PlateIndex;

            if (NOT LeavesFromA && NOT LeavesFromB)
            { continue; }

            const auto OtherTileIndex = LeavesFromA ? Seam._TileIndexB : Seam._TileIndexA;

            if (NOT InField._Tiles.IsValidIndex(OtherTileIndex) || NOT InField._Tiles[OtherTileIndex].Get_IsBuilt())
            { continue; }

            const auto ToFlatPlate = Get_FlatPlateIndex(
                InField, OtherTileIndex, LeavesFromA ? Seam._PlateB : Seam._PlateA);

            if (ToFlatPlate == INDEX_NONE || NOT InField._Tiles.IsValidIndex(Seam._TileIndexA))
            { continue; }

            auto MinEnd = FVector::ZeroVector;
            auto MaxEnd = FVector::ZeroVector;

            // Placed against the A-side tile whichever way this crossing is being left: the seam lies
            // on that tile's far edge, and the interval is the same segment seen from either side.
            Seam.Get_Endpoints(InField._Tiles[Seam._TileIndexA], MinEnd, MaxEnd);

            OutCrossings.Add(Make_Crossing(
                InFlatPlate,
                ToFlatPlate,
                LeavesFromA ? Seam._Direction : Get_OppositeDirection(Seam._Direction),
                MinEnd,
                MaxEnd,
                Seam._TraversalClearanceUu));
        }

        for (auto LinkIndex = 0; LinkIndex < InField._ResolvedLinks.Num(); ++LinkIndex)
        {
            ++InOutCost._CellsRead;

            const auto& Link = InField._ResolvedLinks[LinkIndex];

            // An end that resolved onto nothing, and a link switched off, join nothing at all: the
            // record stays on its volume either way and only the field's graph drops it.
            if (NOT Link.Get_IsTraversable())
            { continue; }

            // Both ends on one plate is a legal shortcut across it — a ladder up and back down the
            // same floor — so it is emitted like any other, not skipped as degenerate.
            if (Link._StartFlatPlate == InFlatPlate && Get_IsTraversedForward(Link))
            {
                OutCrossings.Add(Make_LinkCrossing(
                    InFlatPlate,
                    Link._EndFlatPlate,
                    Link,
                    Link._Start,
                    LinkIndex));
            }

            if (Link._EndFlatPlate == InFlatPlate && Get_IsTraversedBackward(Link))
            {
                OutCrossings.Add(Make_LinkCrossing(
                    InFlatPlate,
                    Link._StartFlatPlate,
                    Link,
                    Link._End,
                    LinkIndex));
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsReachable(
            const FCk_GroundNav_Field&             InField,
            const FCk_GroundNav_ReachabilityQuery& InQuery)
        -> FCk_GroundNav_ReachabilityResult
    {
        using namespace reachability_query_private;

        auto Result = FCk_GroundNav_ReachabilityResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;
            return Result;
        }

        const auto Start = Get_Resolved(InField, InQuery._Start, InQuery._VerticalToleranceUu);

        DoAccumulate_Cost(Result._Cost, Start._Cost);

        if (NOT Start.Get_IsSuccess())
        {
            Result._Status = Start._Status;
            return Result;
        }

        const auto End = Get_Resolved(InField, InQuery._End, InQuery._VerticalToleranceUu);

        DoAccumulate_Cost(Result._Cost, End._Cost);

        Result._StartSurface = Start._Surface;

        if (NOT End.Get_IsSuccess())
        {
            Result._Status = End._Status;
            return Result;
        }

        Result._Status = ECk_NavSurface_QueryStatus::Success;
        Result._EndSurface = End._Surface;

        const auto StartLabel = InField.Get_ReachabilityLabel(Start._Surface._TileIndex, Start._Surface._PlateIndex);
        const auto EndLabel = InField.Get_ReachabilityLabel(End._Surface._TileIndex, End._Surface._PlateIndex);

        if (StartLabel == EndLabel)
        {
            Result._Reachability = ECk_GroundNav_Reachability::PossiblyReachable;
        }
        else if (InField.Get_IsComponentOpen(StartLabel) || InField.Get_IsComponentOpen(EndLabel))
        {
            Result._Reachability = ECk_GroundNav_Reachability::Unknown_OpenComponent;
        }
        else
        {
            Result._Reachability = ECk_GroundNav_Reachability::Unreachable;
        }

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_FloodFill(
            const FCk_GroundNav_Field&                             InField,
            const FCk_GroundNav_FloodQuery&                        InQuery,
            TFunctionRef<bool(const FCk_GroundNav_FloodCrossing&)> InShouldStop)
        -> FCk_GroundNav_FloodResult
    {
        using namespace reachability_query_private;

        auto Result = FCk_GroundNav_FloodResult{};

        if (NOT Get_IsRadiusAnswerable(InField, InQuery._Agent))
        {
            Result._Status = ECk_NavSurface_QueryStatus::Blocked;
            return Result;
        }

        const auto Source = Get_Resolved(InField, InQuery._Source, InQuery._VerticalToleranceUu);

        DoAccumulate_Cost(Result._Cost, Source._Cost);

        if (NOT Source.Get_IsSuccess())
        {
            Result._Status = Source._Status;
            return Result;
        }

        const auto SourceFlatPlate = Get_FlatPlateIndex(
            InField, Source._Surface._TileIndex, Source._Surface._PlateIndex);

        if (SourceFlatPlate == INDEX_NONE)
        {
            Result._Status = ECk_NavSurface_QueryStatus::NoSurface;
            return Result;
        }

        Result._Status = ECk_NavSurface_QueryStatus::Success;
        Result._SourceSurface = Source._Surface;
        Result._SourceFlatPlate = SourceFlatPlate;
        Result._SourcePoint = FVector{InQuery._Source.X, InQuery._Source.Y, static_cast<double>(Source._SurfaceZUu)};
        Result._PlateEntries.SetNum(Get_FlatPlateCount(InField));

        const auto RadiusUu = InQuery._Agent._RadiusUu;
        const auto MaxDistanceUu = static_cast<double>(InQuery._MaxDistanceUu);

        auto Candidates = TArray<FFloodCandidate>{};
        auto Frontier = TArray<FFrontierEntry>{};

        auto Waypoints = TArray<FVector>{};
        auto Chain = TArray<FCk_GroundNav_FunnelPortal>{};
        auto Reachable = TArray<FCk_GroundNav_Crossing>{};

        const auto DoRelax = [&](
            const FCk_GroundNav_Crossing&               InCrossing,
            TConstArrayView<FCk_GroundNav_FunnelPortal> InChain,
            int32                                       InPredecessor) -> void
        {
            if (NOT Get_IsAdmitted(InCrossing._ClearanceUu, InQuery._Agent))
            { return; }

            auto EntryPoint = FVector::ZeroVector;

            const auto DistanceUu = Get_StringPull_ToSegment(
                Result._SourcePoint, InChain, Make_FunnelPortal(InCrossing), RadiusUu, EntryPoint, Waypoints);

            auto Candidate = FFloodCandidate{};
            Candidate._Crossing = InCrossing;
            Candidate._EntryPoint = EntryPoint;
            Candidate._DistanceUu = DistanceUu;
            Candidate._Predecessor = InPredecessor;

            Candidates.Add(Candidate);

            Frontier.HeapPush(FFrontierEntry{DistanceUu, Candidates.Num() - 1}, TLess<>{});
        };

        Get_CrossingsFrom(InField, SourceFlatPlate, Reachable, Result._Cost);

        for (const auto& Crossing : Reachable)
        { DoRelax(Crossing, TConstArrayView<FCk_GroundNav_FunnelPortal>{}, INDEX_NONE); }

        while (NOT Frontier.IsEmpty())
        {
            auto Top = FFrontierEntry{};
            Frontier.HeapPop(Top, TLess<>{});

            if (NOT Candidates.IsValidIndex(Top._CandidateIndex))
            { continue; }

            // The cap bounds the COUNT, so it is tested before the pop is counted: a cap of three that
            // reported four would not be a cap.
            if (InQuery._MaxExpansions > 0 && Result._ExpansionCount >= InQuery._MaxExpansions)
            { break; }

            ++Result._ExpansionCount;

            // COPIED, not referenced: relaxing below appends to the pool the candidate lives in.
            const auto Candidate = Candidates[Top._CandidateIndex];

            // The heap is ordered, so nothing nearer than this is still waiting.
            if (MaxDistanceUu > 0.0 && Candidate._DistanceUu > MaxDistanceUu)
            { break; }

            auto Settled = FCk_GroundNav_FloodCrossing{};
            Settled._Crossing = Candidate._Crossing;
            Settled._EntryPoint = Candidate._EntryPoint;
            Settled._DistanceUu = Candidate._DistanceUu;
            Settled._Predecessor = Candidate._Predecessor;

            if (InShouldStop(Settled))
            { break; }

            if (Get_IsCrossingSettled(Result, Settled._Crossing))
            { continue; }

            if (NOT Result._PlateEntries.IsValidIndex(Settled._Crossing._ToFlatPlate))
            { continue; }

            const auto SettledIndex = Result._Crossings.Add(Settled);
            Result._PlateEntries[Settled._Crossing._ToFlatPlate].Add(SettledIndex);

            DoBuild_PortalChain(Result, SettledIndex, Chain);

            Reachable.Reset();
            Get_CrossingsFrom(InField, Settled._Crossing._ToFlatPlate, Reachable, Result._Cost);

            for (const auto& Next : Reachable)
            {
                if (Next._ToFlatPlate == Settled._Crossing._FromFlatPlate)
                { continue; }

                DoRelax(Next, Chain, SettledIndex);
            }
        }

        return Result;
    }

    auto
        Get_FloodFill(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_FloodQuery& InQuery)
        -> FCk_GroundNav_FloodResult
    {
        const auto NeverStop = [](const FCk_GroundNav_FloodCrossing&) -> bool
        {
            return false;
        };

        return Get_FloodFill(InField, InQuery, NeverStop);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_FloodDistanceTo(
            const FCk_GroundNav_Field&       InField,
            const FCk_GroundNav_FloodResult& InFlood,
            const FVector&                   InTarget,
            float                            InVerticalToleranceUu,
            const FCk_GroundNav_QueryAgent&  InAgent)
        -> TOptional<double>
    {
        using namespace reachability_query_private;

        if (NOT InFlood.Get_IsSuccess())
        { return {}; }

        const auto Target = Get_Resolved(InField, InTarget, InVerticalToleranceUu);

        if (NOT Target.Get_IsSuccess())
        { return {}; }

        const auto TargetPoint = FVector{InTarget.X, InTarget.Y, static_cast<double>(Target._SurfaceZUu)};

        const auto TargetFlatPlate = Get_FlatPlateIndex(
            InField, Target._Surface._TileIndex, Target._Surface._PlateIndex);

        if (TargetFlatPlate == INDEX_NONE)
        { return {}; }

        // A plate is a convex rectangle, so the straight line between two of its points stays on it.
        if (TargetFlatPlate == InFlood._SourceFlatPlate)
        {
            return FVector2D::Distance(
                FVector2D{InFlood._SourcePoint.X, InFlood._SourcePoint.Y},
                FVector2D{TargetPoint.X, TargetPoint.Y});
        }

        if (NOT InFlood._PlateEntries.IsValidIndex(TargetFlatPlate) ||
            InFlood._PlateEntries[TargetFlatPlate].IsEmpty())
        { return {}; }

        auto Waypoints = TArray<FVector>{};
        auto Chain = TArray<FCk_GroundNav_FunnelPortal>{};
        auto Best = TOptional<double>{};

        for (const auto EntryIndex : InFlood._PlateEntries[TargetFlatPlate])
        {
            if (NOT InFlood._Crossings.IsValidIndex(EntryIndex))
            { continue; }

            DoBuild_PortalChain(InFlood, EntryIndex, Chain);

            const auto DistanceUu = Get_StringPull(
                InFlood._SourcePoint, TargetPoint, Chain, InAgent._RadiusUu, Waypoints);

            if (NOT Best.IsSet() || DistanceUu < Best.GetValue())
            { Best = DistanceUu; }
        }

        return Best;
    }

    auto
        Get_FloodDistancesTo(
            const FCk_GroundNav_Field&       InField,
            const FCk_GroundNav_FloodResult& InFlood,
            TConstArrayView<FVector>         InTargets,
            float                            InVerticalToleranceUu,
            const FCk_GroundNav_QueryAgent&  InAgent,
            TArray<TOptional<double>>&       OutDistances)
        -> void
    {
        OutDistances.Reset();
        OutDistances.Reserve(InTargets.Num());

        for (const auto& Target : InTargets)
        { OutDistances.Add(Get_FloodDistanceTo(InField, InFlood, Target, InVerticalToleranceUu, InAgent)); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

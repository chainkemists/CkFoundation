#include "CkGroundNav_FieldLinks.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"

// --------------------------------------------------------------------------------------------------------------------
// THE ONE IMPLEMENTATION FILE UNDER Field/ THAT INCLUDES Query/.
//
// Resolving an authored link IS a projection over the field being composed - a link is two world
// points, and what they stand on is exactly what Get_ProjectPoint answers - and the flat plate a
// crossing and a label speak is exactly what Get_FlatPlateIndex answers. The three places a link has
// to be resolved are the field-level composition points, so that answer has to be reachable
// from where a field is composed.
//
// The header layering is untouched: no Field/ HEADER includes Query/, so there is no header cycle and
// nothing about the order the reflected types are generated in moves.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace fieldlinks_private
    {
        auto Do_ResolveEnd(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_LinkRecord& InRecord,
            const FVector&                  InPoint,
            FCk_GroundNav_SurfaceRef&       OutSurface,
            int32&                          OutFlatPlate,
            ECk_NavSurface_QueryStatus&     OutStatus) -> void
        {
            auto Query = FCk_GroundNav_ProjectionQuery{};

            Query._Location = InPoint;
            Query._HorizontalExtentUu = InRecord.Get_ProjectionHorizontalExtentUu();
            Query._UpExtentUu = InRecord.Get_ProjectionVerticalExtentUu();
            Query._DownExtentUu = InRecord.Get_ProjectionVerticalExtentUu();
            Query._Mode = InRecord.Get_ProjectionMode();

            // The agent is left at its default zero radius, which admits every walkable cell. Whether a
            // body fits is decided later against the record's own _ClearanceUu; an endpoint narrowed by
            // the widest agent that might one day use the link would resolve differently per asker, and
            // the resolution is a property of the field.
            const auto Projection = Get_ProjectPoint(InField, Query);

            OutStatus = Projection._Status;

            if (NOT Projection.Get_IsSuccess())
            { return; }

            OutSurface = Projection._Surface;
            OutFlatPlate = Get_FlatPlateIndex(
                InField, Projection._Surface._TileIndex, Projection._Surface._PlateIndex);
        }

        auto Do_AddEndTiles(
            const FCk_GroundNav_ResolvedLink& InLink,
            TSet<int32>&                      OutTileIndices) -> void
        {
            if (InLink._StartSurface._TileIndex != INDEX_NONE)
            { OutTileIndices.Add(InLink._StartSurface._TileIndex); }

            if (InLink._EndSurface._TileIndex != INDEX_NONE)
            { OutTileIndices.Add(InLink._EndSurface._TileIndex); }
        }

        /**
         * The tiles the news is about: every end, before and after, of every link whose resolved entry
         * is not the entry of the same id in the field it was derived from.
         *
         * Matched by ID rather than by position, because the list a derive is handed is the authored
         * list and one removal moves every record after it. Compared as a WHOLE entry rather than by
         * the fields a caller happens to care about, so a multiplier that repriced a link moves the
         * ground that link is on exactly as a link that moved plates does.
         *
         * OutChangedIds collects the ids of exactly those entries, on the same walk, so the two halves
         * of one answer cannot drift apart. An entry whose ends resolved to nothing stamps no tile and
         * still names its id: what changed is the LINK, and a reader keyed on links has to hear it.
         */
        auto Get_ChangedLinkEndTiles(
            const TArray<FCk_GroundNav_ResolvedLink>& InBefore,
            const TArray<FCk_GroundNav_ResolvedLink>& InAfter,
            TSet<int32>&                             OutChangedIds) -> TSet<int32>
        {
            auto BeforeIndexById = TMap<int32, int32>{};
            BeforeIndexById.Reserve(InBefore.Num());

            for (auto Index = 0; Index < InBefore.Num(); ++Index)
            { BeforeIndexById.Emplace(InBefore[Index]._Id, Index); }

            auto TileIndices = TSet<int32>{};
            auto MatchedBefore = TSet<int32>{};

            for (const auto& After : InAfter)
            {
                const auto* BeforeIndex = BeforeIndexById.Find(After._Id);

                if (BeforeIndex == nullptr)
                {
                    Do_AddEndTiles(After, TileIndices);
                    OutChangedIds.Add(After._Id);
                    continue;
                }

                MatchedBefore.Add(*BeforeIndex);

                const auto& Before = InBefore[*BeforeIndex];

                if (Before == After)
                { continue; }

                Do_AddEndTiles(Before, TileIndices);
                Do_AddEndTiles(After, TileIndices);

                // The id rather than the two entries: an id is the same on both sides of a change by
                // construction, and it is the only half of this answer that survives a renumbering.
                OutChangedIds.Add(After._Id);
            }

            for (auto Index = 0; Index < InBefore.Num(); ++Index)
            {
                if (MatchedBefore.Contains(Index))
                { continue; }

                Do_AddEndTiles(InBefore[Index], TileIndices);
                OutChangedIds.Add(InBefore[Index]._Id);
            }

            return TileIndices;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoResolve_Links(
            FCk_GroundNav_Field& InOutField)
        -> void
    {
        using namespace fieldlinks_private;

        // A flat plate index is only as good as the numbering it was taken under, and the labelling
        // pass that runs after this one re-derives the same numbering rather than inheriting it, so
        // neither pass depends on which of them ran first.
        DoDerive_PlateOffsets(InOutField);

        InOutField._ResolvedLinks.Reset();
        InOutField._ResolvedLinks.Reserve(InOutField._Params._Links.Num());

        InOutField._UnresolvedLinkCount = 0;

        for (const auto& Record : InOutField._Params._Links)
        {
            auto Resolved = FCk_GroundNav_ResolvedLink{};

            Resolved._Id = Record.Get_Id();
            Resolved._Start = Record.Get_Start();
            Resolved._End = Record.Get_End();
            Resolved._Direction = Record.Get_Direction();
            Resolved._CostMultiplierForward = Record.Get_CostMultiplierForward();
            Resolved._CostMultiplierBackward = Record.Get_CostMultiplierBackward();
            Resolved._ClearanceUu = Record.Get_ClearanceUu();
            Resolved._AreaTag = Record.Get_AreaTag();
            Resolved._UserTypeTag = Record.Get_UserTypeTag();
            Resolved._Enable = Record.Get_Enable();

            // A DISABLED link is projected exactly as an enabled one is. Its status is then an honest
            // account of the ground under it, which is what lets switching it back on be a decision
            // about the link rather than a question about ground nobody looked at.
            Do_ResolveEnd(InOutField, Record, Record.Get_Start(),
                Resolved._StartSurface, Resolved._StartFlatPlate, Resolved._StartStatus);

            Do_ResolveEnd(InOutField, Record, Record.Get_End(),
                Resolved._EndSurface, Resolved._EndFlatPlate, Resolved._EndStatus);

            if (NOT Resolved.Get_IsResolved())
            { ++InOutField._UnresolvedLinkCount; }

            InOutField._ResolvedLinks.Emplace(Resolved);
        }

        if (InOutField._UnresolvedLinkCount == 0)
        { return; }

        // Display, never Warning. An end over ground that is not baked yet is the expected state of a
        // link authored ahead of its geometry, and a build that warned about it would be reporting the
        // schedule rather than a defect.
        ck::groundnav::Display(
            TEXT("GroundNav field at epoch [{}] resolved [{}] authored link(s), of which [{}] have an "
                 "end that found no ground. Such a link contributes no crossing and no reachability "
                 "until a publish over that ground resolves it; its record is kept either way, and the "
                 "per-end status says whether the ground is missing or merely unbaked"),
            InOutField._Epoch._Value, InOutField._ResolvedLinks.Num(), InOutField._UnresolvedLinkCount);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_FieldWithLinks(
            const FCk_GroundNav_Field&              InField,
            const TArray<FCk_GroundNav_LinkRecord>& InLinks,
            const FCk_GroundNav_Epoch&              InEpoch)
        -> FCk_GroundNav_LinkDeriveResult
    {
        using namespace fieldlinks_private;

        auto StageResult = FCk_GroundNav_BakeStageResult{};
        auto Derived = MakeShared<FCk_GroundNav_Field>(InField);

        Derived->_Params._Links = InLinks;

        DoResolve_Links(*Derived);
        DoLabel_Reachability(*Derived);

        auto ChangedIds = TSet<int32>{};

        const auto ChangedTiles =
            Get_ChangedLinkEndTiles(InField._ResolvedLinks, Derived->_ResolvedLinks, ChangedIds);

        auto ChangedAnyTile = false;

        for (const auto TileIndex : ChangedTiles)
        {
            if (NOT Derived->_Tiles.IsValidIndex(TileIndex))
            { continue; }

            Derived->_Tiles[TileIndex]._Epoch = InEpoch;
            ChangedAnyTile = true;
        }

        if (ChangedAnyTile)
        { Derived->_Epoch = InEpoch; }

        StageResult.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        auto Result = FCk_GroundNav_LinkDeriveResult{};

        Result._Field = Derived;
        Result._Result = StageResult;
        Result._ChangedLinkIds = ChangedIds.Array();
        Result._ChangedLinkIds.Sort();

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

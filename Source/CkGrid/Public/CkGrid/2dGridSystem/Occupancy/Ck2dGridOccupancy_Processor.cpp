#include "Ck2dGridOccupancy_Processor.h"

#include "Ck2dGridOccupancy_Utils.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment.h"
#include "CkGrid/CkGrid_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkRecord/Record/CkRecord_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridOccupancy_StampCells);
CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridOccupancy_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridOccupancy_SyncReplication);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_2dGridOccupancy_StampCells::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_2dGridOccupancy_Current& InCurrent) const
        -> void
    {
        // 1. Build the desired set: every cell of every VALID placement -> that placement.
        //    Valid entries only; dead placements are already pruned by CkRecord.
        auto GridBase = FCk_Handle{InHandle};
        auto Desired = TMap<FIntPoint, FCk_Handle_2dGridPlacement>{};
        RecordOf_GridPlacements_Utils::ForEach_ValidEntry(GridBase,
        [&](FCk_Handle_2dGridPlacement InPlacement)
        {
            const auto& Params = InPlacement.Get<FFragment_2dGridPlacement_Params>();
            for (const auto& Coord : Params.Get_Cells())
            {
                Desired.Add(Coord, InPlacement);
            }
        });

        // 2. Un-stamp cells that are stamped now but not in Desired.
        for (const auto& StampedPair : InCurrent._StampedCells)
        {
            if (Desired.Contains(StampedPair.Key))
            { continue; }

            auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InHandle, StampedPair.Key);
            if (ck::Is_NOT_Valid(Cell))
            { continue; }

            Cell.Remove<FTag_2dGridCell_Occupied>();

            if (Cell.Has<FFragment_2dGridCell_Occupancy>())
            { Cell.Get<FFragment_2dGridCell_Occupancy>()._Placement = FCk_Handle_2dGridPlacement{}; }
        }

        // 3. Stamp cells that are desired but not currently stamped (or stamped by a different placement).
        for (const auto& DesiredPair : Desired)
        {
            const auto* MaybeStamped = InCurrent._StampedCells.Find(DesiredPair.Key);
            if (MaybeStamped != nullptr && *MaybeStamped == DesiredPair.Value)
            { continue; }

            auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(InHandle, DesiredPair.Key);
            if (ck::Is_NOT_Valid(Cell))
            { continue; }

            Cell.AddOrGet<FTag_2dGridCell_Occupied>();
            Cell.AddOrGet<FFragment_2dGridCell_Occupancy>()._Placement = DesiredPair.Value;
        }

        // 4. The desired set is now authoritative.
        InCurrent._StampedCells = MoveTemp(Desired);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_2dGridOccupancy_Replicate::
        ForEachEntity(
            TimeType,
            HandleType InHandle) const
        -> void
    {
        auto GridBase = FCk_Handle{InHandle};

        auto Entries = TArray<FCk_2dGridPlacement_ReplicatedEntry>{};

        RecordOf_GridPlacements_Utils::ForEach_ValidEntry(GridBase,
        [&](FCk_Handle_2dGridPlacement InPlacement)
        {
            const auto& Params = InPlacement.Get<FFragment_2dGridPlacement_Params>();

            Entries.Emplace(FCk_2dGridPlacement_ReplicatedEntry(Params.Get_Occupant())
                .Set_Anchor(Params.Get_Anchor())
                .Set_Rotation(Params.Get_Rotation())
                .Set_Cells(Params.Get_Cells()));
        });

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_2dGridPlacements>(
            GridBase, [&](FCk_RepData_2dGridPlacements& Data)
        {
            Data.Placements = MoveTemp(Entries);
        });

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_2dGridOccupancy_SyncReplication::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_2dGridOccupancy_SyncReplication& InSync) const
        -> void
    {
        const auto& InCurrent  = InSync.Get_PlacementsToReplicate();
        const auto& InPrevious = InSync.Get_PlacementsToReplicate_Previous();

        if (InCurrent == InPrevious)
        {
            InHandle.Remove<FFragment_2dGridOccupancy_SyncReplication>();
            return;
        }

        // Identity is the occupant handle. Diffing only by occupant would miss a same-occupant
        // re-place (move/rotate keeps the occupant, changes anchor/cells), so any occupant whose
        // ENTRY changed is treated as a remove-then-add. The set of occupants needing a teardown is
        // therefore "everything in Previous that is not present-AND-identical in Current".
        const auto ByOccupant = &FCk_2dGridPlacement_ReplicatedEntry::Get_Occupant;
        const auto OccupantsGone = ck::algo::Except(InPrevious, InCurrent, ByOccupant);

        auto Grid = InHandle;

        // Teardown pass: an occupant whose entry is gone OR changed loses its client placement first
        // (CkRecord prunes it; reconcile un-stamps the cells next tick). A removed-then-readded entry
        // in the same delta frees its old footprint before the add pass re-places it.
        const auto TearDownOccupant = [&](const FCk_Handle& InOccupant)
        {
            auto Existing = UCk_Utils_2dGridOccupancy_UE::Get_PlacementForOccupant(InOccupant);
            if (ck::Is_NOT_Valid(Existing))
            { return; }
            UCk_Utils_2dGridOccupancy_UE::Request_RemovePlacement(Existing);
        };

        for (const auto& Entry : OccupantsGone)
        { TearDownOccupant(Entry.Get_Occupant()); }

        // Apply pass: every current entry that differs from its previous counterpart (new occupant,
        // or same occupant with a changed footprint) is (re)built. Request_AddPlacement is the raw,
        // un-authority-gated data layer — it auto-replaces any stale placement for that occupant,
        // connects into the record, and the reconcile pass stamps the cells. Clients never originate
        // placements; this only mirrors replicated state.
        for (const auto& Entry : InCurrent)
        {
            const auto* Prev = InPrevious.FindByPredicate([&](const FCk_2dGridPlacement_ReplicatedEntry& InPrev)
            { return InPrev.Get_Occupant() == Entry.Get_Occupant(); });

            if (Prev != nullptr && *Prev == Entry)
            { continue; }

            // Re-place over a changed occupant: drop the stale placement so the new footprint is clean.
            TearDownOccupant(Entry.Get_Occupant());

            auto Occupant = Entry.Get_Occupant();
            UCk_Utils_2dGridOccupancy_UE::Request_AddPlacement(
                Grid, Occupant, Entry.Get_Anchor(), Entry.Get_Rotation(), Entry.Get_Cells());
        }

        InHandle.Remove<FFragment_2dGridOccupancy_SyncReplication>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

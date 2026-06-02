#include "Ck2dGridOccupancy_Processor.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridOccupancy_StampCells);

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
}

// --------------------------------------------------------------------------------------------------------------------

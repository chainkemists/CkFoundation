#include "Ck2dGridOccupancy_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Snapshot registrations (aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the name). The grid's
// placement record + each occupant's back-ref round-trip; FFragment_2dGridOccupancy_Current does NOT
// (its _StampedCells map is DERIVED — the StampCells reconcile rebuilds it from the restored record).

using FSnap_RecordOf_GridPlacements    = ck::FFragment_RecordOf_GridPlacements;
using FSnap_2dGridOccupant_PlacementRef = ck::FFragment_2dGridOccupant_PlacementRef;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOf_GridPlacements);
CK_REGISTER_SNAPSHOTABLE(FSnap_2dGridOccupant_PlacementRef);

// --------------------------------------------------------------------------------------------------------------------

// Client-side RegisterLazy registrar. The container entry lives on the GRID entity, so the
// handler's Entity is the grid itself — apply the SyncReplication fragment to it directly
// (no inventory-style record indirection). The ClientOnly SyncReplication processor then
// rebuilds placement entities and the reconcile pass stamps the cells.
[[maybe_unused]] static struct F2dGridOccupancy_RepHandlerRegistrar
{
    F2dGridOccupancy_RepHandlerRegistrar()
    {
        const auto DoApplyPlacements = [](FCk_Handle& Entity,
            const TArray<FCk_2dGridPlacement_ReplicatedEntry>& NewPlacements,
            const TArray<FCk_2dGridPlacement_ReplicatedEntry>& OldPlacements)
        {
            Entity.AddOrGet<ck::FFragment_2dGridOccupancy_SyncReplication>(NewPlacements, OldPlacements);
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_2dGridPlacements::StaticStruct(); },
            {
                // Stamps the sync fragment consumed by the Occupancy SyncReplication processor
                // (which owns rebuild + reconcile) — always Applied, the processor has its own gating.
                .Apply = [DoApplyPlacements](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    DoApplyPlacements(Entity,
                        New.Get<FCk_RepData_2dGridPlacements>().Placements,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_2dGridPlacements>().Placements
                            : TArray<FCk_2dGridPlacement_ReplicatedEntry>{});
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} G2dGridOccupancy_RepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------

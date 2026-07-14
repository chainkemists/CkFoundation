#include "Ck2dGridOccupancy_Fragment.h"

#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment.h" // FCk_Fragment_2dGridPlacement_ParamsData + entry/RepData types (Produce)
#include "CkRecord/Record/CkRecord_Utils.h"                            // TUtils_RecordOfEntities::ForEach_ValidEntry (Produce)

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
                },
                // Produce-only capture (Phase 3A.4): mirror FProcessor_2dGridOccupancy_Replicate's live-state build
                // off the grid's placement record. NO SeedContainer — the live ReplicateOnRestore still seeds under
                // Model A (double-seed guard). Keyed on the grid entity (which holds the placement record).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_RecordOf_GridPlacements>())
                    { return {}; }

                    auto Entries = TArray<FCk_2dGridPlacement_ReplicatedEntry>{};

                    ck::RecordOf_GridPlacements_Utils::ForEach_ValidEntry(Entity,
                    [&](FCk_Handle_2dGridPlacement InPlacement)
                    {
                        const auto& Params = InPlacement.Get<FCk_Fragment_2dGridPlacement_ParamsData>();

                        Entries.Emplace(FCk_2dGridPlacement_ReplicatedEntry(Params.Get_Occupant())
                            .Set_Anchor(Params.Get_Anchor())
                            .Set_Rotation(Params.Get_Rotation())
                            .Set_Cells(Params.Get_Cells()));
                    });

                    auto Data = FCk_RepData_2dGridPlacements{};
                    Data.Placements = MoveTemp(Entries);
                    return FInstancedStruct::Make(Data);
                },
                .Transport = ECk_PersistenceTransport::NetAndSave
            });
    }
} G2dGridOccupancy_RepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------

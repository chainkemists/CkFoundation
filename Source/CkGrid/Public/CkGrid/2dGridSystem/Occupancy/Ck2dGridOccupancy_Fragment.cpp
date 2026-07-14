#include "Ck2dGridOccupancy_Fragment.h"

#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment.h" // FCk_Fragment_2dGridPlacement_ParamsData + entry/RepData types (Produce)
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"            // UCk_Utils_2dGridSystem_UE::Has/Cast (load hydration)
#include "Ck2dGridOccupancy_Utils.h"                                  // UCk_Utils_2dGridOccupancy_UE::Request_AddPlacement (load hydration)
#include "CkRecord/Record/CkRecord_Utils.h"                            // TUtils_RecordOfEntities::ForEach_ValidEntry (Produce)

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"     // RegisterLazyTyped
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // Register_* entry-point bodies

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

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_2dGridPlacements>(
                // Produce-only capture (Phase 3A.4): mirror FProcessor_2dGridOccupancy_Replicate's live-state build
                // off the grid's placement record. Produce is capture-only. Keyed on the grid entity (which holds
                // the placement record).
                [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
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
                // Client net path stamps the sync fragment consumed by the ClientOnly Occupancy SyncReplication
                // processor (which owns rebuild + reconcile). The authority load path takes HydrationApply instead —
                // that processor never runs on the host, so nothing would rebuild the record.
                [DoApplyPlacements](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    const auto& NewPlacements = New.Get<FCk_RepData_2dGridPlacements>().Placements;

                    DoApplyPlacements(Entity,
                        NewPlacements,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_2dGridPlacements>().Placements
                            : TArray<FCk_2dGridPlacement_ReplicatedEntry>{});
                    return ECk_Persistence_ApplyResult::Applied;
                },
                // Load-path authority hydration (mirrors CkInventory [F1-D6]): the payload is keyed on the GRID
                // entity; on the authority host the ClientOnly SyncReplication processor never runs, so re-drive
                // the placement record directly. Request_AddPlacement re-arms MayRequireReplication, so the
                // AuthorityOnly Replicate pass pushes the rebuilt set and clients converge via the ordinary
                // SyncReplication path (no explicit re-arm needed).
                [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT UCk_Utils_2dGridSystem_UE::Has(Entity))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& NewPlacements = New.Get<FCk_RepData_2dGridPlacements>().Placements;
                    auto Grid = UCk_Utils_2dGridSystem_UE::Cast(Entity);
                    for (const auto& Entry : NewPlacements)
                    {
                        // Do NOT gate on occupant validity: a restored placement's occupant is deterministically
                        // invalid (unlabeled ConstructSpawned child → save-transient → entt::null on load), and
                        // Request_AddPlacement adds the placement + Cells regardless (Cells are authoritative; an
                        // invalid occupant only skips the death-watch). A NotReady gate would never flip and would
                        // drop the whole payload past the dispatcher timeout.
                        UCk_Utils_2dGridOccupancy_UE::Request_AddPlacement(
                            Grid, Entry.Get_Occupant(), Entry.Get_Anchor(), Entry.Get_Rotation(), Entry.Get_Cells());
                    }
                    return ECk_Persistence_ApplyResult::Applied;
                });
    }
} G2dGridOccupancy_RepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------

#include "Ck2dGridOccupancy_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

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
                .OnChange = [DoApplyPlacements](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    DoApplyPlacements(Entity, New.Get<FCk_RepData_2dGridPlacements>().Placements, Old.Get<FCk_RepData_2dGridPlacements>().Placements);
                },
                .OnAdd = [DoApplyPlacements](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyPlacements(Entity, Data.Get<FCk_RepData_2dGridPlacements>().Placements, TArray<FCk_2dGridPlacement_ReplicatedEntry>{});
                }
            });
    }
} G2dGridOccupancy_RepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------

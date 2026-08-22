#include "CkJoltCook_Types.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Macros/CkMacros.h"

#include <Containers/Map.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    auto
        ComputeIncrementalPlan(
            const FCk_Jolt_IncrementalPlanInput& InInput)
        -> FCk_Jolt_IncrementalPlan
    {
        auto Plan = FCk_Jolt_IncrementalPlan{};

        auto CookedByName = TMap<FName, const FCk_Jolt_IncrementalCookedActor*>{};
        CookedByName.Reserve(InInput._Cooked.Num());
        ck::algo::ForEach(InInput._Cooked, [&](const FCk_Jolt_IncrementalCookedActor& InCooked)
        {
            CookedByName.Add(InCooked._ActorName, &InCooked);
        });

        const auto PresentNames = ck::algo::Transform<TSet<FName>>(InInput._Present,
            [](const FCk_Jolt_IncrementalPresentActor& InPresent) { return InPresent._ActorName; });

        for (const auto& PresentActor : InInput._Present)
        {
            const auto* const* CookedEntry = CookedByName.Find(PresentActor._ActorName);

            if (CookedEntry == nullptr)
            {
                if (NOT PresentActor._HasBodies)
                { continue; }

                ++Plan._NumAddedActors;
                Plan._DirtyCellIds.Add(PresentActor._CurrentCellId);
                continue;
            }

            const auto& CookedActor = **CookedEntry;

            if (CookedActor._SourceHash == PresentActor._SourceHash)
            {
                ++Plan._NumUnchangedActors;
                continue;
            }

            ++Plan._NumChangedActors;
            Plan._DirtyCellIds.Add(CookedActor._CellId);

            if (PresentActor._HasBodies)
            { Plan._DirtyCellIds.Add(PresentActor._CurrentCellId); }
            else
            { Plan._RemovedActorNames.Add(PresentActor._ActorName); }
        }

        for (const auto& CookedActor : InInput._Cooked)
        {
            if (PresentNames.Contains(CookedActor._ActorName))
            { continue; }

            const auto OwningLevelIsLoaded = InInput._LoadedLevelPackages.Contains(CookedActor._OwningLevelPackage);

            if (NOT OwningLevelIsLoaded)
            {
                ++Plan._NumPreservedUnloadedActors;
                continue;
            }

            Plan._RemovedActorNames.Add(CookedActor._ActorName);
            Plan._DirtyCellIds.Add(CookedActor._CellId);
        }

        return Plan;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ComputeIndexRemap(
            const FCk_Jolt_IndexRemapInput& InInput)
        -> FCk_Jolt_IndexRemap
    {
        auto Remap = FCk_Jolt_IndexRemap{};

        Remap._NewCellIndexByOldCellIndex.Init(INDEX_NONE, InInput._ExistingCellIdsByCellIndex.Num());

        auto NextNewCellIndex = 0;

        for (auto OldCellIndex = 0; OldCellIndex < InInput._ExistingCellIdsByCellIndex.Num(); ++OldCellIndex)
        {
            if (InInput._DirtyCellIds.Contains(InInput._ExistingCellIdsByCellIndex[OldCellIndex]))
            { continue; }

            Remap._NewCellIndexByOldCellIndex[OldCellIndex] = NextNewCellIndex;
            ++NextNewCellIndex;
        }

        ck::algo::ForEach(InInput._WrittenCellIds, [&](const FIntPoint& InWrittenCellId)
        {
            Remap._NewCellIndexByWrittenCellId.Add(InWrittenCellId, NextNewCellIndex);
            ++NextNewCellIndex;
        });

        Remap._NumNewCells = NextNewCellIndex;

        for (const auto& [ActorName, ActorRef] : InInput._ExistingActorLookup)
        {
            if (NOT Remap._NewCellIndexByOldCellIndex.IsValidIndex(ActorRef.Get_CellIndex()))
            { continue; }

            const auto NewCellIndex = Remap._NewCellIndexByOldCellIndex[ActorRef.Get_CellIndex()];
            const auto CellWasRewritten = NewCellIndex == INDEX_NONE;

            if (CellWasRewritten)
            { continue; }

            Remap._ActorLookup.Add(ActorName,
                FCk_Jolt_CookedActorRef{}.Set_CellIndex(NewCellIndex).Set_GroupIndex(ActorRef.Get_GroupIndex()));
        }

        for (const auto& [WrittenCellId, ActorNames] : InInput._WrittenActorNamesByCell)
        {
            const auto* NewCellIndex = Remap._NewCellIndexByWrittenCellId.Find(WrittenCellId);

            if (NewCellIndex == nullptr)
            { continue; }

            for (auto GroupIndex = 0; GroupIndex < ActorNames.Num(); ++GroupIndex)
            {
                Remap._ActorLookup.Add(ActorNames[GroupIndex],
                    FCk_Jolt_CookedActorRef{}.Set_CellIndex(*NewCellIndex).Set_GroupIndex(GroupIndex));
            }
        }

        return Remap;
    }
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkJoltCook_Types.h"

#include "CkJoltCook_MapSelection.h"

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

        const auto Get_CookedKey = [](const FCk_Jolt_IncrementalCookedActor& InCooked)
        { return FCk_Jolt_CookedActorKey{InCooked._OwningLevelPackage, InCooked._ActorName}; };

        const auto Get_PresentKey = [](const FCk_Jolt_IncrementalPresentActor& InPresent)
        { return FCk_Jolt_CookedActorKey{InPresent._OwningLevelPackage, InPresent._ActorName}; };

        const auto IsExcluded = [&](const FName& InLevelPackage)
        { return Get_IsPackageExcluded(InLevelPackage.ToString(), InInput._ExcludedLevelPackagePaths); };

        auto CookedByKey = TMap<FCk_Jolt_CookedActorKey, const FCk_Jolt_IncrementalCookedActor*>{};
        CookedByKey.Reserve(InInput._Cooked.Num());
        ck::algo::ForEach(InInput._Cooked, [&](const FCk_Jolt_IncrementalCookedActor& InCooked)
        {
            CookedByKey.Add(Get_CookedKey(InCooked), &InCooked);
        });

        auto PresentKeys = TSet<FCk_Jolt_CookedActorKey>{};
        for (const auto& PresentActor : InInput._Present)
        {
            if (NOT IsExcluded(PresentActor._OwningLevelPackage))
            { PresentKeys.Add(Get_PresentKey(PresentActor)); }
        }

        for (const auto& PresentActor : InInput._Present)
        {
            if (IsExcluded(PresentActor._OwningLevelPackage))
            { continue; }

            const auto* const* CookedEntry = CookedByKey.Find(Get_PresentKey(PresentActor));

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
            { Plan._RemovedActorKeys.Add(Get_PresentKey(PresentActor)); }
        }

        for (const auto& CookedActor : InInput._Cooked)
        {
            if (IsExcluded(CookedActor._OwningLevelPackage))
            {
                Plan._RemovedActorKeys.Add(Get_CookedKey(CookedActor));
                Plan._DirtyCellIds.Add(CookedActor._CellId);
                continue;
            }

            if (PresentKeys.Contains(Get_CookedKey(CookedActor)))
            { continue; }

            const auto OwningLevelIsLoaded = InInput._LoadedLevelPackages.Contains(CookedActor._OwningLevelPackage);

            if (NOT OwningLevelIsLoaded)
            {
                ++Plan._NumPreservedUnloadedActors;
                continue;
            }

            Plan._RemovedActorKeys.Add(Get_CookedKey(CookedActor));
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

        for (const auto& [LevelPackage, ActorsInLevel] : InInput._ExistingActorLookup)
        {
            for (const auto& [ActorName, ActorRef] : ActorsInLevel.Get_ActorsByName())
            {
                if (NOT Remap._NewCellIndexByOldCellIndex.IsValidIndex(ActorRef.Get_CellIndex()))
                { continue; }

                const auto NewCellIndex = Remap._NewCellIndexByOldCellIndex[ActorRef.Get_CellIndex()];
                const auto CellWasRewritten = NewCellIndex == INDEX_NONE;

                if (CellWasRewritten)
                { continue; }

                Remap._ActorLookup.FindOrAdd(LevelPackage).Get_ActorsByName().Add(ActorName,
                    FCk_Jolt_CookedActorRef{}.Set_CellIndex(NewCellIndex).Set_GroupIndex(ActorRef.Get_GroupIndex()));
            }
        }

        for (const auto& [WrittenCellId, ActorKeys] : InInput._WrittenActorKeysByCell)
        {
            const auto* NewCellIndex = Remap._NewCellIndexByWrittenCellId.Find(WrittenCellId);

            if (NewCellIndex == nullptr)
            { continue; }

            for (auto GroupIndex = 0; GroupIndex < ActorKeys.Num(); ++GroupIndex)
            {
                const auto& ActorKey = ActorKeys[GroupIndex];

                Remap._ActorLookup.FindOrAdd(ActorKey._LevelPackage).Get_ActorsByName().Add(ActorKey._ActorName,
                    FCk_Jolt_CookedActorRef{}.Set_CellIndex(*NewCellIndex).Set_GroupIndex(GroupIndex));
            }
        }

        return Remap;
    }
}

// --------------------------------------------------------------------------------------------------------------------

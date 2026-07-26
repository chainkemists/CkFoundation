#include "Ck2dGridSystem_EntityScript.h"

#include "Ck2dGridSystem_Spec.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"
#include "CkGrid/2dGridSystem/Blocker/Ck2dGridBlocker_Utils.h"
#include "CkGrid/2dGridSystem/Blocker/Ck2dGridBlocker_Fragment_Data.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkGrid/CkGrid_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Flow = Super::Construct(InHandle, InSpawnParams);

    if (ck::Is_NOT_Valid(Spec))
    {
        ck::grid::Warning(TEXT("UCk_2dGridSystem_EntityScript constructed with no Spec assigned; no grid built."));
        return Flow;
    }

    // ---- Grid -----------------------------------------------------------------------------------
    auto Transform = UCk_Utils_Transform_UE::Add(InHandle, SpawnTransform, ECk_Replication::DoesNotReplicate);
    auto Grid = UCk_Utils_2dGridSystem_UE::Add(Transform, Spec->Resolve_GridParams());

    // ---- Per-cell tags --------------------------------------------------------------------------
    for (const auto& Pair : Spec->PerCellTags)
    {
        auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(Grid, Pair.Key);
        if (ck::Is_NOT_Valid(Cell))
        {
            ck::grid::Warning(TEXT("PerCellTags references coordinate [{}] with no matching cell; skipping."),
                Pair.Key.ToString());
            continue;
        }

        for (const auto& Tag : Pair.Value)
        {
            UCk_Utils_2dGridCell_UE::Request_AddTag(Cell, Tag);
        }
    }

    // ---- Blockers -------------------------------------------------------------------------------
    for (const auto& Blocker : Spec->Blockers)
    {
        // Owned by the grid entity, so destroying the grid releases its blocks.
        auto BlockerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

        auto BlockerParams = FCk_Fragment_2dGridBlocker_ParamsData{Grid, Blocker.RangeMin, Blocker.RangeMax};
        BlockerParams.Set_Name(Blocker.Name);

        UCk_Utils_2dGridBlocker_UE::Add(BlockerEntity, BlockerParams);
    }

    return Flow;
}

// --------------------------------------------------------------------------------------------------------------------

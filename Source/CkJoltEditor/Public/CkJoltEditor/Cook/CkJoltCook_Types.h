#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"

#include <Containers/Map.h>
#include <Containers/Set.h>
#include <Math/IntPoint.h>
#include <UObject/NameTypes.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    enum class ECk_Jolt_CookMode : uint8
    {
        Cook,
        DryRun
    };

    // ----------------------------------------------------------------------------------------------------------------

    enum class ECk_Jolt_MeshShapeCookResult : uint8
    {
        Cooked,
        UpToDate,
        NotWorthPreBaking,
        Failed
    };

    // ----------------------------------------------------------------------------------------------------------------

    enum class ECk_Jolt_IncrementalOutcome : uint8
    {
        Incremental,
        FullCook_NoExistingIndex,
        FullCook_ContractDrift,
        FullCook_WorldPartition
    };

    enum class ECk_Jolt_CookStepResult : uint8
    {
        InProgress,
        Done,
        Failed,
        /// The incremental path declined; the caller must run a full cook instead.
        FullCookRequired
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKJOLTEDITOR_API FCk_Jolt_IncrementalCookedActor
    {
        FName _ActorName;
        uint64 _SourceHash = 0;
        FIntPoint _CellId = FIntPoint::ZeroValue;
        FName _OwningLevelPackage;
    };

    struct CKJOLTEDITOR_API FCk_Jolt_IncrementalPresentActor
    {
        FName _ActorName;
        // Half of the actor's identity — see FCk_Jolt_CookedActorKey. Matching a present actor to a
        // cooked one by NAME alone pairs actors across sublevels and rewrites the wrong cell.
        FName _OwningLevelPackage;
        uint64 _SourceHash = 0;
        FIntPoint _CurrentCellId = FIntPoint::ZeroValue;
        bool _HasBodies = false;
    };

    struct CKJOLTEDITOR_API FCk_Jolt_IncrementalPlanInput
    {
        TArray<FCk_Jolt_IncrementalCookedActor> _Cooked;
        TArray<FCk_Jolt_IncrementalPresentActor> _Present;
        TSet<FName> _LoadedLevelPackages;
    };

    struct CKJOLTEDITOR_API FCk_Jolt_IncrementalPlan
    {
        TSet<FIntPoint> _DirtyCellIds;
        TSet<FCk_Jolt_CookedActorKey> _RemovedActorKeys;
        int32 _NumChangedActors = 0;
        int32 _NumAddedActors = 0;
        int32 _NumUnchangedActors = 0;
        int32 _NumPreservedUnloadedActors = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /// Which bake-grid cells must be rewritten. Two rules a caller cannot infer from the signature:
    /// an actor that crossed a cell boundary dirties BOTH cells, and a cooked actor missing from
    /// _Present is deleted only when _LoadedLevelPackages holds its level — otherwise its level is
    /// merely unloaded, and dropping it would leave those actors with no collision at all.
    CKJOLTEDITOR_API auto
        ComputeIncrementalPlan(
            const FCk_Jolt_IncrementalPlanInput& InInput)
        -> FCk_Jolt_IncrementalPlan;

    // ----------------------------------------------------------------------------------------------------------------

    struct CKJOLTEDITOR_API FCk_Jolt_IndexRemapInput
    {
        TArray<FIntPoint> _ExistingCellIdsByCellIndex;
        TSet<FIntPoint> _DirtyCellIds;
        TArray<FIntPoint> _WrittenCellIds;
        TMap<FName, FCk_Jolt_CookedActorsInLevel> _ExistingActorLookup;
        TMap<FIntPoint, TArray<FCk_Jolt_CookedActorKey>> _WrittenActorKeysByCell;
    };

    struct CKJOLTEDITOR_API FCk_Jolt_IndexRemap
    {
        TArray<int32> _NewCellIndexByOldCellIndex;
        TMap<FIntPoint, int32> _NewCellIndexByWrittenCellId;
        TMap<FName, FCk_Jolt_CookedActorsInLevel> _ActorLookup;
        int32 _NumNewCells = 0;
    };

    /// Renumbers cells and rebuilds the actor lookup after some cells were rewritten. Every index
    /// below _NumNewCells is claimed by exactly one of the two index maps, so a caller SIZES its
    /// array to _NumNewCells and PLACES each ref at its computed index — appending instead would let
    /// the order drift from the lookup and hand actors someone else's collision.
    CKJOLTEDITOR_API auto
        ComputeIndexRemap(
            const FCk_Jolt_IndexRemapInput& InInput)
        -> FCk_Jolt_IndexRemap;
}

// --------------------------------------------------------------------------------------------------------------------

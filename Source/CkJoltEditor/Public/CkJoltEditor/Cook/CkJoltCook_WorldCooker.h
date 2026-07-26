#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

/// The shared static-world cooker, consumed by UCk_JoltCook_EditorSubsystem_UE (primary) and
/// UCk_JoltCook_Commandlet (headless). Pipeline and invariants: CkJoltEditor/CLAUDE.md.
class CKJOLTEDITOR_API FCk_Jolt_WorldCooker
{
public:
    CK_GENERATED_BODY(FCk_Jolt_WorldCooker);

    struct FCookStats
    {
        int32 _NumActors = 0;
        int32 _NumBodies = 0;
        int32 _NumCells = 0;
        int32 _NumUniqueShapes = 0;
        int32 _NumActorsUpToDate = 0;
        bool _Success = false;
    };

public:
    /// On World Partition maps, unloaded actors are visited in batches via
    /// FWorldPartitionHelpers::ForEachActorWithLoading. DryRun extracts + reports, saving nothing.
    static auto
    Cook_World(
        UWorld& InWorld,
        bool InDryRun = false) -> FCookStats;

    /// Reports stale/missing/up-to-date counts against the existing cooked index; writes nothing.
    static auto
    Validate_World(
        UWorld& InWorld) -> FCookStats;
};

// --------------------------------------------------------------------------------------------------------------------

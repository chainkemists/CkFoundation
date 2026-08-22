#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include <Templates/UniquePtr.h>

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include "CkJoltEditor/Cook/CkJoltCook_Types.h"

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
        int32 _NumCellsWritten = 0;
        ck::jolt::cook::ECk_Jolt_IncrementalOutcome _Outcome =
            ck::jolt::cook::ECk_Jolt_IncrementalOutcome::Incremental;
        bool _Success = false;
    };

public:
    /// Rebuilds the map's cooked data from scratch: every cell asset and the index are rewritten
    /// from whatever the world currently holds. On World Partition maps, unloaded actors are visited
    /// in batches via FWorldPartitionHelpers::ForEachActorWithLoading. DryRun extracts + reports,
    /// saving nothing.
    ///
    /// The index it writes describes ONLY the levels loaded at the time of the cook — on a
    /// streaming-sublevel map, running this with sublevels unloaded drops their actors from the
    /// index entirely, and a dropped actor gets NO static collision at runtime (a missing lookup
    /// entry is not an error, it is "this actor was never baked"). Prefer Cook_World_Incremental,
    /// which cannot truncate.
    static auto
    Cook_World(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode = ck::jolt::cook::ECk_Jolt_CookMode::Cook) -> FCookStats;

    /// Rewrites only the bake-grid cells whose actors actually changed since the last cook, leaving
    /// every other cell asset and every cooked actor in an UNLOADED level untouched. Falls back to a
    /// full Cook_World — and says so in _Outcome — when there is no existing index, when the index
    /// was cooked under a different cook/Jolt version or bake filter, or on a World Partition world
    /// (whose actors cannot be revisited after the streaming walk releases them).
    static auto
    Cook_World_Incremental(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode = ck::jolt::cook::ECk_Jolt_CookMode::Cook) -> FCookStats;

    /// Reports stale/missing/up-to-date counts against the existing cooked index; writes nothing.
    static auto
    Validate_World(
        UWorld& InWorld) -> FCookStats;
};

// --------------------------------------------------------------------------------------------------------------------

/// The incremental cook as a resumable stepper, so the editor stays interactive while it runs.
/// Cook_World_Incremental is this driven to completion in one call; the editor subsystem drives it a
/// time-slice per frame instead. Holds the Jolt globals and roots the assets it loaded for its whole
/// lifetime, because the shapes it carries between steps do not survive either being released.
///
/// The world may be edited between steps. That produces a cook of the state at sweep time, never a
/// corrupt one, and the edit dirties the actor's hash again so the next cook picks it up.
class CKJOLTEDITOR_API FCk_Jolt_IncrementalCookDriver
{
public:
    CK_GENERATED_BODY(FCk_Jolt_IncrementalCookDriver);

    FCk_Jolt_IncrementalCookDriver(
        UWorld& InWorld,
        ck::jolt::cook::ECk_Jolt_CookMode InMode);

    ~FCk_Jolt_IncrementalCookDriver();

    FCk_Jolt_IncrementalCookDriver(const FCk_Jolt_IncrementalCookDriver&) = delete;
    auto operator=(const FCk_Jolt_IncrementalCookDriver&) -> FCk_Jolt_IncrementalCookDriver& = delete;

public:
    /// Advances until InBudget is spent or the current phase ends, whichever comes first. A zero
    /// budget still advances one unit, so a caller cannot livelock on a budget it set too low.
    auto
    Step(
        FCk_Time InBudget) -> ck::jolt::cook::ECk_Jolt_CookStepResult;

    auto Get_CompletedUnits() const -> int32;
    auto Get_TotalUnits() const -> int32;
    auto Get_Stats() const -> FCk_Jolt_WorldCooker::FCookStats;

private:
    struct FImpl;
    TUniquePtr<FImpl> _Impl;
};

// --------------------------------------------------------------------------------------------------------------------

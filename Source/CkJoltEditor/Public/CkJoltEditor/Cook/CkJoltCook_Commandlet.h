#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJoltEditor/Cook/CkJoltCook_Types.h"

#include <Commandlets/Commandlet.h>
#include <Containers/Array.h>

#include "CkJoltCook_Commandlet.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * Headless Jolt static-world cook across maps — no PIE required (fixes UnrealJolt's
 * "must PIE every level once" limitation).
 *
 * Invocation (the FULL class token is required — UE appends "Commandlet" to a bare -run= token,
 * and "CkJoltCookCommandlet" matches no class; on unique-build-environment projects use the
 * project's own <Target>-Cmd.exe, not the engine's UnrealEditor-Cmd.exe):
 *   <Target>-Cmd.exe <Project>.uproject -run=Ck_JoltCook_Commandlet -Map=/Game/Maps/MyMap
 *   <Target>-Cmd.exe <Project>.uproject -run=Ck_JoltCook_Commandlet -AllMaps [-Root=/Game] [-DryRun]
 *   <Target>-Cmd.exe <Project>.uproject -run=Ck_JoltCook_Commandlet -PackagingMaps [-DryRun]
 *     Unions ProjectPackagingSettings MapsToCook with UWorld metadata under DirectoriesToAlwaysCook;
 *     exclusions skip either source. It rejects bCookAll and -Map/-AllMaps combinations before any cook begins.
 *   Add -Incremental for actor-hash freshness checks and dirty-cell rebuilding. Missing/incompatible
 *   indexes and World Partition maps fall back to full rebuilding. Add -ForceRebuild to rebuild maps
 *   and mesh shapes unconditionally. These two switches are mutually exclusive.
 *
 * Exit codes: 0 = success; 1 = rejected selection, missing entry map, a mesh-shape failure, or any
 * map-load/cook failure. Each failure is logged before the commandlet exits.
 *
 * NOTE: the editor-subsystem path (UCk_JoltCook_EditorSubsystem_UE — world already booted by
 * the editor) is the PRIMARY cook vehicle. This commandlet boots worlds itself; if a map's
 * world-boot proves flaky here (WP initialization, landscape editor data), the documented
 * pivot is a UWorldPartitionBuilder subclass under -run=WorldPartitionBuilderCommandlet
 * (engine-managed loading, precedented by the nav/HLOD builders).
 */
UCLASS()
class CKJOLTEDITOR_API UCk_JoltCook_Commandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_JoltCook_Commandlet);

public:
    auto
    Main(
        const FString& InParams) -> int32 override;

private:
    /// Brings the map's streaming sublevels into the world so the sweep can see their actors.
    /// Excluded packaging-only sublevels may have been loaded by the editor loader, but they are
    /// never required, attached, or allowed to contribute collision output. False = an eligible
    /// sublevel did not load and the map MUST NOT be cooked.
    auto
    DoEnsure_StreamingLevelsInWorld(
        UWorld& InWorld,
        const TArray<FString>& InExcludedLevelPackagePaths) -> bool;

    auto
    DoCook_Map(
        const FString& InMapPackageName,
        ck::jolt::cook::ECk_Jolt_CookMode InMode,
        const TArray<FString>& InExcludedLevelPackagePaths,
        bool InIncremental) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

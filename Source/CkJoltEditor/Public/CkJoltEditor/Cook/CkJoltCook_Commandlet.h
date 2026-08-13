#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Commandlets/Commandlet.h>

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
 *
 * Exit codes: 0 = success, 1 = any map failed to cook.
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
    auto
    DoCook_Map(
        const FString& InMapPackageName,
        bool InDryRun) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

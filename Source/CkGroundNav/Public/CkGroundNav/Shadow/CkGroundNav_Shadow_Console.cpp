#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Utils.h"

#include <Engine/World.h>
#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_shadow_console
{
    // One log call per line rather than one per report: the log truncates long messages, and every
    // consumer of this output reads it a line at a time anyway.
    auto DoLogReport(
        const FString& InReport) -> void
    {
        constexpr auto CullEmptyLines = false;

        auto Lines = TArray<FString>{};
        InReport.ParseIntoArrayLines(Lines, CullEmptyLines);

        for (const auto& Line : Lines)
        { ck::groundnav::Display(TEXT("{}"), Line); }
    }

    static FAutoConsoleCommandWithWorld ConsoleCommand_ShadowReport(
        TEXT("ck.GroundNav.ShadowReport"),
        TEXT("Print the shadow-mode comparison report for this world: one row per fixture with the ")
        TEXT("outcome counters, the status-pair matrix and the length, endpoint, waypoint-count and ")
        TEXT("query-time distributions. Rows are sorted and fixed-width, so two runs diff cleanly. ")
        TEXT("A run that never opened a fixture with Request_BeginShadowFixture buckets everything ")
        TEXT("under the map name, so the whole session reads as one row."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.ShadowReport ran without a World"))
            { return; }

            DoLogReport(UCk_Utils_GroundNav_Shadow_UE::Get_ShadowReport(InWorld));
        }));
}

// --------------------------------------------------------------------------------------------------------------------

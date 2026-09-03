#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_ground_nav_install_algorithm
{
    // What the install boundary does with one ground-path status. Defer is neither an install nor a
    // failure: the shared nav slot keeps the Pending its dispatch stamped and the crowd's pending
    // watchdog remains the only bound on it, so a status that may still become an answer never
    // spends the episode.
    enum class ECk_CrowdAgent_GroundNavInstallAction : uint8
    {
        Install,
        Fail,
        Defer
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One row of the total map from CkGroundNav's own statuses onto the install vocabulary.
     *
     * _Status is carried in the row rather than implied by its index so the table can prove it is
     * still in enum order after somebody inserts a status — see the identity check below.
     * _InstallAs is meaningful for Install and Defer; _Reason for Fail.
     */
    struct FCk_CrowdAgent_GroundNavVerdict
    {
        ECk_GroundNav_PathStatus              _Status;
        ECk_CrowdAgent_GroundNavInstallAction _Action;
        ECk_Nav_PathStatus                    _InstallAs;
        ECk_Nav_PathFailReason                _Reason;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // In ECk_GroundNav_PathStatus declaration order, which the two asserts below enforce.
    inline constexpr FCk_CrowdAgent_GroundNavVerdict kGroundNavVerdicts[] =
    {
        {ECk_GroundNav_PathStatus::InProgress,     ECk_CrowdAgent_GroundNavInstallAction::Defer,
            ECk_Nav_PathStatus::Pending, ECk_Nav_PathFailReason::None},

        {ECk_GroundNav_PathStatus::Ready,          ECk_CrowdAgent_GroundNavInstallAction::Install,
            ECk_Nav_PathStatus::Ready,   ECk_Nav_PathFailReason::None},

        // Recast parity: the crowd reacts to a route that ends short of the goal, and flattening it
        // to Ready would silence the strict-phase ends-short retry on GroundNav alone.
        {ECk_GroundNav_PathStatus::Partial,        ECk_CrowdAgent_GroundNavInstallAction::Install,
            ECk_Nav_PathStatus::Partial, ECk_Nav_PathFailReason::None},

        // The path feature already waited its deferral window for the ground to bake before it
        // published Unbuilt, so what arrives here is the timeout CkNavigation reports as NoNavData.
        {ECk_GroundNav_PathStatus::Unbuilt,        ECk_CrowdAgent_GroundNavInstallAction::Fail,
            ECk_Nav_PathStatus::Failed,  ECk_Nav_PathFailReason::NoNavData},

        {ECk_GroundNav_PathStatus::NoStartSurface, ECk_CrowdAgent_GroundNavInstallAction::Fail,
            ECk_Nav_PathStatus::Failed,  ECk_Nav_PathFailReason::StartProjectFailed},

        {ECk_GroundNav_PathStatus::NoGoalSurface,  ECk_CrowdAgent_GroundNavInstallAction::Fail,
            ECk_Nav_PathStatus::Failed,  ECk_Nav_PathFailReason::EndProjectFailed},

        // FindPathNoPath is what the crowd reads as a planning verdict, so a strict pass that finds
        // no route retries permissive and only then fails the goal — the Recast sequence exactly.
        {ECk_GroundNav_PathStatus::Unreachable,    ECk_CrowdAgent_GroundNavInstallAction::Fail,
            ECk_Nav_PathStatus::Failed,  ECk_Nav_PathFailReason::FindPathNoPath},

        {ECk_GroundNav_PathStatus::BudgetExceeded, ECk_CrowdAgent_GroundNavInstallAction::Fail,
            ECk_Nav_PathStatus::Failed,  ECk_Nav_PathFailReason::BudgetExceeded},

        // A body wider than the field's clearance ceiling is degenerate input, which is what
        // FindPathInvalid names.
        {ECk_GroundNav_PathStatus::Blocked,        ECk_CrowdAgent_GroundNavInstallAction::Fail,
            ECk_Nav_PathStatus::Failed,  ECk_Nav_PathFailReason::FindPathInvalid}
    };

    // A switch without a default only warns when a status is added, which is not enough for a map
    // that must stay total. An APPENDED status trips the count; an INSERTED one trips the identity.
    static_assert(static_cast<SIZE_T>(UE_ARRAY_COUNT(kGroundNavVerdicts)) ==
        static_cast<SIZE_T>(ECk_GroundNav_PathStatus::Blocked) + 1,
        "add the new ECk_GroundNav_PathStatus row to kGroundNavVerdicts");

    inline consteval auto DoVerify_VerdictTable() -> bool
    {
        for (auto Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(kGroundNavVerdicts)); ++Index)
        {
            if (kGroundNavVerdicts[Index]._Status != static_cast<ECk_GroundNav_PathStatus>(Index))
            { return false; }
        }
        return true;
    }

    static_assert(DoVerify_VerdictTable(),
        "kGroundNavVerdicts is no longer in ECk_GroundNav_PathStatus declaration order");

    // ----------------------------------------------------------------------------------------------------------------

    inline auto Get_GroundNavVerdict(
        ECk_GroundNav_PathStatus InStatus)
        -> const FCk_CrowdAgent_GroundNavVerdict&
    {
        const auto Index = static_cast<int32>(InStatus);
        const auto IndexIsInRange = Index >= 0
            && Index < static_cast<int32>(UE_ARRAY_COUNT(kGroundNavVerdicts));

        CK_ENSURE_IF_NOT(IndexIsInRange,
            TEXT("GroundNav path status [{}] is outside the verdict table"), InStatus)
        { return kGroundNavVerdicts[0]; }

        return kGroundNavVerdicts[Index];
    }
}

// --------------------------------------------------------------------------------------------------------------------

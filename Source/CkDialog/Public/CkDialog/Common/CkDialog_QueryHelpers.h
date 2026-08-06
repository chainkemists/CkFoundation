#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkDialog/Emitter/CkDialogEmitter_Fragment_Data.h"
#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

#include <GameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Stateless helpers shared by the emitter processor and the emitter utils; every method is a pure function of
    // its args.
    struct CKDIALOG_API FDialog_QueryHelpers
    {
        static auto
        Get_WorldTimeNow(
            UWorld* InWorld) -> FCk_Time;

        static auto
        Get_WorldTimeNow(
            const FCk_Handle& InHandle) -> FCk_Time;

        // A line is visible to the emitter iff its filter tags are empty (a global line) OR ANY-overlap the emitter
        // tags; and when the request carries extra filter tags, the line must ALSO overlap those. Invisible lines are
        // excluded from the result entirely (they are not "failed").
        static auto
        Get_IsLineVisible(
            const FGameplayTagContainer& InLineFilterTags,
            const FGameplayTagContainer& InEmitterTags,
            const FGameplayTagContainer& InExtraFilterTags) -> bool;

        static auto
        Get_ConditionResult(
            FCk_Handle InConditionEntity,
            const FCk_Handle_DialogLine& InLine,
            const FCk_Handle_DialogEmitter& InEmitter) -> ECk_Dialog_ConditionResult;

        static auto
        Make_QueryEntry(
            const FCk_Handle_DialogLine& InLine,
            ECk_DialogLine_QueryResult InResult) -> FCk_DialogLine_QueryEntry;

        static auto
        Get_ResolvedSortPolicy(
            const FCk_DialogEmitter_Spec& InParams,
            const FCk_Request_DialogEmitter_Query& InQuery) -> ECk_Dialog_QuerySortPolicy;

        static auto
        Sort_Entries(
            TArray<FCk_DialogLine_QueryEntry>& InEntries,
            ECk_Dialog_QuerySortPolicy InPolicy) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    // Comparators for FDialog_QueryHelpers::Sort_Entries; stable sort keeps registration order among equal counts.
    struct CKDIALOG_API ByDialogLineConditionCount_Ascending
    {
        auto operator()(const FCk_DialogLine_QueryEntry& InL, const FCk_DialogLine_QueryEntry& InR) const -> bool;
    };

    struct CKDIALOG_API ByDialogLineConditionCount_Descending
    {
        auto operator()(const FCk_DialogLine_QueryEntry& InL, const FCk_DialogLine_QueryEntry& InR) const -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------

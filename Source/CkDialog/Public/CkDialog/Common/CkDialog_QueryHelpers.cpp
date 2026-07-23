#include "CkDialog/Common/CkDialog_QueryHelpers.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkDialog/Condition/CkDialogCondition_EntityScript.h"
#include "CkDialog/Line/CkDialogLine_Utils.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FDialog_QueryHelpers::
        Get_WorldTimeNow(
            UWorld* InWorld)
        -> FCk_Time
    {
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{InWorld};
        return UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();
    }

    auto
        FDialog_QueryHelpers::
        Get_WorldTimeNow(
            const FCk_Handle& InHandle)
        -> FCk_Time
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return FCk_Time::ZeroSecond(); }

        return Get_WorldTimeNow(World);
    }

    auto
        FDialog_QueryHelpers::
        Get_IsLineVisible(
            const FGameplayTagContainer& InLineFilterTags,
            const FGameplayTagContainer& InEmitterTags,
            const FGameplayTagContainer& InExtraFilterTags)
        -> bool
    {
        const auto PassesEmitter = InLineFilterTags.IsEmpty() || InLineFilterTags.HasAny(InEmitterTags);
        if (NOT PassesEmitter)
        { return false; }

        if (NOT InExtraFilterTags.IsEmpty() && NOT InLineFilterTags.HasAny(InExtraFilterTags))
        { return false; }

        return true;
    }

    auto
        FDialog_QueryHelpers::
        Get_ConditionResult(
            FCk_Handle InConditionEntity,
            const FCk_Handle_DialogLine& InLine,
            const FCk_Handle_DialogEmitter& InEmitter)
        -> ECk_Dialog_ConditionResult
    {
        const auto HasScript = InConditionEntity.Has<ck::FFragment_EntityScript_Current>();

        auto* Script = HasScript
            ? InConditionEntity.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get()
            : nullptr;
        auto* Condition = Cast<UCk_DialogCondition_EntityScript>(Script);

        // Load-bearing guard: the Evaluate() below dereferences Condition. CK_ENSURE_IF_NOT compiles to
        // `if constexpr(false)` under CK_DISABLE_ENSURE_CHECKS, so the recovery must live in a separate ordinary
        // branch — collapsing it into the macro body would null-deref in that build.
        const auto ConditionValid = ck::IsValid(Condition);
        CK_ENSURE_IF_NOT(ConditionValid,
            TEXT("Dialog condition entity [{}] has no valid UCk_DialogCondition_EntityScript — evaluated after the "
                 "registry reported ready. Treating as Fail (a line whose gate cannot be evaluated must not emit)."),
            InConditionEntity)
        {}
        if (NOT ConditionValid)
        { return ECk_Dialog_ConditionResult::Fail; }

        return Condition->Evaluate(InConditionEntity, InLine, InEmitter);
    }

    auto
        FDialog_QueryHelpers::
        Make_QueryEntry(
            const FCk_Handle_DialogLine& InLine,
            ECk_DialogLine_QueryResult InResult)
        -> FCk_DialogLine_QueryEntry
    {
        auto Entry = FCk_DialogLine_QueryEntry{InLine, InResult};
        Entry.Set_LineID(UCk_Utils_DialogLine_UE::Get_LineID(InLine));
        Entry.Set_Text(UCk_Utils_DialogLine_UE::Get_Text(InLine));
        Entry.Set_LinkedEventTag(UCk_Utils_DialogLine_UE::Get_LinkedEventTag(InLine));
        Entry.Set_NumConditions(UCk_Utils_DialogLine_UE::Get_NumConditions(InLine));
        return Entry;
    }

    auto
        FDialog_QueryHelpers::
        Get_ResolvedSortPolicy(
            const FCk_Fragment_DialogEmitter_ParamsData& InParams,
            const FCk_Request_DialogEmitter_Query& InQuery)
        -> ECk_Dialog_QuerySortPolicy
    {
        return InQuery.Get_SortPolicy() != ECk_Dialog_QuerySortPolicy::None
            ? InQuery.Get_SortPolicy()
            : InParams.Get_DefaultSortPolicy();
    }

    auto
        FDialog_QueryHelpers::
        Sort_Entries(
            TArray<FCk_DialogLine_QueryEntry>& InEntries,
            ECk_Dialog_QuerySortPolicy InPolicy)
        -> void
    {
        switch (InPolicy)
        {
            case ECk_Dialog_QuerySortPolicy::ConditionCountAscending:
            {
                InEntries.StableSort(ck::algo::ByDialogLineConditionCount_Ascending{});
                break;
            }
            case ECk_Dialog_QuerySortPolicy::ConditionCountDescending:
            {
                InEntries.StableSort(ck::algo::ByDialogLineConditionCount_Descending{});
                break;
            }
            case ECk_Dialog_QuerySortPolicy::None:
            default:
            { break; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        ByDialogLineConditionCount_Ascending::
        operator()(
            const FCk_DialogLine_QueryEntry& InL,
            const FCk_DialogLine_QueryEntry& InR) const
        -> bool
    {
        return InL.Get_NumConditions() < InR.Get_NumConditions();
    }

    auto
        ByDialogLineConditionCount_Descending::
        operator()(
            const FCk_DialogLine_QueryEntry& InL,
            const FCk_DialogLine_QueryEntry& InR) const
        -> bool
    {
        return InL.Get_NumConditions() > InR.Get_NumConditions();
    }
}

// --------------------------------------------------------------------------------------------------------------------

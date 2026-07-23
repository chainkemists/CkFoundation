#include "CkDialogLine_Utils.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "StructUtils/InstancedStruct.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_DialogLine_UE, FCk_Handle_DialogLine,
    ck::FFragment_DialogLine_Params, ck::FTag_DialogLine);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogLine_UE::
    Get_LineID(
        const FCk_Handle_DialogLine& InLine)
    -> FName
{
    return InLine.Get<ck::FFragment_DialogLine_Params>().Get_LineID();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_Text(
        const FCk_Handle_DialogLine& InLine)
    -> FText
{
    return InLine.Get<ck::FFragment_DialogLine_Params>().Get_Text();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_EventTag(
        const FCk_Handle_DialogLine& InLine)
    -> FGameplayTag
{
    return InLine.Get<ck::FFragment_DialogLine_Params>().Get_EventTag();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_LinkedEventTag(
        const FCk_Handle_DialogLine& InLine)
    -> FGameplayTag
{
    return InLine.Get<ck::FFragment_DialogLine_Params>().Get_LinkedEventTag();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_HasLinkedEventTag(
        const FCk_Handle_DialogLine& InLine)
    -> bool
{
    return Get_LinkedEventTag(InLine).IsValid();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_FilterTags(
        const FCk_Handle_DialogLine& InLine)
    -> FGameplayTagContainer
{
    return InLine.Get<ck::FFragment_DialogLine_Params>().Get_FilterTags();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_NumConditions(
        const FCk_Handle_DialogLine& InLine)
    -> int32
{
    return InLine.Get<ck::FFragment_DialogLine_Params>().Get_NumConditions();
}

auto
    UCk_Utils_DialogLine_UE::
    Get_Conditions(
        const FCk_Handle_DialogLine& InLine)
    -> TArray<FCk_Handle>
{
    auto Result = TArray<FCk_Handle>{};
    ForEach_Condition(InLine, [&](FCk_Handle InCondition)
    {
        Result.Emplace(InCondition);
    });
    return Result;
}

auto
    UCk_Utils_DialogLine_UE::
    ForEach_Condition(
        const FCk_Handle_DialogLine& InLine,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT RecordOfDialogConditions_Utils::Has(InLine))
    { return; }

    RecordOfDialogConditions_Utils::ForEach_ValidEntry(InLine, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DialogLine_UE::
    Create(
        const FCk_Handle& InRegistryRoot,
        const FGameplayTagContainer& InBankTags,
        const FCk_DialogBank_LineData& InLineData,
        const TFunction<void()>& InOnConditionConstructed,
        int32& OutNumConditionsQueued)
    -> FCk_Handle_DialogLine
{
    OutNumConditionsQueued = 0;

    auto MergedFilterTags = InBankTags;
    MergedFilterTags.AppendTags(InLineData.Get_ExtraTags());

    auto NumValidConditions = 0;
    for (const auto& ConditionArchetype : InLineData.Get_Conditions())
    {
        if (ck::IsValid(ConditionArchetype.Get()))
        { ++NumValidConditions; }
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InRegistryRoot, [&](FCk_Handle InNewEntity)
    {
        auto Params = FCk_Fragment_DialogLine_ParamsData{InLineData.Get_LineID(), InLineData.Get_EventTag()};
        Params.Set_Text(InLineData.Get_Text());
        Params.Set_LinkedEventTag(InLineData.Get_LinkedEventTag());
        Params.Set_FilterTags(MergedFilterTags);
        Params.Set_NumConditions(NumValidConditions);

        InNewEntity.Add<ck::FFragment_DialogLine_Params>(Params);
        InNewEntity.Add<ck::FTag_DialogLine>();

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        UCk_Utils_Handle_UE::Set_DebugName(InNewEntity,
            FName{*ck::Format_UE(TEXT("DialogLine: {}"), InLineData.Get_LineID())});
#endif

        // Register the ENTER tag into per-tag EnTT storage so the emitter query candidate-set finds this line.
        UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(InNewEntity, InLineData.Get_EventTag());
    });

    auto NewLine = ck::StaticCast<FCk_Handle_DialogLine>(NewEntity);

    if (NumValidConditions == 0)
    { return NewLine; }

    RecordOfDialogConditions_Utils::AddIfMissing(NewLine, ECk_Record_EntryHandlingPolicy::Default);

    for (const auto& ConditionArchetype : InLineData.Get_Conditions())
    {
        if (ck::Is_NOT_Valid(ConditionArchetype.Get()))
        { continue; }

        auto LineOwner = NewEntity;
        UCk_Utils_EntityScript_UE::Add(
            LineOwner,
            TWeakObjectPtr<UCk_EntityScript_UE>{ConditionArchetype.Get()},
            FInstancedStruct{},
            [NewLine, InOnConditionConstructed](FCk_Handle InConditionEntity) mutable
            {
                InConditionEntity.Add<ck::FTag_DialogCondition>();
                RecordOfDialogConditions_Utils::Request_Connect(NewLine, InConditionEntity,
                    ECk_Record_LabelRequirementPolicy::Optional);

                if (InOnConditionConstructed)
                { InOnConditionConstructed(); }
            });

        ++OutNumConditionsQueued;
    }

    return NewLine;
}

// --------------------------------------------------------------------------------------------------------------------

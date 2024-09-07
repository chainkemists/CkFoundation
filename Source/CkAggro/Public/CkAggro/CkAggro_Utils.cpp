#include "CkAggro_Utils.h"

#include "CkAggro/CkAggro_Fragment.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Handle& InTarget,
        const FCk_Fragment_Aggro_Params& InParams)
    -> FCk_Handle_Aggro
{
    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InTarget, [&](FCk_Handle InNewEntity)
    {
        InNewEntity.Add<ck::FTag_Aggro>();
        ck::UAggroedEntity_Utils::Add(InNewEntity, InTarget);
        UCk_Utils_FloatAttribute_UE::Add
        (
            InNewEntity,
            FCk_Fragment_FloatAttribute_ParamsData
            {
                TAG_Aggro_FloatAttribute_Name,
               InParams.Get_ScoreStartingValue()
            }.Set_MinMax(InParams.Get_MinMax())
            .Set_MinValue(InParams.Get_MinValue())
            .Set_MaxValue(InParams.Get_MaxValue())
        );

        UCk_Entity_ConstructionScript_PDA::Request_Construct(InNewEntity, InParams.Get_ConstructionScript(), {});
    });

    auto AggroHandle = Cast(NewEntity);

    RecordOfAggro_Utils::AddIfMissing(InHandle);
    RecordOfAggro_Utils::Request_Connect(InHandle, AggroHandle);

    ck::UUtils_Signal_OnNewAggroAdded::Broadcast(InHandle, ck::MakePayload(InHandle, AggroHandle));

    return AggroHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Aggro_UE, FCk_Handle_Aggro, ck::FTag_Aggro)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Aggro_UE::
    TryGet_AggroByTarget(
        const FCk_Handle& InAggroOwnerEntity,
        const FCk_Handle& InTarget)
    -> FCk_Handle_Aggro
{
    auto Ret = FCk_Handle_Aggro{};

    RecordOfAggro_Utils::ForEach_ValidEntry(InAggroOwnerEntity, [&](const FCk_Handle_Aggro& InAggro)
    {
        if (Get_AggroTarget(InAggro) == InTarget)
        {
            Ret = InAggro;
        }
    });

    return Ret;
}

auto
    UCk_Utils_Aggro_UE::
    Get_HighestAggro(
        const FCk_Handle& InAggroOwnerEntity)
    -> FCk_Handle_Aggro
{
    const auto& AllAggroEntities = RecordOfAggro_Utils::Get_ValidEntries(InAggroOwnerEntity);

    const auto& HighestAggro = ck::algo::MaxElement(AllAggroEntities, [&](const FCk_Handle_Aggro& InAggro) { return Get_AggroScore(InAggro); });

    if (ck::Is_NOT_Valid(HighestAggro))
    { return {}; }

    return *HighestAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Get_LowestAggro(
        const FCk_Handle& InAggroOwnerEntity)
    -> FCk_Handle_Aggro
{
    const auto& AllAggroEntities = RecordOfAggro_Utils::Get_ValidEntries(InAggroOwnerEntity);

    const auto& LowestAggro = ck::algo::MinElement(AllAggroEntities, [&](const FCk_Handle_Aggro& InAggro) { return Get_AggroScore(InAggro); });

    if (ck::Is_NOT_Valid(LowestAggro))
    { return {}; }

    return *LowestAggro;
}

auto
    UCk_Utils_Aggro_UE::
    Get_AggroTarget(
        const FCk_Handle_Aggro& InAggro)
    -> FCk_Handle
{
    return ck::UAggroedEntity_Utils::Get_StoredEntity(InAggro);
}

auto
    UCk_Utils_Aggro_UE::
    Get_AggroScoreAttribute(
        const FCk_Handle_Aggro& InAggro)
    -> FCk_Handle_FloatAttribute
{
    return UCk_Utils_FloatAttribute_UE::TryGet(InAggro, TAG_Aggro_FloatAttribute_Name);
}

auto
    UCk_Utils_Aggro_UE::
    Get_AggroScore(
        const FCk_Handle_Aggro& InAggro)
    -> float
{
    return UCk_Utils_FloatAttribute_UE::Get_FinalValue(Get_AggroScoreAttribute(InAggro));
}

auto
    UCk_Utils_Aggro_UE::
    BindTo_OnNewAggroAdded(
        FCk_Handle& InAggroOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnNewAggroAdded, InAggroOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InAggroOwner;
}

auto
    UCk_Utils_Aggro_UE::
    UnbindFrom_OnNewAggroAdded(
        FCk_Handle& InAggroOwner,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnNewAggroAdded, InAggroOwner, InDelegate);
    return InAggroOwner;
}

auto
    UCk_Utils_Aggro_UE::
    ForEach_Aggro(
        FCk_Handle& InAggroOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Aggro>
{
    auto Aggro = TArray<FCk_Handle_Aggro>{};

    ForEach_Aggro(InAggroOwnerEntity, [&](FCk_Handle_Aggro InAggro)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAggro, InOptionalPayload); }
        else
        { Aggro.Emplace(InAggro); }
    });

    return Aggro;
}

auto
    UCk_Utils_Aggro_UE::
    ForEach_Aggro(
        FCk_Handle& InAggroOwnerEntity,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc)
    -> void
{
    RecordOfAggro_Utils::ForEach_ValidEntry(InAggroOwnerEntity, InFunc);
}

auto
    UCk_Utils_Aggro_UE::
    ForEach_Aggro_Sorted(
        FCk_Handle& InAggroOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Aggro>
{
    auto Aggro = TArray<FCk_Handle_Aggro>{};

    ForEach_Aggro_Sorted(InAggroOwnerEntity, InSortingPolicy, [&](FCk_Handle_Aggro InAggro)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAggro, InOptionalPayload); }
        else
        { Aggro.Emplace(InAggro); }
    });

    return Aggro;
}

auto
    UCk_Utils_Aggro_UE::
    ForEach_Aggro_Sorted(
        FCk_Handle& InAggroOwnerEntity,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc)
    -> void
{
    auto AggroHandles = ForEach_Aggro(InAggroOwnerEntity, {}, FCk_Lambda_InHandle {});
    ck::algo::Sort(AggroHandles, [&](const FCk_Handle_Aggro& InA, const FCk_Handle_Aggro& InB)
    {
        switch(InSortingPolicy)
        {
            case ECk_ScoreSortingPolicy::SmallestToLargest: return Get_AggroScore(InA) < Get_AggroScore(InB);
            case ECk_ScoreSortingPolicy::LargestToSmallest: return Get_AggroScore(InA) > Get_AggroScore(InB);
        }

        return false;
    });

    ck::algo::ForEach(AggroHandles, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------

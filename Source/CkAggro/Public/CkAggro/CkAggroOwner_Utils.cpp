#include "CkAggro/CkAggroOwner_Utils.h"

#include "CkAggro/CkAggroOwner_Fragment.h"
#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AggroOwner_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AggroOwner_Params& InParams)
    -> FCk_Handle_AggroOwner
{
    RecordOfAggro_Utils::AddIfMissing(InHandle);
    InHandle.Add<ck::FFragment_AggroOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_AggroOwner_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AggroOwner_UE, FCk_Handle_AggroOwner, ck::FFragment_AggroOwner_Params, ck::FFragment_AggroOwner_Current)

// --------------------------------------------------------------------------------------------------------------------


auto
    UCk_Utils_AggroOwner_UE::
    TryGet_AggroByTarget(
        const FCk_Handle& InAggroOwnerEntity,
        const FCk_Handle& InTarget)
    -> FCk_Handle_Aggro
{
    auto Ret = FCk_Handle_Aggro{};

    RecordOfAggro_Utils::ForEach_ValidEntry(InAggroOwnerEntity, [&](const FCk_Handle_Aggro& InAggro)
    {
        if (UCk_Utils_Aggro_UE::Get_AggroTarget(InAggro) == InTarget)
        {
            Ret = InAggro;
        }
    });

    return Ret;
}

auto
    UCk_Utils_AggroOwner_UE::
    Get_BestAggro(
        const FCk_Handle& InAggroOwnerEntity)
    -> FCk_Handle_Aggro
{
    return InAggroOwnerEntity.Get<ck::FFragment_AggroOwner_Current>().Get_BestAggro();
}

auto
    UCk_Utils_AggroOwner_UE::
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
    UCk_Utils_AggroOwner_UE::
    UnbindFrom_OnNewAggroAdded(
        FCk_Handle& InAggroOwner,
        const FCk_Delegate_Aggro_OnNewAggroAdded& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnNewAggroAdded, InAggroOwner, InDelegate);
    return InAggroOwner;
}

auto
    UCk_Utils_AggroOwner_UE::
    ForEach_Aggro(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Aggro>
{
    auto Aggro = TArray<FCk_Handle_Aggro>{};

    ForEach_Aggro(InAggroOwnerEntity, InExclusionPolicy, [&](FCk_Handle_Aggro InAggro)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAggro, InOptionalPayload); }
        else
        { Aggro.Emplace(InAggro); }
    });

    return Aggro;
}

auto
    UCk_Utils_AggroOwner_UE::
    ForEach_Aggro(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc)
    -> void
{
    RecordOfAggro_Utils::ForEach_ValidEntry(InAggroOwnerEntity, [&](const FCk_Handle_Aggro& InAggro)
    {
        if (InExclusionPolicy == ECk_Aggro_ExclusionPolicy::IgnoreExcluded && InAggro.Has<ck::FTag_Aggro_Excluded>())
        { return; }

        InFunc(InAggro);
    });
}

auto
    UCk_Utils_AggroOwner_UE::
    ForEach_Aggro_Sorted(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        const FInstancedStruct& InOptionalPayload,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Aggro>
{
    auto Aggro = TArray<FCk_Handle_Aggro>{};

    ForEach_Aggro_Sorted(InAggroOwnerEntity, InExclusionPolicy, InSortingPolicy, [&](FCk_Handle_Aggro InAggro)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAggro, InOptionalPayload); }
        else
        { Aggro.Emplace(InAggro); }
    });

    return Aggro;
}

auto
    UCk_Utils_AggroOwner_UE::
    ForEach_Aggro_Sorted(
        const FCk_Handle& InAggroOwnerEntity,
        ECk_Aggro_ExclusionPolicy InExclusionPolicy,
        ECk_ScoreSortingPolicy InSortingPolicy,
        const TFunction<void(FCk_Handle_Aggro)>& InFunc)
    -> void
{
    using Utils = UCk_Utils_Aggro_UE;

    auto AggroHandles = ForEach_Aggro(InAggroOwnerEntity, InExclusionPolicy, {}, FCk_Lambda_InHandle {});
    ck::algo::Sort(AggroHandles, [&](const FCk_Handle_Aggro& InA, const FCk_Handle_Aggro& InB)
    {
        switch(InSortingPolicy)
        {
            case ECk_ScoreSortingPolicy::SmallestToLargest: return Utils::Get_AggroScore(InA) < Utils::Get_AggroScore(InB);
            case ECk_ScoreSortingPolicy::LargestToSmallest: return Utils::Get_AggroScore(InA) > Utils::Get_AggroScore(InB);
        }

        return false;
    });

    ck::algo::ForEach(AggroHandles, InFunc);
}


// --------------------------------------------------------------------------------------------------------------------

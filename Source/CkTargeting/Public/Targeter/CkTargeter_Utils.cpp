#include "CkTargeter_Utils.h"

#include "CkSignal/CkSignal_Macros.h"
#include "Targetable/CkTargetable_Fragment.h"
#include "Targetable/CkTargetable_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Targeter_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Targeter_ParamsData& InParams)
    -> FCk_Handle_Targeter
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_Targeter_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Targeter_Current>();
    });

    auto NewTargeterEntity = Cast(NewEntity);

    RecordOfTargeters_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfTargeters_Utils::Request_Connect(InHandle, NewTargeterEntity);

    return NewTargeterEntity;
}

auto
    UCk_Utils_Targeter_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleTargeter_ParamsData& InParams)
    -> TArray<FCk_Handle_Targeter>
{
    return ck::algo::Transform<TArray<FCk_Handle_Targeter>>(InParams.Get_TargeterParams(),
    [&](const FCk_Fragment_Targeter_ParamsData& InTargeterParams)
    {
        return Add(InHandle, InTargeterParams);
    });
}

auto
    UCk_Utils_Targeter_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfTargeters_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Targeter, UCk_Utils_Targeter_UE, FCk_Handle_Targeter, ck::FFragment_Targeter_Current, ck::FFragment_Targeter_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Targeter_UE::
    IsValid(
        const FCk_Targeter_BasicInfo& InTargeterInfo)
    -> bool
{
    return ck::IsValid(InTargeterInfo);
}

auto
    UCk_Utils_Targeter_UE::
    IsEqual(
        const FCk_Targeter_BasicInfo& InTargeterInfoA,
        const FCk_Targeter_BasicInfo& InTargeterInfoB)
    -> bool
{
    return InTargeterInfoA == InTargeterInfoB;
}

auto
    UCk_Utils_Targeter_UE::
    IsNotEqual(
        const FCk_Targeter_BasicInfo& InTargeterInfoA,
        const FCk_Targeter_BasicInfo& InTargeterInfoB)
    -> bool
{
    return InTargeterInfoA != InTargeterInfoB;
}

auto
    UCk_Utils_Targeter_UE::
    TryGet_Targeter(
        const FCk_Handle& InTargeterOwnerEntity,
        FGameplayTag InTargeterName)
    -> FCk_Handle_Targeter
{
    return RecordOfTargeters_Utils::Get_ValidEntry_If(InTargeterOwnerEntity, ck::algo::MatchesGameplayLabelExact{InTargeterName});
}

auto
    UCk_Utils_Targeter_UE::
    Get_CanTarget(
        const FCk_Handle_Targeter& InTargeter,
        const FCk_Handle_Targetable& InTarget)
    -> ECk_Targetable_Status
{
    if (UCk_Utils_Targetable_UE::Get_EnableDisable(InTarget) == ECk_EnableDisable::Disable)
    { return ECk_Targetable_Status::CannotTarget; }

    // Adding attachment node is deferred and may happen after setup is run, but we should not try to target this until attachment node has been added
    if (NOT InTarget.Has<ck::FTag_Targetable_IsReady>())
    { return ECk_Targetable_Status::NotYetReady; }

    const auto& TargetingQuery    =  InTargeter.Get<ck::FFragment_Targeter_Params>().Get_Params().Get_TargetingQuery();
    const auto& TargetabilityTags = UCk_Utils_Targetable_UE::Get_TargetabilityTags(InTarget);
    const auto& QueryResult       = TargetingQuery.Matches(TargetabilityTags);

    return QueryResult ?
        ECk_Targetable_Status::CanTarget :
        ECk_Targetable_Status::CannotTarget;
}

auto
    UCk_Utils_Targeter_UE::
    Get_TargetingStatus(
        const FCk_Handle_Targeter& InTargeter)
    -> ECk_Targeter_TargetingStatus
{
    return InTargeter.Get<ck::FFragment_Targeter_Current>().Get_TargetingStatus();
}

auto
    UCk_Utils_Targeter_UE::
    Get_ExtractOwners_FromTargetList(
        const FCk_Targeter_TargetList& InTargetList)
    -> TArray<FCk_Handle>
{
    return ck::algo::Transform<TArray<FCk_Handle>>(InTargetList.Get_Targets(),
    [](const FCk_Targetable_BasicInfo& InBasicInfo)
    {
        return InBasicInfo.Get_Owner();
    });
}

auto
    UCk_Utils_Targeter_UE::
    Get_ExtractTargetables_FromTargetList(
        const FCk_Targeter_TargetList& InTargetList)
    -> TArray<FCk_Handle_Targetable>
{
    return ck::algo::Transform<TArray<FCk_Handle_Targetable>>(InTargetList.Get_Targets(),
    [](const FCk_Targetable_BasicInfo& InBasicInfo)
    {
        return InBasicInfo.Get_Targetable();
    });
}

auto
    UCk_Utils_Targeter_UE::
    ForEach_Targeter(
        FCk_Handle& InTargeterOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Targeter>
{
    auto Targeters = TArray<FCk_Handle_Targeter>{};

    ForEach_Targeter(InTargeterOwnerEntity, [&](FCk_Handle_Targeter InTargeter)
    {
        Targeters.Emplace(InTargeter);

        std::ignore = InDelegate.ExecuteIfBound(InTargeter, InOptionalPayload);
    });

    return Targeters;
}

auto
    UCk_Utils_Targeter_UE::
    ForEach_Targeter(
        FCk_Handle& InTargeterOwnerEntity,
        const TFunction<void(FCk_Handle_Targeter)>& InFunc)
    -> void
{
    RecordOfTargeters_Utils::ForEach_ValidEntry(InTargeterOwnerEntity, InFunc);
}

auto
    UCk_Utils_Targeter_UE::
    FilterTargets_ByPredicate(
        const FCk_Targeter_BasicInfo& InTargeter,
        const FCk_Targeter_TargetList& InTargetList,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InTargeter_InTarget_OutResult InPredicate)
    -> FCk_Targeter_TargetList
{
    const auto& FilteredTargets = ck::algo::Filter(InTargetList.Get_Targets(),
    [&](const FCk_Targetable_BasicInfo& InTargetBasicInfo)
    {
        if (ck::Is_NOT_Valid(InTargetBasicInfo))
        { return false; }

        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InTargeter, InTargetBasicInfo, InOptionalPayload, PredicateResult);
        }

        return *PredicateResult;
    });

    return FCk_Targeter_TargetList{FilteredTargets};
}

auto
    UCk_Utils_Targeter_UE::
    SortTargets_ByPredicate(
        const FCk_Targeter_BasicInfo& InTargeter,
        FCk_Targeter_TargetList& InTargetList,
        const FInstancedStruct& InOptionalPayload,
        FCk_Predicate_InTargeter_In2Targets_OutResult InPredicate)
    -> void
{
    const auto& SortedTargets = ck::algo::Sort(InTargetList.Get_Targets(),
    [&](const FCk_Targetable_BasicInfo& InTargetBasicInfoA, const FCk_Targetable_BasicInfo& InTargetBasicInfoB)
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InTargeter, InTargetBasicInfoA, InTargetBasicInfoB, InOptionalPayload, PredicateResult);
        }

        return *PredicateResult;
    });

    InTargetList = FCk_Targeter_TargetList{SortedTargets};
}

auto
    UCk_Utils_Targeter_UE::
    IntersectTargets(
        const FCk_Targeter_TargetList& InTargetListA,
        const FCk_Targeter_TargetList& InTargetListB)
    -> FCk_Targeter_TargetList
{
    const auto Result = ck::algo::Intersect(InTargetListA.Get_Targets(), InTargetListB.Get_Targets());
    return FCk_Targeter_TargetList{Result};
}

auto
    UCk_Utils_Targeter_UE::
    ExceptTargets(
        const FCk_Targeter_TargetList& InTargetListA,
        const FCk_Targeter_TargetList& InTargetListB)
    -> FCk_Targeter_TargetList
{
    const auto Result = ck::algo::Except(InTargetListA.Get_Targets(), InTargetListB.Get_Targets());
    return FCk_Targeter_TargetList{Result};
}

auto
    UCk_Utils_Targeter_UE::
    Request_Start(
        FCk_Handle_Targeter& InTargeter)
    -> FCk_Handle_Targeter
{
    InTargeter.AddOrGet<ck::FTag_Targeter_NeedsUpdate>();
    InTargeter.Get<ck::FFragment_Targeter_Current>()._TargetingStatus = ECk_Targeter_TargetingStatus::Targeting;

    return InTargeter;
}

auto
    UCk_Utils_Targeter_UE::
    Request_Stop(
        FCk_Handle_Targeter& InTargeter)
    -> FCk_Handle_Targeter
{
    InTargeter.Get<ck::FFragment_Targeter_Current>()._TargetingStatus = ECk_Targeter_TargetingStatus::NotTargeting;

    if (InTargeter.Has<ck::FTag_Targeter_NeedsUpdate>())
    {
        InTargeter.Remove<ck::FTag_Targeter_NeedsUpdate>();

        const auto TargeterBasicInfo = FCk_Targeter_BasicInfo{InTargeter, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InTargeter)};

        // Don't fire the signal if the Targeter doesn't have any result computed yet
        if (ck::IsValid(InTargeter.Get<ck::FFragment_Targeter_Current>()._CachedTargets))
        {
            InTargeter.Get<ck::FFragment_Targeter_Current>()._CachedTargets.Reset();
            ck::UUtils_Signal_Targeter_OnTargetsQueried::Broadcast(InTargeter, ck::MakePayload(TargeterBasicInfo, FCk_Targeter_TargetList{}));
        }
    }

    return InTargeter;
}

auto
    UCk_Utils_Targeter_UE::
    Request_QueryTargets(
        FCk_Handle_Targeter& InTargeter,
        const FCk_Request_Targeter_QueryTargets& InRequest,
        const FCk_Delegate_Targeter_OnTargetsQueried& InDelegate)
    -> FCk_Handle_Targeter
{
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Targeter_OnTargetsQueried, InTargeter, InDelegate);

    InTargeter.AddOrGet<ck::FFragment_Targeter_Requests>()._Requests.Emplace(InRequest);

    return InTargeter;
}

auto
    UCk_Utils_Targeter_UE::
    BindTo_OnTargetsQueried(
        FCk_Handle_Targeter& InTargeterHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Targeter_OnTargetsQueried& InDelegate)
    -> FCk_Handle_Targeter
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Targeter_OnTargetsQueried, InTargeterHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InTargeterHandle;
}

auto
    UCk_Utils_Targeter_UE::
    UnbindFrom_OnTargetsQueried(
        FCk_Handle_Targeter& InTargeterHandle,
        const FCk_Delegate_Targeter_OnTargetsQueried& InDelegate)
    -> FCk_Handle_Targeter
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Targeter_OnTargetsQueried, InTargeterHandle, InDelegate);
    return InTargeterHandle;
}

auto
    UCk_Utils_Targeter_UE::
    Request_Cleanup(
        FCk_Handle_Targeter& InTargeter)
    -> void
{
    InTargeter.AddOrGet<ck::FTag_Targeter_NeedsCleanup>();
}

// --------------------------------------------------------------------------------------------------------------------

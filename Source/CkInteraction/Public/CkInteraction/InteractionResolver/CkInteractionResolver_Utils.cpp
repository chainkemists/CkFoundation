#include "CkInteractionResolver_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"
#include "CkInteraction/CkInteraction_Stats.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Utils.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Interaction::ResolveTargets"), STAT_Interaction_ResolveTargets, STATGROUP_CkInteraction);
DECLARE_CYCLE_STAT(TEXT("Interaction::SortByDistance"), STAT_Interaction_SortByDistance, STATGROUP_CkInteraction);
DECLARE_DWORD_COUNTER_STAT(TEXT("Interaction CanInteractWith Calls"), STAT_Interaction_CanInteractWithCalls, STATGROUP_CkInteraction);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractionResolver_UE::
    Add(
        FCk_Handle& InInteractSource,
        const FCk_InteractionResolver_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_InteractionResolver
{
    InInteractSource.Add<ck::FFragment_InteractionResolver_Params>(InParams);
    InInteractSource.Add<ck::FFragment_InteractionResolver_Current>();

    CK_ENSURE_IF_NOT(NOT InParams.Get_IntentChannelMappings().IsEmpty(),
        TEXT("InteractionResolver added to Handle [{}] has an EMPTY IntentChannelMappings. It will not function correctly!"),
        InInteractSource)
    {}

    return Cast(InInteractSource);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractionResolver_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_InteractionResolver_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_InteractionResolver
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams, InReplicates);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_InteractionResolver_UE, FCk_Handle_InteractionResolver,
    ck::FFragment_InteractionResolver_Params, ck::FFragment_InteractionResolver_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractionResolver_UE::
    Request_StartIntent(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_StartIntent& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_StopIntent(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_StopIntent& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_AddInteractTarget(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_AddInteractTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_RemoveInteractTarget(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_RemoveInteractTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_RemoveAllTargetsByChannel(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_RemoveAllTargetsByChannel& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    ResolveBestInteractTargets_Immediate(
        const FCk_Handle_InteractionResolver& InResolver,
        FGameplayTag InIntent,
        const TArray<FCk_Handle_InteractTarget>& InAvailableTargets)
    -> TArray<FCk_Handle_InteractTarget>
{
    return DoResolveTargets_Internal(InResolver, InIntent, InAvailableTargets);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Get_BestInteractTargets(
        const FCk_Handle_InteractionResolver& InResolver,
        FGameplayTag InIntent)
    -> TArray<FCk_Handle_InteractTarget>
{
    const auto& Current = InResolver.Get<ck::FFragment_InteractionResolver_Current>();
    const auto CachedTargets = Current.Get_CachedBestTargets().Find(InIntent);

    if (ck::Is_NOT_Valid(CachedTargets, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    return *CachedTargets;
}

auto
    UCk_Utils_InteractionResolver_UE::
    BindTo_OnBestTargetsChanged(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Delegate_InteractionResolver_OnBestTargetsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_InteractionResolver
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractionResolver_OnBestTargetsChanged, InResolver, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InResolver;
}

auto
    UCk_Utils_InteractionResolver_UE::
    UnbindFrom_OnBestTargetsChanged(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Delegate_InteractionResolver_OnBestTargetsChanged& InDelegate)
    -> FCk_Handle_InteractionResolver
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractionResolver_OnBestTargetsChanged, InResolver, InDelegate);
    return InResolver;
}

auto
    UCk_Utils_InteractionResolver_UE::
    DoResolveTargets_Internal(
        const FCk_Handle_InteractionResolver& InResolver,
        const FGameplayTag& InIntent,
        const TArray<FCk_Handle_InteractTarget>& InAvailableTargets)
    -> TArray<FCk_Handle_InteractTarget>
{
    SCOPE_CYCLE_COUNTER(STAT_Interaction_ResolveTargets);

    CK_ENSURE_IF_NOT(ck::IsValid(InResolver),
        TEXT("Cannot resolve targets for invalid resolver handle"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InIntent),
        TEXT("Cannot resolve targets for invalid intent"))
    { return {}; }

    const auto& Params = InResolver.Get<ck::FFragment_InteractionResolver_Params>();
    const auto& Mappings = Params.Get_IntentChannelMappings();

    const auto* IntentMapping = Mappings.FindByPredicate([&](const FCk_InteractionResolver_IntentChannelMapping& InMapping)
    {
        return InMapping.Get_Intent().MatchesTagExact(InIntent);
    });

    if (ck::Is_NOT_Valid(IntentMapping, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::interaction::VeryVerbose(TEXT("No mapping found for intent [{}]"), InIntent);
        return {};
    }

    const auto DistanceSorting = IntentMapping->Get_DistanceSorting();
    const auto MaxConcurrentInteractions = IntentMapping->Get_MaxConcurrentInteractions();

    CK_ENSURE_IF_NOT(MaxConcurrentInteractions > 0,
        TEXT("MaxConcurrentInteractions must be greater than 0 for intent [{}]"), InIntent)
    { return {}; }

    auto ValidTargets = TArray<FCk_Handle_InteractTarget>{};

    // The mapping's channel order IS the priority order.
    for (const auto& Channel : IntentMapping->Get_Channels())
    {
        for (const auto& Target : InAvailableTargets)
        {
            if (ck::Is_NOT_Valid(Target))
            { continue; }

            if (NOT UCk_Utils_InteractTarget_UE::Get_InteractionChannel(Target).MatchesTagExact(Channel))
            { continue; }

            // The three non-CanInteractWith results below are accepted so a target with an ACTIVE
            // interaction stays in the resolved set — otherwise a re-evaluation drops it and the
            // bridge code cancels its live interaction. This only reaches consumers that compose an
            // InteractSource, because AlreadyExists is decided against the InteractSource CAST of
            // InSource; the pin below the sort is what protects everyone else.
            INC_DWORD_STAT(STAT_Interaction_CanInteractWithCalls);
            const auto CanInteractResult = UCk_Utils_InteractTarget_UE::Get_CanInteractWith(Target, InResolver);
            if (CanInteractResult != ECk_CanInteractWithResult::CanInteractWith &&
                CanInteractResult != ECk_CanInteractWithResult::AlreadyExists &&
                CanInteractResult != ECk_CanInteractWithResult::TargetRejectedSecondInteraction &&
                CanInteractResult != ECk_CanInteractWithResult::SourceRejectedSecondInteraction)
            { continue; }

            ValidTargets.Add(Target);
        }

        if (ValidTargets.Num() >= MaxConcurrentInteractions)
        { break; }
    }

    if (DistanceSorting == ECk_InteractionResolver_DistanceSorting::Enabled && ValidTargets.Num() > 1)
    { DoSortByDistance(InResolver, ValidTargets); }

    // An interaction this resolver ALREADY started outranks proximity. Without this, a nearer
    // same-channel target appearing mid-hold sorts ahead and the truncation below evicts the live
    // one; the consumer bridge cancels what it no longer sees, and a cancelled Timed interaction is
    // destroyed along with its progress -- so a hold resets to zero for a reason the player cannot
    // perceive. The A-to-B retarget case is unaffected: the target you looked away from leaves
    // AvailableTargets entirely, so it never reaches this pin.
    //
    // Deliberately NOT keyed on Get_CanInteractWith's AlreadyExists result: that compares the
    // interaction's source against the INTERACT-SOURCE CAST of InSource, so it is unreachable for
    // any consumer that composes no InteractSource feature. Ask the interaction record directly.
    if (ValidTargets.Num() > MaxConcurrentInteractions)
    {
        auto Pinned = TArray<FCk_Handle_InteractTarget>{};
        auto Unpinned = TArray<FCk_Handle_InteractTarget>{};

        for (const auto& Target : ValidTargets)
        {
            const auto& LiveInteraction = UCk_Utils_Interaction_UE::TryGet(Target, InResolver, Target,
                UCk_Utils_InteractTarget_UE::Get_InteractionChannel(Target));

            if (ck::IsValid(LiveInteraction))
            { Pinned.Emplace(Target); }
            else
            { Unpinned.Emplace(Target); }
        }

        if (NOT Pinned.IsEmpty())
        {
            Pinned.Append(Unpinned);
            ValidTargets = MoveTemp(Pinned);
        }

        ValidTargets.SetNum(MaxConcurrentInteractions);
    }

    return ValidTargets;
}

auto
    UCk_Utils_InteractionResolver_UE::
    DoSortByDistance(
        const FCk_Handle_InteractionResolver& InResolver,
        TArray<FCk_Handle_InteractTarget>& InOutTargets)
    -> void
{
    SCOPE_CYCLE_COUNTER(STAT_Interaction_SortByDistance);

    const auto SourceHandle = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InResolver);

    if (NOT UCk_Utils_Transform_UE::Has(SourceHandle))
    {
        CK_ENSURE_IF_NOT(false,
            TEXT("Distance sorting enabled but source [{}] does NOT have Transform feature"), SourceHandle)
        { return; }
    }

    const auto SourceLocation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(SourceHandle);

    InOutTargets.Sort([&](const FCk_Handle_InteractTarget& InA, const FCk_Handle_InteractTarget& InB)
    {
        const auto TargetOwnerA = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InA);
        const auto TargetOwnerB = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InB);

        if (NOT UCk_Utils_Transform_UE::Has(TargetOwnerA) || NOT UCk_Utils_Transform_UE::Has(TargetOwnerB))
        { return false; }

        const auto LocationA = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(TargetOwnerA);
        const auto LocationB = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(TargetOwnerB);

        const auto DistanceA = FVector::Dist(SourceLocation, LocationA);
        const auto DistanceB = FVector::Dist(SourceLocation, LocationB);

        return DistanceA < DistanceB;
    });
}

// --------------------------------------------------------------------------------------------------------------------

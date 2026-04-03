#include "CkInteractionResolver_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInteraction/CkInteraction_Log.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractionResolver_UE::
    Add(
        FCk_Handle& InInteractSource,
        const FCk_InteractionResolver_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_InteractionResolver
{
    CK_ENSURE_IF_NOT(NOT InParams.Get_IntentChannelMappings().IsEmpty(),
        TEXT("Unable to add InteractionResolver to Handle [{}] since IntentChannelMappings is empty"),
        InInteractSource)
    { return {}; }

    InInteractSource.Add<ck::FFragment_InteractionResolver_Params>(InParams);
    InInteractSource.Add<ck::FFragment_InteractionResolver_Current>();

    return Cast(InInteractSource);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_InteractionResolver_UE, FCk_Handle_InteractionResolver,
    ck::FFragment_InteractionResolver_Params, ck::FFragment_InteractionResolver_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractionResolver_UE::
    Request_StartIntent(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_StartIntent& InRequest)
    -> void
{
    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_StopIntent(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_StopIntent& InRequest)
    -> void
{
    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_AddInteractTarget(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_AddInteractTarget& InRequest)
    -> void
{
    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_RemoveInteractTarget(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_RemoveInteractTarget& InRequest)
    -> void
{
    InResolver.AddOrGet<ck::FFragment_InteractionResolver_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_InteractionResolver_UE::
    Request_RemoveAllTargetsByChannel(
        FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_RemoveAllTargetsByChannel& InRequest)
    -> void
{
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
    CK_ENSURE_IF_NOT(ck::IsValid(InResolver),
        TEXT("Cannot resolve targets for invalid resolver handle"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InIntent),
        TEXT("Cannot resolve targets for invalid intent"))
    { return {}; }

    const auto& Params = InResolver.Get<ck::FFragment_InteractionResolver_Params>();
    const auto& Mappings = Params.Get_IntentChannelMappings();

    // Find the mapping for this intent
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

    // Iterate through channels in priority order
    for (const auto& Channel : IntentMapping->Get_Channels())
    {
        // Find targets that match this channel from the provided available targets
        for (const auto& Target : InAvailableTargets)
        {
            // Skip invalid targets
            if (NOT ck::IsValid(Target))
            { continue; }

            if (NOT UCk_Utils_InteractTarget_UE::Get_InteractionChannel(Target).MatchesTagExact(Channel))
            { continue; }

            // Allow targets that already have an active interaction to remain in the resolved set.
            // Without this, re-evaluation triggered by an unrelated intent stopping would drop
            // targets with active interactions, causing the bridge code to cancel those interactions.
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

    if (ValidTargets.Num() > MaxConcurrentInteractions)
    { ValidTargets.SetNum(MaxConcurrentInteractions); }

    return ValidTargets;
}

auto
    UCk_Utils_InteractionResolver_UE::
    DoSortByDistance(
        const FCk_Handle_InteractionResolver& InResolver,
        TArray<FCk_Handle_InteractTarget>& InOutTargets)
    -> void
{
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

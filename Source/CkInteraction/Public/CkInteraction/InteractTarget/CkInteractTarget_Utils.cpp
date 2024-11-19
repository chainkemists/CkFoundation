#include "CkInteractTarget_Utils.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment.h"
#include "CkInteraction/CkInteraction_Log.h"
#include "CkInteraction/InteractSource/CkInteractSource_Utils.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    Add(
        FCk_Handle& InInteractTargetOwner,
        const FCk_Fragment_InteractTarget_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_InteractTarget
{
    auto NewInteractTargetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_InteractTarget>(InInteractTargetOwner);

    NewInteractTargetEntity.Add<ck::FFragment_InteractTarget_Params>(InParams);
    NewInteractTargetEntity.Add<ck::FFragment_InteractTarget_Current>();
    NewInteractTargetEntity.Add<ck::FTag_InteractTarget_RequiresSetup>();

    UCk_Utils_GameplayLabel_UE::Add(NewInteractTargetEntity, InParams.Get_InteractionChannel());

    RecordOfInteractTargets_Utils::AddIfMissing(InInteractTargetOwner, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfInteractTargets_Utils::Request_Connect(InInteractTargetOwner, NewInteractTargetEntity);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::interaction::VeryVerbose
        (
            TEXT("Skipping creation of InteractTarget Rep Fragment on Entity [{}] because it's set to [{}]"),
            NewInteractTargetEntity,
            InReplicates
        );

        return NewInteractTargetEntity;
    }

    TryAddReplicatedFragment<UCk_Fragment_InteractTarget_Rep>(NewInteractTargetEntity);
    return NewInteractTargetEntity;
}

auto
    UCk_Utils_InteractTarget_UE::
    AddMultiple(
        FCk_Handle& InInteractTargetOwner,
        const FCk_Fragment_MultipleInteractTarget_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_InteractTarget>
{
    return ck::algo::Transform<TArray<FCk_Handle_InteractTarget>>(
        InParams.Get_InteractTargetParams(), [&](const FCk_Fragment_InteractTarget_ParamsData& InParam)
    {
        return Add(InInteractTargetOwner, InParam, InReplicates);
    });
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_InteractTarget_UE, FCk_Handle_InteractTarget,
    ck::FFragment_InteractTarget_Params, ck::FFragment_InteractTarget_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    Request_StartInteraction(
        FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Try_InteractTarget_StartInteraction& InRequest)
    -> FCk_Handle_InteractTarget
{
    InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests.Emplace(InRequest);
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Request_CancelInteraction(
        FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Request_InteractTarget_CancelInteraction& InRequest)
    -> FCk_Handle_InteractTarget
{
    InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests.Emplace(InRequest);
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Request_CancelAllInteractions(
        FCk_Handle_InteractTarget& InInteractTarget)
    -> FCk_Handle_InteractTarget
{
    auto& Requests = InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests;

    UCk_Utils_Interaction_UE::ForEach(InInteractTarget, [&](const FCk_Handle_Interaction& InInteraction)
    {
        const auto& Source = UCk_Utils_Interaction_UE::Get_InteractionSource(InInteraction);
        Requests.Emplace(FCk_Request_InteractTarget_CancelInteraction{Source});
    });
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_Enabled(
        const FCk_Handle_InteractTarget& InHandle)
    -> ECk_EnableDisable
{
    return InHandle.Get<ck::FFragment_InteractTarget_Current>()._Enabled;
}

auto
    UCk_Utils_InteractTarget_UE::
    Set_Enabled(
        FCk_Handle_InteractTarget& InHandle,
        ECk_EnableDisable InEnabled)
    -> void
{
    InHandle.Get<ck::FFragment_InteractTarget_Current>()._Enabled = InEnabled;

    if (InEnabled == ECk_EnableDisable::Disable)
    {
        Request_CancelAllInteractions(InHandle);
    }
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_InteractionChannel(
        const FCk_Handle_InteractTarget& InHandle)
    -> const FGameplayTag&
{
    return InHandle.Get<ck::FFragment_InteractTarget_Params>().Get_Params().Get_InteractionChannel();
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_CanInteractWith(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InSource)
    -> ECk_CanInteractWithResult
{
    if (ck::Is_NOT_Valid(InTarget))
    { return ECk_CanInteractWithResult::TargetNotValid; }

    if (ck::Is_NOT_Valid(InSource))
    { return ECk_CanInteractWithResult::SourceNotValid; }

    if (Get_Enabled(InTarget) == ECk_EnableDisable::Disable)
    { return ECk_CanInteractWithResult::TargetDisabled; }

    const auto& InInteractSource = UCk_Utils_InteractSource_UE::Cast(InSource);
    if (ck::IsValid(InInteractSource))
    {
        const auto& SourceChannel = UCk_Utils_InteractSource_UE::Get_InteractionChannel(InInteractSource);
        const auto& TargetChannel = Get_InteractionChannel(InTarget);

        if (NOT TargetChannel.MatchesTagExact(SourceChannel))
        { return ECk_CanInteractWithResult::ChannelMismatch; }

        // If no multiple interactions, don't allow interactions if any are current or pending
        if (UCk_Utils_InteractSource_UE::Get_InteractionCountPerSourcePolicy(InInteractSource) == ECk_InteractionSource_ConcurrentInteractionsPolicy::SingleInteraction)
        {
            if (UCk_Utils_InteractSource_UE::Get_CurrentInteractions(InInteractSource).Num() + UCk_Utils_InteractSource_UE::Get_PendingInteractions(InInteractSource).Num() > 0)
            { return ECk_CanInteractWithResult::MultipleInteractionsDisabledForSource; }
        }
    }

    // TODO: This only works on auth currently, will need to duplicate interact targets to allow clients to filter this way
    const auto& MatchingInteraction = UCk_Utils_Interaction_UE::TryGet(InTarget, InInteractSource, InTarget, Get_InteractionChannel(InTarget));
    if (ck::IsValid(MatchingInteraction))
    {
        // No duplicate interactions
        return ECk_CanInteractWithResult::AlreadyExists;
    }

    // If no multiple interactions, don't allow interactions if any are current
    if (InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params().Get_ConcurrentInteractionsPolicy() == ECk_InteractionTarget_ConcurrentInteractionsPolicy::SingleInteraction &&
        UCk_Utils_Interaction_UE::RecordOfInteractions_Utils::Get_ValidEntriesCount(InTarget) > 0)
    { return ECk_CanInteractWithResult::MultipleInteractionsDisabledForTarget; }

    return ECk_CanInteractWithResult::CanInteractWith;
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_CurrentInteractions(
        FCk_Handle_InteractTarget& InHandle)
    -> TArray<FCk_Handle_Interaction>
{
    return UCk_Utils_Interaction_UE::ForEach(InHandle, {}, {});
}

auto
    UCk_Utils_InteractTarget_UE::
    TryGet(
        const FCk_Handle& InInteractTargetOwner,
        FGameplayTag InInteractionChannel)
    -> FCk_Handle_InteractTarget
{
    return RecordOfInteractTargets_Utils::Get_ValidEntry_If(InInteractTargetOwner,
        ck::algo::MatchesGameplayLabelExact{InInteractionChannel});
}

auto
    UCk_Utils_InteractTarget_UE::
    TryGet_Interaction(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InInteractSource)
    -> FCk_Handle_Interaction
{
    return UCk_Utils_Interaction_UE::TryGet(InTarget, InInteractSource, InTarget, Get_InteractionChannel(InTarget));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    BindTo_OnNewInteraction(
        FCk_Handle_InteractTarget& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractTarget_OnNewInteraction& InDelegate)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractTarget_OnNewInteraction, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_InteractTarget_UE::
    UnbindFrom_OnNewInteraction(
        FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnNewInteraction& InDelegate)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractTarget_OnNewInteraction, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_InteractTarget_UE::
    BindTo_OnInteractionFinished(
        FCk_Handle_InteractTarget& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractTarget_OnInteractionFinished& InDelegate)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractTarget_OnInteractionFinished, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_InteractTarget_UE::
    UnbindFrom_OnInteractionFinished(
        FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnInteractionFinished& InDelegate)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractTarget_OnInteractionFinished, InHandle, InDelegate);
    return InHandle;
}


// --------------------------------------------------------------------------------------------------------------------